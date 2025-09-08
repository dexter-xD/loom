#include "lua_bridge.h"
#include "syntax_highlighter.h"
#include "plugin_manager.h"
#include "debug_log.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

static LuaBridge *g_bridge = nullptr;

LuaBridge::LuaBridge(QObject *parent)
    : QObject(parent)
    , m_lua(nullptr)
    , m_currentCursorPosition(1, 1)
    , m_syntaxHighlighter(nullptr)
    , m_pluginManager(nullptr)
    , m_nextTimerId(1)
{
    g_bridge = this;
}

LuaBridge::~LuaBridge()
{

    for (auto timer : m_timers) {
        timer->stop();
        delete timer;
    }
    m_timers.clear();

    if (m_lua) {
        lua_close(m_lua);
        m_lua = nullptr;
    }
    g_bridge = nullptr;
}

bool LuaBridge::initialize()
{

    m_lua = luaL_newstate();
    if (!m_lua) {
        m_lastError = "Failed to create Lua state";
        return false;
    }

    luaL_openlibs(m_lua);

    setupLuaPath();

    registerEditorAPI();

    DEBUG_LOG_LUA("Lua bridge initialized successfully");
    return true;
}

bool LuaBridge::loadConfig(const QString &configPath)
{
    if (!m_lua) {
        m_lastError = "Lua state not initialized";
        return false;
    }

    if (!QFile::exists(configPath)) {
        DEBUG_LOG_LUA("Config file not found:" << configPath);
        return true; 
    }

    int result = luaL_dofile(m_lua, configPath.toUtf8().constData());
    if (result != 0) {
        handleLuaError("Loading config file");
        return false;
    }

    lua_getglobal(m_lua, "config");
    bool hasConfig = lua_istable(m_lua, -1);
    lua_pop(m_lua, 1);

    lua_getglobal(m_lua, "get_config");
    bool hasGetConfig = lua_isfunction(m_lua, -1);
    lua_pop(m_lua, 1);

    if (!hasConfig) {
        DEBUG_LOG_LUA("Config table not found in configuration file");
    }

    if (!hasGetConfig) {
        DEBUG_LOG_LUA("get_config function not found in configuration file");
    }

    DEBUG_LOG_LUA("Config loaded successfully from:" << configPath);
    DEBUG_LOG_LUA("Config table available:" << hasConfig);
    DEBUG_LOG_LUA("get_config function available:" << hasGetConfig);
    return true;
}

bool LuaBridge::executeString(const QString &luaCode)
{
    if (!m_lua) {
        m_lastError = "Lua state not initialized";
        return false;
    }

    int result = luaL_dostring(m_lua, luaCode.toUtf8().constData());
    if (result != 0) {
        handleLuaError("Executing Lua string");
        return false;
    }

    return true;
}

void LuaBridge::registerEditorAPI()
{
    if (!m_lua) {
        return;
    }

    lua_newtable(m_lua);

    registerFunction("open_file", lua_openFile);
    registerFunction("save_file", lua_saveFile);

    registerFunction("get_text", lua_getText);
    registerFunction("set_text", lua_setText);
    registerFunction("get_cursor_position", lua_getCursorPosition);
    registerFunction("set_cursor_position", lua_setCursorPosition);

    registerFunction("set_status_text", lua_setStatusText);

    registerFunction("add_syntax_rule", lua_addSyntaxRule);
    registerFunction("clear_syntax_rules", lua_clearSyntaxRules);

    registerFunction("register_event_handler", lua_registerEventHandler);

    registerFunction("create_timer", lua_createTimer);
    registerFunction("stop_timer", lua_stopTimer);

    registerFunction("debug_log", lua_debugLog);

    registerFunction("set_theme", lua_setTheme);
    registerFunction("get_theme", lua_getTheme);
    registerFunction("toggle_theme", lua_toggleTheme);

    lua_newtable(m_lua);
    lua_pushcfunction(m_lua, lua_registerEventHandler);
    lua_setfield(m_lua, -2, "connect");
    lua_setglobal(m_lua, "events");

    lua_newtable(m_lua);
    lua_pushcfunction(m_lua, lua_createTimer);
    lua_setfield(m_lua, -2, "create");
    lua_pushcfunction(m_lua, lua_stopTimer);
    lua_setfield(m_lua, -2, "stop");
    lua_setglobal(m_lua, "timer");

    lua_newtable(m_lua);
    lua_pushcfunction(m_lua, lua_listPlugins);
    lua_setfield(m_lua, -2, "list");
    lua_pushcfunction(m_lua, lua_isPluginLoaded);
    lua_setfield(m_lua, -2, "is_loaded");
    lua_pushcfunction(m_lua, lua_getPluginConfig);
    lua_setfield(m_lua, -2, "get_config");
    lua_setglobal(m_lua, "plugins");

    lua_setglobal(m_lua, "editor");

}

void LuaBridge::emitEvent(const QString &eventName, const QVariantList &args)
{
    if (!m_lua) {
        return;
    }

    QStringList handlers = m_eventHandlers.value(eventName);

    for (const QString &handlerFunction : handlers) {

        lua_getglobal(m_lua, handlerFunction.toUtf8().constData());

        if (lua_isfunction(m_lua, -1)) {

            lua_pushstring(m_lua, eventName.toUtf8().constData());

            for (const QVariant &arg : args) {
                if (arg.type() == QVariant::String) {
                    lua_pushstring(m_lua, arg.toString().toUtf8().constData());
                } else if (arg.type() == QVariant::Int) {
                    lua_pushinteger(m_lua, arg.toInt());
                } else if (arg.type() == QVariant::Double) {
                    lua_pushnumber(m_lua, arg.toDouble());
                } else if (arg.type() == QVariant::Bool) {
                    lua_pushboolean(m_lua, arg.toBool());
                } else {

                    lua_pushstring(m_lua, arg.toString().toUtf8().constData());
                }
            }

            int numArgs = 1 + args.size(); 
            if (lua_pcall(m_lua, numArgs, 0, 0) != 0) {
                QString error = QString("Error calling event handler '%1' for event '%2': %3")
                    .arg(handlerFunction)
                    .arg(eventName)
                    .arg(lua_tostring(m_lua, -1));
                DEBUG_LOG_LUA(error);
                lua_pop(m_lua, 1); 
            }
        } else {

            lua_pop(m_lua, 1); 
        }
    }
}

void LuaBridge::registerEventHandler(const QString &eventName, const QString &handlerFunction)
{

    if (!m_eventHandlers.contains(eventName)) {
        m_eventHandlers[eventName] = QStringList();
    }

    if (!m_eventHandlers[eventName].contains(handlerFunction)) {
        m_eventHandlers[eventName].append(handlerFunction);
    }
}

QString LuaBridge::lastError() const
{
    return m_lastError;
}

void LuaBridge::setupLuaPath()
{
    if (!m_lua) {
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString configDir = appDir + "/config";
    QString pluginDir = appDir + "/plugins";
    
    QString systemConfigDir = "/usr/share/loom/config";
    QString systemPluginDir = "/usr/share/loom/plugins";

    QString luaPath = QString("%1/?.lua;%2/?.lua;%3/?.lua;%4/?.lua;").arg(configDir, pluginDir, systemConfigDir, systemPluginDir);

    lua_getglobal(m_lua, "package");
    lua_pushstring(m_lua, luaPath.toUtf8().constData());
    lua_setfield(m_lua, -2, "path");
    lua_pop(m_lua, 1);

}

void LuaBridge::registerFunction(const QString &name, lua_CFunction func)
{
    if (!m_lua || !func) {
        return;
    }

    lua_pushcfunction(m_lua, func);
    lua_setfield(m_lua, -2, name.toUtf8().constData());
}

void LuaBridge::handleLuaError(const QString &context)
{
    if (!m_lua) {
        return;
    }

    const char *error = lua_tostring(m_lua, -1);
    m_lastError = QString("%1: %2").arg(context, error ? error : "Unknown error");

    lua_pop(m_lua, 1);

    LOG_ERROR("Lua error -" << m_lastError);
}

int LuaBridge::lua_openFile(lua_State *L)
{
    if (!g_bridge) {
        return 0;
    }

    const char *filePath = luaL_checkstring(L, 1);
    emit g_bridge->fileOpenRequested(QString::fromUtf8(filePath));
    return 0;
}

int LuaBridge::lua_saveFile(lua_State *L)
{
    if (!g_bridge) {
        return 0;
    }

    const char *filePath = nullptr;
    if (lua_gettop(L) >= 1) {
        filePath = luaL_checkstring(L, 1);
    }

    emit g_bridge->fileSaveRequested(filePath ? QString::fromUtf8(filePath) : QString());
    return 0;
}

int LuaBridge::lua_getText(lua_State *L)
{
    if (!g_bridge) {
        DEBUG_LOG_LUA("lua_getText: g_bridge is null");
        lua_pushstring(L, "");
        return 1;
    }

    QString text = g_bridge->getEditorText();
    DEBUG_LOG_LUA("lua_getText called, returning text length:" << text.length());
    lua_pushstring(L, text.toUtf8().constData());
    return 1;
}

int LuaBridge::lua_setText(lua_State *L)
{
    if (!g_bridge) {
        DEBUG_LOG_LUA("lua_setText: g_bridge is null");
        return 0;
    }

    const char *text = luaL_checkstring(L, 1);
    QString textStr = QString::fromUtf8(text);
    DEBUG_LOG_LUA("lua_setText called with text:" << textStr);
    g_bridge->setEditorText(textStr);
    return 0;
}

int LuaBridge::lua_getCursorPosition(lua_State *L)
{
    if (!g_bridge) {
        lua_pushinteger(L, 1);
        lua_pushinteger(L, 1);
        return 2;
    }

    QPair<int, int> pos = g_bridge->getEditorCursorPosition();
    lua_pushinteger(L, pos.first);  
    lua_pushinteger(L, pos.second); 
    return 2;
}

int LuaBridge::lua_setCursorPosition(lua_State *L)
{
    if (!g_bridge) {
        return 0;
    }

    int line = luaL_checkinteger(L, 1);
    int column = luaL_checkinteger(L, 2);
    g_bridge->setEditorCursorPosition(line, column);
    return 0;
}

int LuaBridge::lua_setStatusText(lua_State *L)
{
    if (!g_bridge) {
        return 0;
    }

    const char *message = luaL_checkstring(L, 1);
    emit g_bridge->statusMessageRequested(QString::fromUtf8(message));
    return 0;
}

QString LuaBridge::getConfigString(const QString &key, const QString &defaultValue)
{
    if (!m_lua) {
        return defaultValue;
    }

    lua_getglobal(m_lua, "get_config");
    if (lua_isfunction(m_lua, -1)) {

        lua_pushstring(m_lua, key.toUtf8().constData());
        lua_pushstring(m_lua, defaultValue.toUtf8().constData());

        int result = lua_pcall(m_lua, 2, 1, 0);
        if (result == 0) {
            QString value = defaultValue;
            if (lua_isstring(m_lua, -1)) {
                value = QString::fromUtf8(lua_tostring(m_lua, -1));
            }
            lua_pop(m_lua, 1); 
            return value;
        } else {
            lua_pop(m_lua, 1); 
        }
    } else {
        lua_pop(m_lua, 1); 
    }

    QStringList keys = key.split('.');
    lua_getglobal(m_lua, "config");

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return defaultValue;
    }

    for (const QString &k : keys) {
        lua_pushstring(m_lua, k.toUtf8().constData());
        lua_gettable(m_lua, -2);
        lua_remove(m_lua, -2); 

        if (lua_isnil(m_lua, -1)) {
            lua_pop(m_lua, 1);
            return defaultValue;
        }
    }

    QString value = defaultValue;
    if (lua_isstring(m_lua, -1)) {
        value = QString::fromUtf8(lua_tostring(m_lua, -1));
    }

    lua_pop(m_lua, 1); 
    return value;
}

int LuaBridge::getConfigInt(const QString &key, int defaultValue)
{
    if (!m_lua) {
        return defaultValue;
    }

    lua_getglobal(m_lua, "get_config");
    if (lua_isfunction(m_lua, -1)) {

        lua_pushstring(m_lua, key.toUtf8().constData());
        lua_pushinteger(m_lua, defaultValue);

        int result = lua_pcall(m_lua, 2, 1, 0);
        if (result == 0) {
            int value = defaultValue;
            if (lua_isnumber(m_lua, -1)) {
                value = lua_tointeger(m_lua, -1);
            }
            lua_pop(m_lua, 1); 
            return value;
        } else {
            lua_pop(m_lua, 1); 
        }
    } else {
        lua_pop(m_lua, 1); 
    }

    QStringList keys = key.split('.');
    lua_getglobal(m_lua, "config");

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return defaultValue;
    }

    for (const QString &k : keys) {
        lua_pushstring(m_lua, k.toUtf8().constData());
        lua_gettable(m_lua, -2);
        lua_remove(m_lua, -2); 

        if (lua_isnil(m_lua, -1)) {
            lua_pop(m_lua, 1);
            return defaultValue;
        }
    }

    int value = defaultValue;
    if (lua_isnumber(m_lua, -1)) {
        value = lua_tointeger(m_lua, -1);
    }

    lua_pop(m_lua, 1); 
    return value;
}

bool LuaBridge::getConfigBool(const QString &key, bool defaultValue)
{
    if (!m_lua) {
        return defaultValue;
    }

    lua_getglobal(m_lua, "get_config");
    if (lua_isfunction(m_lua, -1)) {

        lua_pushstring(m_lua, key.toUtf8().constData());
        lua_pushboolean(m_lua, defaultValue);

        int result = lua_pcall(m_lua, 2, 1, 0);
        if (result == 0) {
            bool value = defaultValue;
            if (lua_isboolean(m_lua, -1)) {
                value = lua_toboolean(m_lua, -1);
            } else if (lua_isnumber(m_lua, -1)) {

                value = lua_tonumber(m_lua, -1) != 0;
            }
            lua_pop(m_lua, 1); 
            return value;
        } else {
            lua_pop(m_lua, 1); 
        }
    } else {
        lua_pop(m_lua, 1); 
    }

    QStringList keys = key.split('.');
    lua_getglobal(m_lua, "config");

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return defaultValue;
    }

    for (const QString &k : keys) {
        lua_pushstring(m_lua, k.toUtf8().constData());
        lua_gettable(m_lua, -2);
        lua_remove(m_lua, -2); 

        if (lua_isnil(m_lua, -1)) {
            lua_pop(m_lua, 1);
            return defaultValue;
        }
    }

    bool value = defaultValue;
    if (lua_isboolean(m_lua, -1)) {
        value = lua_toboolean(m_lua, -1);
    } else if (lua_isnumber(m_lua, -1)) {

        value = lua_tonumber(m_lua, -1) != 0;
    }

    lua_pop(m_lua, 1); 
    return value;
}

QMap<QString, QString> LuaBridge::getKeybindings()
{
    QMap<QString, QString> keybindings;

    if (!m_lua) {
        return keybindings;
    }

    lua_getglobal(m_lua, "config");
    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        DEBUG_LOG_LUA("Config table not found for keybindings");
        return keybindings;
    }

    lua_pushstring(m_lua, "keybindings");
    lua_gettable(m_lua, -2);

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 2);
        DEBUG_LOG_LUA("Keybindings table not found in configuration");
        return keybindings;
    }

    lua_pushnil(m_lua);
    int count = 0;
    while (lua_next(m_lua, -2) != 0) {
        count++;

        if (lua_isstring(m_lua, -2) && lua_isstring(m_lua, -1)) {
            QString key = QString::fromUtf8(lua_tostring(m_lua, -2));
            QString action = QString::fromUtf8(lua_tostring(m_lua, -1));
            keybindings[key] = action;
        }
        lua_pop(m_lua, 1);
    }

    lua_pop(m_lua, 2);

    return keybindings;
}

QMap<QString, QString> LuaBridge::getSyntaxColors()
{
    QMap<QString, QString> syntaxColors;

    if (!m_lua) {
        return syntaxColors;
    }

    lua_getglobal(m_lua, "config");
    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        DEBUG_LOG_LUA("Config table not found for syntax colors");
        return syntaxColors;
    }

    lua_pushstring(m_lua, "syntax");
    lua_gettable(m_lua, -2);

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 2);
        DEBUG_LOG_LUA("Syntax colors table not found in configuration");
        return syntaxColors;
    }

    lua_pushnil(m_lua);
    while (lua_next(m_lua, -2) != 0) {
        if (lua_isstring(m_lua, -2) && lua_isstring(m_lua, -1)) {
            QString key = QString::fromUtf8(lua_tostring(m_lua, -2));
            QString color = QString::fromUtf8(lua_tostring(m_lua, -1));
            syntaxColors[key] = color;
        }
        lua_pop(m_lua, 1);
    }

    lua_pop(m_lua, 2);

    return syntaxColors;
}

QMap<QString, QString> LuaBridge::getMarkdownSyntaxColors()
{
    QMap<QString, QString> markdownSyntaxColors;

    if (!m_lua) {
        return markdownSyntaxColors;
    }

    lua_getglobal(m_lua, "config");
    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        DEBUG_LOG_LUA("Config table not found for markdown syntax colors");
        return markdownSyntaxColors;
    }

    lua_pushstring(m_lua, "markdown_syntax");
    lua_gettable(m_lua, -2);

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 2);
        DEBUG_LOG_LUA("Markdown syntax colors table not found in configuration");
        return markdownSyntaxColors;
    }

    lua_pushnil(m_lua);
    while (lua_next(m_lua, -2) != 0) {
        if (lua_isstring(m_lua, -2) && lua_isstring(m_lua, -1)) {
            QString key = QString::fromUtf8(lua_tostring(m_lua, -2));
            QString color = QString::fromUtf8(lua_tostring(m_lua, -1));
            markdownSyntaxColors[key] = color;
        }
        lua_pop(m_lua, 1);
    }

    lua_pop(m_lua, 2);

    return markdownSyntaxColors;
}

QMap<QString, QString> LuaBridge::getBasicHighlighterColors()
{
    QMap<QString, QString> basicHighlighterColors;

    if (!m_lua) {
        return basicHighlighterColors;
    }

    lua_getglobal(m_lua, "config");
    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 1);
        DEBUG_LOG_LUA("Config table not found for basic highlighter colors");
        return basicHighlighterColors;
    }

    lua_pushstring(m_lua, "basic_highlighter");
    lua_gettable(m_lua, -2);

    if (!lua_istable(m_lua, -1)) {
        lua_pop(m_lua, 2);
        DEBUG_LOG_LUA("Basic highlighter colors table not found in configuration");
        return basicHighlighterColors;
    }

    lua_pushnil(m_lua);
    while (lua_next(m_lua, -2) != 0) {
        if (lua_isstring(m_lua, -2) && lua_isstring(m_lua, -1)) {
            QString key = QString::fromUtf8(lua_tostring(m_lua, -2));
            QString color = QString::fromUtf8(lua_tostring(m_lua, -1));
            basicHighlighterColors[key] = color;
        }
        lua_pop(m_lua, 1);
    }

    lua_pop(m_lua, 2);

    return basicHighlighterColors;
}

void LuaBridge::setEditorText(const QString &text)
{
    DEBUG_LOG_LUA("LuaBridge::setEditorText emitting signal with text:" << text);
    emit textChangeRequested(text);
}

QString LuaBridge::getEditorText() const
{
    return m_currentText;
}

void LuaBridge::setEditorCursorPosition(int line, int column)
{
    emit cursorMoveRequested(line, column);
}

QPair<int, int> LuaBridge::getEditorCursorPosition() const
{
    return m_currentCursorPosition;
}

void LuaBridge::updateEditorState(const QString &text, int line, int column)
{
    m_currentText = text;
    m_currentCursorPosition = QPair<int, int>(line, column);
}

bool LuaBridge::executeFile(const QString &filePath)
{
    if (!m_lua) {
        m_lastError = "Lua state not initialized";
        return false;
    }

    if (!QFile::exists(filePath)) {
        m_lastError = QString("File not found: %1").arg(filePath);
        return false;
    }

    int result = luaL_dofile(m_lua, filePath.toUtf8().constData());
    if (result != 0) {
        handleLuaError("Executing Lua file");
        return false;
    }

    DEBUG_LOG_LUA("Lua file executed successfully:" << filePath);
    return true;
}

void LuaBridge::setSyntaxHighlighter(SyntaxHighlighter *highlighter)
{
    m_syntaxHighlighter = highlighter;

}

void LuaBridge::setPluginManager(PluginManager *pluginManager)
{
    m_pluginManager = pluginManager;

}

void LuaBridge::loadSyntaxRulesForLanguage(const QString &language)
{
    if (!m_syntaxHighlighter) {
        DEBUG_LOG_LUA("No syntax highlighter set, cannot load rules for language:" << language);
        return;
    }

}

int LuaBridge::lua_addSyntaxRule(lua_State *L)
{
    if (!g_bridge || !g_bridge->m_syntaxHighlighter) {
        lua_pushstring(L, "No syntax highlighter available");
        lua_error(L);
        return 0;
    }

    const char *pattern = luaL_checkstring(L, 1);
    const char *colorName = luaL_checkstring(L, 2);

    g_bridge->m_syntaxHighlighter->addRule(QString::fromUtf8(pattern), QString::fromUtf8(colorName));

    return 0;
}

int LuaBridge::lua_clearSyntaxRules(lua_State *L)
{
    Q_UNUSED(L)

    if (!g_bridge || !g_bridge->m_syntaxHighlighter) {
        lua_pushstring(L, "No syntax highlighter available");
        lua_error(L);
        return 0;
    }

    g_bridge->m_syntaxHighlighter->clearRules();

    return 0;
}

int LuaBridge::lua_registerEventHandler(lua_State *L)
{

    if (lua_gettop(L) != 2) {
        lua_pushstring(L, "register_event_handler expects 2 arguments: event_name, handler_function");
        lua_error(L);
        return 0;
    }

    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        lua_pushstring(L, "register_event_handler expects string arguments");
        lua_error(L);
        return 0;
    }

    QString eventName = lua_tostring(L, 1);
    QString handlerFunction = lua_tostring(L, 2);

    if (!g_bridge) {
        lua_pushstring(L, "No bridge available");
        lua_error(L);
        return 0;
    }

    g_bridge->registerEventHandler(eventName, handlerFunction);

    return 0;
}

int LuaBridge::lua_createTimer(lua_State *L)
{

    if (lua_gettop(L) < 2) {
        lua_pushstring(L, "create_timer expects at least 2 arguments: interval, callback_function [, repeat]");
        lua_error(L);
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        lua_pushstring(L, "create_timer expects number and string arguments");
        lua_error(L);
        return 0;
    }

    int interval = lua_tointeger(L, 1);
    QString callbackFunction = lua_tostring(L, 2);
    bool repeat = true; 

    if (lua_gettop(L) >= 3 && lua_isboolean(L, 3)) {
        repeat = lua_toboolean(L, 3);
    }

    if (!g_bridge) {
        lua_pushstring(L, "No bridge available");
        lua_error(L);
        return 0;
    }

    QTimer *timer = new QTimer(g_bridge);
    timer->setInterval(interval);
    timer->setSingleShot(!repeat);

    int timerId = g_bridge->m_nextTimerId++;
    g_bridge->m_timers[timerId] = timer;

    QObject::connect(timer, &QTimer::timeout, [callbackFunction]() {
        if (g_bridge && g_bridge->m_lua) {

            lua_getglobal(g_bridge->m_lua, callbackFunction.toUtf8().constData());

            if (lua_isfunction(g_bridge->m_lua, -1)) {
                if (lua_pcall(g_bridge->m_lua, 0, 0, 0) != 0) {
                    QString error = QString("Timer callback error: %1").arg(lua_tostring(g_bridge->m_lua, -1));
                    DEBUG_LOG_LUA(error);
                    lua_pop(g_bridge->m_lua, 1); 
                }
            } else {
                DEBUG_LOG_LUA("Timer callback" << callbackFunction << "is not a function");
                lua_pop(g_bridge->m_lua, 1); 
            }
        }
    });

    timer->start();

    lua_pushinteger(L, timerId);
    return 1;
}

int LuaBridge::lua_stopTimer(lua_State *L)
{

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "stop_timer expects 1 argument: timer_id");
        lua_error(L);
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "stop_timer expects number argument");
        lua_error(L);
        return 0;
    }

    int timerId = lua_tointeger(L, 1);

    if (!g_bridge) {
        lua_pushstring(L, "No bridge available");
        lua_error(L);
        return 0;
    }

    if (g_bridge->m_timers.contains(timerId)) {
        QTimer *timer = g_bridge->m_timers[timerId];
        timer->stop();
        delete timer;
        g_bridge->m_timers.remove(timerId);

        lua_pushboolean(L, true);
    } else {
        DEBUG_LOG_LUA("Timer" << timerId << "not found");
        lua_pushboolean(L, false);
    }

    return 1;
}

int LuaBridge::lua_listPlugins(lua_State *L)
{
    if (!g_bridge || !g_bridge->m_pluginManager) {
        lua_newtable(L);
        return 1;
    }

    QStringList loadedPlugins = g_bridge->m_pluginManager->loadedPlugins();
    QStringList availablePlugins = g_bridge->m_pluginManager->availablePlugins();

    lua_newtable(L);

    lua_newtable(L);
    for (int i = 0; i < loadedPlugins.size(); ++i) {
        lua_pushstring(L, loadedPlugins[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "loaded");

    lua_newtable(L);
    for (int i = 0; i < availablePlugins.size(); ++i) {
        lua_pushstring(L, availablePlugins[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "available");

    return 1;
}

int LuaBridge::lua_isPluginLoaded(lua_State *L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
        lua_pushstring(L, "is_plugin_loaded expects 1 string argument: plugin_name");
        lua_error(L);
        return 0;
    }

    QString pluginName = lua_tostring(L, 1);

    if (!g_bridge || !g_bridge->m_pluginManager) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool isLoaded = g_bridge->m_pluginManager->isPluginLoaded(pluginName);
    lua_pushboolean(L, isLoaded);

    return 1;
}

int LuaBridge::lua_getPluginConfig(lua_State *L)
{
    if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
        lua_pushstring(L, "get_plugin_config expects at least 1 string argument: plugin_name [, key]");
        lua_error(L);
        return 0;
    }

    QString pluginName = lua_tostring(L, 1);
    QString configKey = QString("plugins.%1").arg(pluginName);

    if (lua_gettop(L) >= 2 && lua_isstring(L, 2)) {
        QString subKey = lua_tostring(L, 2);
        configKey += QString(".%1").arg(subKey);
    }

    if (!g_bridge) {
        lua_pushnil(L);
        return 1;
    }

    if (configKey.endsWith(".enabled")) {
        bool enabled = g_bridge->getConfigBool(configKey, true);
        lua_pushboolean(L, enabled);
    } else {

        QString stringValue = g_bridge->getConfigString(configKey, "");
        if (!stringValue.isEmpty()) {
            lua_pushstring(L, stringValue.toUtf8().constData());
        } else {
            int intValue = g_bridge->getConfigInt(configKey, -1);
            if (intValue != -1) {
                lua_pushinteger(L, intValue);
            } else {
                bool boolValue = g_bridge->getConfigBool(configKey, false);
                lua_pushboolean(L, boolValue);
            }
        }
    }

    return 1;
}

int LuaBridge::lua_debugLog(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        return 0;
    }

    const char *message = luaL_checkstring(L, 1);
    if (message) {
#ifdef NDEBUG

        (void)message; 
#else

        DEBUG_LOG_LUA(message);
#endif
    }

    return 0;
}

int LuaBridge::lua_setTheme(lua_State *L)
{
    if (!g_bridge) {
        lua_pushstring(L, "No bridge available");
        lua_error(L);
        return 0;
    }

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "set_theme expects 1 argument: theme_name");
        lua_error(L);
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "set_theme expects string argument");
        lua_error(L);
        return 0;
    }

    QString themeName = lua_tostring(L, 1);
    emit g_bridge->themeChangeRequested(themeName);

    return 0;
}

int LuaBridge::lua_getTheme(lua_State *L)
{
    if (!g_bridge) {
        lua_pushstring(L, "gruvbox"); 
        return 1;
    }

    QString currentTheme = g_bridge->getConfigString("theme.name", "gruvbox");
    lua_pushstring(L, currentTheme.toUtf8().constData());

    return 1;
}

int LuaBridge::lua_toggleTheme(lua_State *L)
{
    if (!g_bridge) {
        return 0;
    }

    bool pluginEnabled = g_bridge->getConfigBool("plugins.theme_switcher.enabled", false);
    bool autoLoad = g_bridge->getConfigBool("plugins.theme_switcher.auto_load", false);

    if (!pluginEnabled || !autoLoad) {

        g_bridge->executeString("editor.set_status_text('Theme switcher plugin is disabled')");
        return 0;
    }

    g_bridge->executeString("toggle_theme()");

    return 0;
}
