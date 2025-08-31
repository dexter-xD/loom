// main window class containing all ui elements
// manages central editor widget, status bar, and menu system
// coordinates between ui components and editor core

#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextCursor>
#include <QFont>
#include <QKeySequence>
#include <QShortcut>
#include <QMap>
#include <QIcon>
#include <QList>
#include <QCloseEvent>
#include <QInputDialog>
#include <QTextDocument>
#include <QPushButton>
#include <QTimer>
#include <QKeyEvent>
#include "buffer.h"
#include "lua_bridge.h"
#include "syntax_highlighter.h"
#include "plugin_manager.h"
#include "code_editor.h"

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow();

    void openFile(const QString &filePath);
    void saveFile();
    void saveFileAs();
    void newFile();
    void closeFile(int index);
    void closeCurrentFile();

    void updateStatusBar(int line, int column, bool modified);
    void updateStatusBar(); 

    void loadConfiguration();

    void loadPlugins();

    void ensureAtLeastOneTab();
    void applyConfiguration();
    void setupKeybindings();

protected:

    void closeEvent(QCloseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private slots:

    void onTextChanged();
    void onCursorPositionChanged();

    void onTabChanged(int index);
    void onTabCloseRequested(int index);

    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveFileAs();
    void onCloseFile();
    void onExit();

    void onLuaFileOpenRequested(const QString &filePath);
    void onLuaFileSaveRequested(const QString &filePath);
    void onLuaTextChangeRequested(const QString &content);
    void onLuaCursorMoveRequested(int line, int column);
    void onLuaStatusMessageRequested(const QString &message);

    void executeAction(const QString &action);

    void updateLuaEditorState();

private:

    QTabWidget *m_tabWidget;
    QStatusBar *m_statusBar;

    QList<Buffer*> m_buffers;
    QList<CodeEditor*> m_textEditors;
    QList<SyntaxHighlighter*> m_syntaxHighlighters;

    LuaBridge *m_luaBridge;

    PluginManager *m_pluginManager;

    QMap<QString, QShortcut*> m_shortcuts;

    void setupUI();
    void setupMenus();
    void setupStatusBar();
    void connectSignals();

    bool saveCurrentFile();
    void updateWindowTitle();

    int createNewTab(const QString &title = "Untitled");
    void updateTabTitle(int index);
    void updateTabModificationIndicator(int index);
    Buffer* getCurrentBuffer();
    CodeEditor* getCurrentTextEditor();
    SyntaxHighlighter* getCurrentSyntaxHighlighter();
    int getCurrentTabIndex();

    void setupSyntaxHighlighting();
    void setupSyntaxHighlightingForTab(int index);
    void detectAndSetLanguage(const QString &filePath);
    QString detectLanguageFromExtension(const QString &filePath);

    void showFindDialog();
    void showReplaceDialog();
    void findText(const QString &searchText, bool caseSensitive = false);
    void replaceText(const QString &searchText, const QString &replaceText, bool replaceAll = false);

    void setCurrentLanguage(const QString &language);
};

#endif 