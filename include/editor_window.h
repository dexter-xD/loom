// main window class containing all ui elements
// manages central editor widget, status bar, and menu system
// coordinates between ui components and editor core

#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <QApplication>
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
#include <QSplitter>
#include "buffer.h"
#include "lua_bridge.h"
#include "debug_log.h"
#include "tree_sitter_highlighter.h"
#include "markdown_highlighter.h"
#include "basic_highlighter.h"
#include "plugin_manager.h"
#include "code_editor.h"
#include "file_tree_widget.h"

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow();

    void openFile(const QString &filePath);
    void openProject(const QString &projectPath);
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
    void onLuaThemeChangeRequested(const QString &themeName);

    void onFileTreeFileOpenRequested(const QString &filePath);
    void onFileTreeVisibilityChanged(bool visible);

    void executeAction(const QString &action);
    bool isPluginActionEnabled(const QString &pluginName) const;

    void updateLuaEditorState();

private:

    QTabWidget *m_tabWidget;
    QStatusBar *m_statusBar;

    QList<Buffer*> m_buffers;
    QList<CodeEditor*> m_textEditors;
    QList<TreeSitterHighlighter*> m_syntaxHighlighters;
    QList<MarkdownHighlighter*> m_markdownHighlighters;
    QList<BasicHighlighter*> m_basicHighlighters;

    LuaBridge *m_luaBridge;

    PluginManager *m_pluginManager;

    QMap<QString, QShortcut*> m_shortcuts;

    // File tree widget
    QSplitter *m_mainSplitter;
    FileTreeWidget *m_fileTreeWidget;

    void setupUI();
    void setupMenus();
    void refreshToolsMenu();
    void setupStatusBar();
    void connectSignals();

    bool saveCurrentFile();
    void updateWindowTitle();

    int createNewTab(const QString &title = "Untitled");
    void updateTabTitle(int index);
    void updateTabModificationIndicator(int index);
    Buffer* getCurrentBuffer();
    CodeEditor* getCurrentTextEditor();
    TreeSitterHighlighter* getCurrentSyntaxHighlighter();
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
    
    void loadTheme(const QString &themeName);
    void applyTheme();
    void updateEditorThemeColors(const QString &themeName);
};

#endif
