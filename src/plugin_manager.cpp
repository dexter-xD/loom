#include "plugin_manager.h"
#include "lua_bridge.h"
#include "debug_log.h"
#include <QFileInfo>
#include <QDirIterator>
#include <QCoreApplication>

PluginManager::PluginManager(LuaBridge *luaBridge, QObject *parent)
    : QObject(parent)
    , m_luaBridge(luaBridge)
    , m_cleanupTimer(new QTimer(this))
{
    if (!m_luaBridge) {
        LOG_ERROR("PluginManager: LuaBridge is null");
        return;
    }

    m_cleanupTimer->setSingleShot(true);
    m_cleanupTimer->setInterval(5000); 
    connect(m_cleanupTimer, &QTimer::timeout, this, &PluginManager::cleanupFailedPlugins);

    DEBUG_LOG_PLUGIN("PluginManager initialized");
}

PluginManager::~PluginManager()
{

    for (const QString &pluginName : m_loadedPlugins) {
        cleanupPlugin(pluginName);
    }

    DEBUG_LOG_PLUGIN("PluginManager destroyed");
}

bool PluginManager::loadPlugins(const QString &pluginDir)
{
    m_pluginDirectory = pluginDir;

    QDir dir(pluginDir);
    if (!dir.exists()) {
        setError(QString("Plugin directory does not exist: %1").arg(pluginDir));
        LOG_ERROR(m_lastError);
        return false;
    }

    DEBUG_LOG_PLUGIN("Loading plugins from directory:" << pluginDir);

    scanPluginDirectory(pluginDir);

    int loadedCount = 0;
    for (const QString &pluginPath : m_availablePlugins) {
        QString pluginName = getPluginNameFromPath(pluginPath);

        if (!isPluginEnabled(pluginName)) {
            DEBUG_LOG_PLUGIN("Plugin disabled, skipping:" << pluginName);
            continue;
        }

        if (loadPlugin(pluginPath)) {
            loadedCount++;
        }
    }

    LOG_INFO("Loaded" << loadedCount << "plugins out of" << m_availablePlugins.size() << "available");
    return loadedCount > 0 || m_availablePlugins.isEmpty();
}

bool PluginManager::loadPlugin(const QString &pluginPath)
{
    if (!m_luaBridge) {
        setError("LuaBridge not available");
        return false;
    }

    QString pluginName = getPluginNameFromPath(pluginPath);

    if (m_loadedPlugins.contains(pluginName)) {
        DEBUG_LOG_PLUGIN("Plugin already loaded:" << pluginName);
        return true;
    }

    bool errorRecovery = true;
    if (m_luaBridge) {
        errorRecovery = m_luaBridge->getConfigBool("plugins.error_recovery", true);
    }

    try {

        if (!validatePlugin(pluginPath)) {
            setPluginError(pluginName, QString("Plugin validation failed: %1").arg(m_lastError));
            if (!errorRecovery) {
                return false;
            }
            DEBUG_LOG_PLUGIN("Plugin validation failed but continuing due to error recovery:" << pluginName);
        }

        if (!executePluginFile(pluginPath)) {
            setPluginError(pluginName, QString("Plugin execution failed: %1").arg(m_lastError));
            if (!errorRecovery) {
                return false;
            }
            DEBUG_LOG_PLUGIN("Plugin execution failed but continuing due to error recovery:" << pluginName);
            return false; 
        }

        if (!initializePlugin(pluginName)) {
            setPluginError(pluginName, QString("Plugin initialization failed: %1").arg(m_lastError));
            if (!errorRecovery) {
                return false;
            }
            DEBUG_LOG_PLUGIN("Plugin initialization failed but plugin loaded:" << pluginName);

        }

        m_loadedPlugins.append(pluginName);
        clearPluginError(pluginName);

        LOG_INFO("Plugin loaded successfully:" << pluginName);
        emit pluginLoaded(pluginName);

        return true;

    } catch (const std::exception &e) {
        setPluginError(pluginName, QString("Plugin loading exception: %1").arg(e.what()));
        LOG_ERROR("Exception while loading plugin" << pluginName << ":" << e.what());
        return false;
    } catch (...) {
        setPluginError(pluginName, "Unknown plugin loading error");
        LOG_ERROR("Unknown exception while loading plugin" << pluginName);
        return false;
    }
}

void PluginManager::unloadPlugin(const QString &pluginName)
{
    if (!m_loadedPlugins.contains(pluginName)) {
        DEBUG_LOG_PLUGIN("Plugin not loaded:" << pluginName);
        return;
    }

    if (cleanupPlugin(pluginName)) {
        m_loadedPlugins.removeAll(pluginName);
        DEBUG_LOG_PLUGIN("Plugin unloaded successfully:" << pluginName);
        emit pluginUnloaded(pluginName);
    } else {
        setPluginError(pluginName, QString("Plugin cleanup failed: %1").arg(m_lastError));
        emit pluginError(pluginName, m_lastError);
    }
}

void PluginManager::reloadPlugins()
{
    DEBUG_LOG_PLUGIN("Reloading all plugins");

    QStringList currentPlugins = m_loadedPlugins;
    for (const QString &pluginName : currentPlugins) {
        unloadPlugin(pluginName);
    }

    m_availablePlugins.clear();

    if (!m_pluginDirectory.isEmpty()) {
        loadPlugins(m_pluginDirectory);
    }
}

QStringList PluginManager::loadedPlugins() const
{
    return m_loadedPlugins;
}

QStringList PluginManager::availablePlugins() const
{
    QStringList names;
    for (const QString &path : m_availablePlugins) {
        names.append(getPluginNameFromPath(path));
    }
    return names;
}

bool PluginManager::isPluginLoaded(const QString &pluginName) const
{
    return m_loadedPlugins.contains(pluginName);
}

bool PluginManager::isPluginEnabled(const QString &pluginName) const
{

    if (m_luaBridge) {

        bool globalEnabled = m_luaBridge->getConfigBool("plugins.enabled", true);
        if (!globalEnabled) {
            return false;
        }

        QString configKey = QString("plugins.%1.enabled").arg(pluginName);
        return m_luaBridge->getConfigBool(configKey, true); 
    }

    return m_pluginEnabled.value(pluginName, true);
}

void PluginManager::setPluginEnabled(const QString &pluginName, bool enabled)
{
    m_pluginEnabled[pluginName] = enabled;

    if (!enabled && isPluginLoaded(pluginName)) {
        unloadPlugin(pluginName);
    }

    else if (enabled && !isPluginLoaded(pluginName)) {

        for (const QString &path : m_availablePlugins) {
            if (getPluginNameFromPath(path) == pluginName) {
                loadPlugin(path);
                break;
            }
        }
    }
}

QString PluginManager::lastError() const
{
    return m_lastError;
}

QStringList PluginManager::getPluginErrors() const
{
    QStringList errors;
    for (auto it = m_pluginErrors.constBegin(); it != m_pluginErrors.constEnd(); ++it) {
        errors.append(QString("%1: %2").arg(it.key(), it.value()));
    }
    return errors;
}

void PluginManager::cleanupFailedPlugins()
{

    QStringList failedPlugins;
    for (const QString &pluginName : m_loadedPlugins) {
        if (m_pluginErrors.contains(pluginName)) {
            failedPlugins.append(pluginName);
        }
    }

    for (const QString &pluginName : failedPlugins) {
        m_loadedPlugins.removeAll(pluginName);
        DEBUG_LOG_PLUGIN("Removed failed plugin from loaded list:" << pluginName);
    }
}

void PluginManager::scanPluginDirectory(const QString &dir)
{
    m_availablePlugins.clear();

    QDirIterator iterator(dir, QStringList() << "*.lua", QDir::Files);
    while (iterator.hasNext()) {
        QString filePath = iterator.next();
        if (isValidPluginFile(filePath)) {
            m_availablePlugins.append(filePath);
            DEBUG_LOG_PLUGIN("Found plugin file:" << filePath);
        }
    }

    DEBUG_LOG_PLUGIN("Scanned plugin directory, found" << m_availablePlugins.size() << "plugin files");
}

bool PluginManager::isValidPluginFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        return false;
    }

    if (fileInfo.suffix().toLower() != "lua") {
        return false;
    }

    if (fileInfo.size() == 0) {
        return false;
    }

    return true;
}

QString PluginManager::getPluginNameFromPath(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);
    return fileInfo.baseName();
}

bool PluginManager::validatePlugin(const QString &pluginPath)
{
    if (!isValidPluginFile(pluginPath)) {
        setError("Invalid plugin file");
        return false;
    }

    return true;
}

bool PluginManager::executePluginFile(const QString &pluginPath)
{
    if (!m_luaBridge) {
        setError("LuaBridge not available");
        return false;
    }

    if (!m_luaBridge->executeFile(pluginPath)) {
        setError(QString("Failed to execute plugin file: %1").arg(m_luaBridge->lastError()));
        return false;
    }

    return true;
}

bool PluginManager::initializePlugin(const QString &pluginName)
{
    if (!m_luaBridge) {
        setError("LuaBridge not available");
        return false;
    }

    QString initCode = QString(
        "if %1 and type(%1.initialize) == 'function' then "
        "  local success, err = pcall(%1.initialize) "
        "  if not success then "
        "    error('Plugin initialization failed: ' .. tostring(err)) "
        "  end "
        "end"
    ).arg(pluginName);

    if (!m_luaBridge->executeString(initCode)) {
        setError(QString("Plugin initialization failed: %1").arg(m_luaBridge->lastError()));
        return false;
    }

    DEBUG_LOG_PLUGIN("Plugin initialized:" << pluginName);
    return true;
}

bool PluginManager::cleanupPlugin(const QString &pluginName)
{
    if (!m_luaBridge) {
        setError("LuaBridge not available");
        return false;
    }

    QString cleanupCode = QString(
        "if %1 and type(%1.cleanup) == 'function' then "
        "  local success, err = pcall(%1.cleanup) "
        "  if not success then "
        "    if editor and editor.debug_log then "
        "      editor.debug_log('Plugin cleanup warning: ' .. tostring(err)) "
        "    end "
        "  end "
        "end "
        "%1 = nil"
    ).arg(pluginName);

    if (!m_luaBridge->executeString(cleanupCode)) {
        setError(QString("Plugin cleanup failed: %1").arg(m_luaBridge->lastError()));
        return false;
    }

    DEBUG_LOG_PLUGIN("Plugin cleaned up:" << pluginName);
    return true;
}

void PluginManager::setError(const QString &error)
{
    m_lastError = error;
    LOG_ERROR("PluginManager error:" << error);
}

void PluginManager::setPluginError(const QString &pluginName, const QString &error)
{
    m_pluginErrors[pluginName] = error;
    setError(error);
    emit pluginError(pluginName, error);

    if (!m_cleanupTimer->isActive()) {
        m_cleanupTimer->start();
    }
}

void PluginManager::clearPluginError(const QString &pluginName)
{
    m_pluginErrors.remove(pluginName);
}