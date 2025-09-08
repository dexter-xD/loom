#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include <QString>
#include <QVariantList>
#include <QObject>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <QTimer>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// forward declaration
class SyntaxHighlighter;
class PluginManager;

// interface between c++ editor and lua scripting engine
// exposes editor functions to lua and handles lua callbacks
// manages lua state and error handling
class LuaBridge : public QObject
{
    Q_OBJECT

public:
    explicit LuaBridge(QObject *parent = nullptr);
    ~LuaBridge();

    bool initialize();

    bool loadConfig(const QString &configPath);

    bool executeFile(const QString &filePath);

    bool executeString(const QString &luaCode);

    void registerEditorAPI();

    void emitEvent(const QString &eventName, const QVariantList &args);

    void registerEventHandler(const QString &eventName, const QString &handlerFunction);

    QString lastError() const;

    QString getConfigString(const QString &key, const QString &defaultValue = QString());
    int getConfigInt(const QString &key, int defaultValue = 0);
    bool getConfigBool(const QString &key, bool defaultValue = false);

    QMap<QString, QString> getKeybindings();

    QMap<QString, QString> getSyntaxColors();

    QMap<QString, QString> getMarkdownSyntaxColors();

    QMap<QString, QString> getBasicHighlighterColors();

    void setEditorText(const QString &text);
    QString getEditorText() const;
    void setEditorCursorPosition(int line, int column);
    QPair<int, int> getEditorCursorPosition() const;

    void updateEditorState(const QString &text, int line, int column);

    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);

    void loadSyntaxRulesForLanguage(const QString &language);

    void setPluginManager(PluginManager *pluginManager);

signals:

    void fileOpenRequested(const QString &filePath);
    void fileSaveRequested(const QString &filePath);
    void textChangeRequested(const QString &content);
    void cursorMoveRequested(int line, int column);
    void statusMessageRequested(const QString &message);
    void themeChangeRequested(const QString &themeName);

private:
    lua_State *m_lua;
    QString m_lastError;

    QString m_currentText;
    QPair<int, int> m_currentCursorPosition;

    SyntaxHighlighter *m_syntaxHighlighter;

    PluginManager *m_pluginManager;

    QMap<QString, QStringList> m_eventHandlers;

    QMap<int, QTimer*> m_timers;
    int m_nextTimerId;

    void setupLuaPath();

    void registerFunction(const QString &name, lua_CFunction func);

    void handleLuaError(const QString &context);

    static int lua_openFile(lua_State *L);
    static int lua_saveFile(lua_State *L);
    static int lua_getText(lua_State *L);
    static int lua_setText(lua_State *L);
    static int lua_getCursorPosition(lua_State *L);
    static int lua_setCursorPosition(lua_State *L);
    static int lua_setStatusText(lua_State *L);

    static int lua_addSyntaxRule(lua_State *L);
    static int lua_clearSyntaxRules(lua_State *L);

    static int lua_registerEventHandler(lua_State *L);

    static int lua_createTimer(lua_State *L);
    static int lua_stopTimer(lua_State *L);
    
    static int lua_debugLog(lua_State *L);

    static int lua_listPlugins(lua_State *L);
    static int lua_isPluginLoaded(lua_State *L);
    static int lua_getPluginConfig(lua_State *L);
    
    static int lua_setTheme(lua_State *L);
    static int lua_getTheme(lua_State *L);
    static int lua_toggleTheme(lua_State *L);
};

#endif
