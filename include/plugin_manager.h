#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

class LuaBridge;

class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(LuaBridge *luaBridge, QObject *parent = nullptr);
    ~PluginManager();

    bool loadPlugins(const QString &pluginDir);
    bool loadPlugin(const QString &pluginPath);
    void unloadPlugin(const QString &pluginName);
    void reloadPlugins();

    QStringList loadedPlugins() const;
    QStringList availablePlugins() const;
    bool isPluginLoaded(const QString &pluginName) const;

    bool isPluginEnabled(const QString &pluginName) const;
    void setPluginEnabled(const QString &pluginName, bool enabled);

    QString lastError() const;
    QStringList getPluginErrors() const;

signals:

    void pluginLoaded(const QString &pluginName);
    void pluginUnloaded(const QString &pluginName);
    void pluginError(const QString &pluginName, const QString &error);

private slots:

    void cleanupFailedPlugins();

private:
    LuaBridge *m_luaBridge;
    QString m_pluginDirectory;
    QStringList m_loadedPlugins;
    QStringList m_availablePlugins;
    QMap<QString, bool> m_pluginEnabled;
    QMap<QString, QString> m_pluginErrors;
    QString m_lastError;
    QTimer *m_cleanupTimer;

    void scanPluginDirectory(const QString &dir);
    bool isValidPluginFile(const QString &filePath) const;
    QString getPluginNameFromPath(const QString &filePath) const;

    bool validatePlugin(const QString &pluginPath);
    bool executePluginFile(const QString &pluginPath);
    bool initializePlugin(const QString &pluginName);
    bool cleanupPlugin(const QString &pluginName);

    void setError(const QString &error);
    void setPluginError(const QString &pluginName, const QString &error);
    void clearPluginError(const QString &pluginName);
};

#endif 