// main window class containing all ui elements
// manages central editor widget, status bar, and menu system
// coordinates between ui components and editor core

#include "editor_window.h"
#include <QApplication>
#include <QTextStream>
#include <QFileInfo>
#include <QFont>
#include "debug_log.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QInputDialog>
#include <QPainter>
#include <QBuffer>
#include <QPixmap>
#include <QIcon>
#include <QTabBar>
#include <QToolButton>
#include "tree_sitter_highlighter.h"
#include "markdown_highlighter.h"
#include "basic_highlighter.h"

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_statusBar(nullptr)
    , m_luaBridge(nullptr)
{

    m_luaBridge = new LuaBridge(this);
    if (!m_luaBridge->initialize()) {
        LOG_ERROR("Failed to initialize Lua bridge:" << m_luaBridge->lastError());
    }

    m_pluginManager = new PluginManager(m_luaBridge, this);

    m_luaBridge->setPluginManager(m_pluginManager);

    connect(m_pluginManager, &PluginManager::pluginLoaded,
            this, [this](const QString &pluginName) {
                m_statusBar->showMessage(QString("Plugin loaded: %1").arg(pluginName), 3000);
            });
    connect(m_pluginManager, &PluginManager::pluginError,
            this, [this](const QString &pluginName, const QString &error) {
                m_statusBar->showMessage(QString("Plugin error (%1): %2").arg(pluginName, error), 5000);
            });

    setupUI();
    setupStatusBar();
    connectSignals();

    loadConfiguration();

    applyConfiguration();
    setupKeybindings();

    setupSyntaxHighlighting();

    loadPlugins();

    setupMenus();

    updateLuaEditorState();

    setWindowTitle("Loom");

    setMinimumSize(400, 300);
}

EditorWindow::~EditorWindow()
{

    for (auto shortcut : m_shortcuts) {
        delete shortcut;
    }
    m_shortcuts.clear();

    for (Buffer* buffer : m_buffers) {
        delete buffer;
    }
    m_buffers.clear();

    m_textEditors.clear();
    m_syntaxHighlighters.clear();
    m_markdownHighlighters.clear();
    m_basicHighlighters.clear();

    delete m_pluginManager;
    delete m_luaBridge;
}

void EditorWindow::setupUI()
{

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    m_fileTreeWidget = new FileTreeWidget(this);
    m_fileTreeWidget->setVisible(false); 

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabBarAutoHide(false);

    m_mainSplitter->addWidget(m_fileTreeWidget);
    m_mainSplitter->addWidget(m_tabWidget);

    m_mainSplitter->setStretchFactor(0, 0); 
    m_mainSplitter->setStretchFactor(1, 1); 
    m_mainSplitter->setSizes({200, 800}); 

    QColor lineNumberBg = QColor(40, 37, 34).darker(110);  
    QString handleStyle = QString(
        "QSplitter::handle {"
        "    background-color: %1;"  
        "    border: none;"
        "    margin: 0px;"
        "    padding: 0px;"
        "    width: 1px;"  
        "}"
        "QSplitter::handle:hover {"
        "    background-color: %2;"  
        "}"
    ).arg(lineNumberBg.name(), lineNumberBg.lighter(120).name());

    m_mainSplitter->setStyleSheet(handleStyle);
    m_mainSplitter->setHandleWidth(1);  
}

void EditorWindow::setupMenus()
{

    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *newAction = new QAction("&New", this);
    newAction->setStatusTip("Create a new file");
    connect(newAction, &QAction::triggered, this, &EditorWindow::onNewFile);
    fileMenu->addAction(newAction);

    QAction *openAction = new QAction("&Open", this);
    openAction->setStatusTip("Open an existing file");
    connect(openAction, &QAction::triggered, this, &EditorWindow::onOpenFile);
    fileMenu->addAction(openAction);

    fileMenu->addSeparator();

    QAction *saveAction = new QAction("&Save", this);
    saveAction->setStatusTip("Save the current file");
    connect(saveAction, &QAction::triggered, this, &EditorWindow::onSaveFile);
    fileMenu->addAction(saveAction);

    QAction *saveAsAction = new QAction("Save &As...", this);
    saveAsAction->setStatusTip("Save the file with a new name");
    connect(saveAsAction, &QAction::triggered, this, &EditorWindow::onSaveFileAs);
    fileMenu->addAction(saveAsAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &EditorWindow::onExit);
    fileMenu->addAction(exitAction);

    QAction *closeAction = new QAction("&Close", this);
    closeAction->setStatusTip("Close the current file");
    connect(closeAction, &QAction::triggered, this, &EditorWindow::onCloseFile);
    fileMenu->addAction(closeAction);

    fileMenu->addSeparator();

    QMenu *editMenu = menuBar()->addMenu("&Edit");

    QAction *undoAction = new QAction("&Undo", this);
    undoAction->setStatusTip("Undo the last action");
    connect(undoAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->undo();
    });
    editMenu->addAction(undoAction);

    QAction *redoAction = new QAction("&Redo", this);
    redoAction->setStatusTip("Redo the last undone action");
    connect(redoAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->redo();
    });
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    QAction *cutAction = new QAction("Cu&t", this);
    cutAction->setStatusTip("Cut the selected text");
    connect(cutAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->cut();
    });
    editMenu->addAction(cutAction);

    QAction *copyAction = new QAction("&Copy", this);
    copyAction->setStatusTip("Copy the selected text");
    connect(copyAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->copy();
    });
    editMenu->addAction(copyAction);

    QAction *pasteAction = new QAction("&Paste", this);
    pasteAction->setStatusTip("Paste text from clipboard");
    connect(pasteAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->paste();
    });
    editMenu->addAction(pasteAction);

    editMenu->addSeparator();

    QAction *selectAllAction = new QAction("Select &All", this);
    selectAllAction->setStatusTip("Select all text");
    connect(selectAllAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->selectAll();
    });
    editMenu->addAction(selectAllAction);

    editMenu->addSeparator();

    QAction *findAction = new QAction("&Find", this);
    findAction->setStatusTip("Find text in the document");
    connect(findAction, &QAction::triggered, this, &EditorWindow::showFindDialog);
    editMenu->addAction(findAction);

    QAction *replaceAction = new QAction("&Replace", this);
    replaceAction->setStatusTip("Find and replace text in the document");
    connect(replaceAction, &QAction::triggered, this, &EditorWindow::showReplaceDialog);
    editMenu->addAction(replaceAction);

    QMenu *viewMenu = menuBar()->addMenu("&View");

    QAction *fullscreenAction = new QAction("Toggle &Fullscreen", this);
    fullscreenAction->setStatusTip("Toggle fullscreen mode");
    connect(fullscreenAction, &QAction::triggered, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    viewMenu->addAction(fullscreenAction);

    viewMenu->addSeparator();

    QAction *relativeLineNumbersAction = new QAction("Toggle &Relative Line Numbers", this);
    relativeLineNumbersAction->setStatusTip("Toggle relative line numbers display");
    relativeLineNumbersAction->setCheckable(true);
    connect(relativeLineNumbersAction, &QAction::triggered, [this](bool checked) {
        for (CodeEditor* textEdit : m_textEditors) {
            textEdit->setRelativeLineNumbers(checked);
        }
        m_statusBar->showMessage(checked ? "Relative line numbers enabled" : "Absolute line numbers enabled", 2000);
    });
    viewMenu->addAction(relativeLineNumbersAction);

    viewMenu->addSeparator();

    QAction *toggleFileTreeAction = new QAction("Toggle &File Tree", this);
    toggleFileTreeAction->setStatusTip("Toggle file tree visibility (F12)");
    connect(toggleFileTreeAction, &QAction::triggered, [this]() {
        if (m_fileTreeWidget) {
            bool wasVisible = m_fileTreeWidget->isVisible();
            m_fileTreeWidget->toggleVisibility();
            bool nowVisible = m_fileTreeWidget->isVisible();

            if (wasVisible != nowVisible) {
                m_statusBar->showMessage(nowVisible ? "File tree shown" : "File tree hidden", 2000);
            } else if (!m_fileTreeWidget->rootPath().isEmpty()) {
                m_statusBar->showMessage("File tree toggled", 2000);
            } else {
                m_statusBar->showMessage("No project open - open a project folder first", 3000);
            }
        }
    });
    viewMenu->addAction(toggleFileTreeAction);

    viewMenu->addSeparator();

    QMenu *languageMenu = viewMenu->addMenu("Syntax &Language");

    QStringList languages = {"text", "cpp", "python", "javascript", "lua", "java", "json", "xml", "html", "css", "markdown"};
    for (const QString &lang : languages) {
        QAction *langAction = new QAction(lang.toUpper(), this);
        langAction->setStatusTip(QString("Set syntax highlighting to %1").arg(lang));
        connect(langAction, &QAction::triggered, [this, lang]() {
            setCurrentLanguage(lang);
        });
        languageMenu->addAction(langAction);
    }

    menuBar()->addMenu("&Tools");
    refreshToolsMenu();
}

void EditorWindow::refreshToolsMenu()
{

    QMenu *toolsMenu = nullptr;
    for (QAction *action : menuBar()->actions()) {
        if (action->text() == "&Tools") {
            toolsMenu = action->menu();
            break;
        }
    }

    if (!toolsMenu) {
        return;
    }

    toolsMenu->clear();

    bool hasPluginMenuItems = false;

    if (isPluginActionEnabled("autoformat")) {
        QAction *formatAction = new QAction("&Format Document", this);
        formatAction->setStatusTip("Format the current document using auto-formatter");
        connect(formatAction, &QAction::triggered, [this]() {
            if (m_luaBridge) {
                QString formatScript = "if autoformat then autoformat.format_document() end";
                if (m_luaBridge->executeString(formatScript)) {
                    m_statusBar->showMessage("Document formatted", 2000);
                } else {
                    m_statusBar->showMessage("Format failed: Auto-formatter error", 3000);
                }
            }
        });
        toolsMenu->addAction(formatAction);
        hasPluginMenuItems = true;
    }

    if (isPluginActionEnabled("theme_switcher")) {
        QAction *toggleThemeAction = new QAction("&Toggle Theme", this);
        toggleThemeAction->setStatusTip("Switch to the next available theme");
        connect(toggleThemeAction, &QAction::triggered, [this]() {
            if (m_luaBridge) {
                m_luaBridge->executeString("toggle_theme()");
            }
        });
        toolsMenu->addAction(toggleThemeAction);
        hasPluginMenuItems = true;
    }

    if (hasPluginMenuItems) {
        toolsMenu->addSeparator();
    }

    QAction *reloadPluginsAction = new QAction("&Reload Plugins", this);
    reloadPluginsAction->setStatusTip("Reload all plugins from the plugins directory");
    connect(reloadPluginsAction, &QAction::triggered, [this]() {
        if (m_pluginManager) {
            m_pluginManager->reloadPlugins();
            QStringList loadedPlugins = m_pluginManager->loadedPlugins();
            m_statusBar->showMessage(QString("Reloaded %1 plugin(s)").arg(loadedPlugins.size()), 3000);

            refreshToolsMenu();
        }
    });
    toolsMenu->addAction(reloadPluginsAction);

    QAction *listPluginsAction = new QAction("List &Plugins", this);
    listPluginsAction->setStatusTip("Show information about loaded plugins");
    connect(listPluginsAction, &QAction::triggered, [this]() {
        if (m_pluginManager) {
            QStringList loadedPlugins = m_pluginManager->loadedPlugins();
            QStringList availablePlugins = m_pluginManager->availablePlugins();
            QStringList errors = m_pluginManager->getPluginErrors();

            QString message = QString("Loaded: %1/%2 plugins").arg(loadedPlugins.size()).arg(availablePlugins.size());
            if (!errors.isEmpty()) {
                message += QString(" (%1 errors)").arg(errors.size());
            }

            m_statusBar->showMessage(message, 5000);

            DEBUG_LOG_EDITOR("=== PLUGIN STATUS ===");
            DEBUG_LOG_EDITOR("Available plugins:" << availablePlugins);
            DEBUG_LOG_EDITOR("Loaded plugins:" << loadedPlugins);
            if (!errors.isEmpty()) {
                DEBUG_LOG_EDITOR("Plugin errors:" << errors);
            }
        }
    });
    toolsMenu->addAction(listPluginsAction);
}

void EditorWindow::setupStatusBar()
{

    m_statusBar = statusBar();
    m_statusBar->showMessage("Ready");
}

void EditorWindow::connectSignals()
{

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &EditorWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &EditorWindow::onTabCloseRequested);

    if (m_fileTreeWidget) {
        connect(m_fileTreeWidget, &FileTreeWidget::fileOpenRequested,
                this, &EditorWindow::onFileTreeFileOpenRequested);
        connect(m_fileTreeWidget, &FileTreeWidget::visibilityChanged,
                this, &EditorWindow::onFileTreeVisibilityChanged);
    }

    if (m_luaBridge) {
        connect(m_luaBridge, &LuaBridge::fileOpenRequested,
                this, &EditorWindow::onLuaFileOpenRequested);
        connect(m_luaBridge, &LuaBridge::fileSaveRequested,
                this, &EditorWindow::onLuaFileSaveRequested);
        connect(m_luaBridge, &LuaBridge::textChangeRequested,
                this, &EditorWindow::onLuaTextChangeRequested);
        connect(m_luaBridge, &LuaBridge::cursorMoveRequested,
                this, &EditorWindow::onLuaCursorMoveRequested);
        connect(m_luaBridge, &LuaBridge::statusMessageRequested,
                this, &EditorWindow::onLuaStatusMessageRequested);
        connect(m_luaBridge, &LuaBridge::themeChangeRequested,
                this, &EditorWindow::onLuaThemeChangeRequested);
    }
}

void EditorWindow::openProject(const QString &projectPath)
{
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        QMessageBox::warning(this, "Project Not Found",
                           QString("Project directory '%1' does not exist.").arg(projectPath));
        return;
    }

    if (m_fileTreeWidget) {
        m_fileTreeWidget->setRootPath(projectPath);
        m_fileTreeWidget->setVisible(true); 

        QString projectName = projectDir.dirName();
        setWindowTitle(QString("Loom - %1").arg(projectName));

        m_statusBar->showMessage(QString("Opened project: %1").arg(projectName), 3000);
        DEBUG_LOG_EDITOR("Opened project:" << projectPath);
    }
}

void EditorWindow::openFile(const QString &filePath)
{

    for (int i = 0; i < m_buffers.size(); ++i) {
        if (m_buffers[i]->filePath() == filePath) {

            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    int tabIndex = -1;
    if (m_tabWidget->count() == 1 && m_buffers.size() == 1) {
        Buffer* existingBuffer = m_buffers[0];
        if (existingBuffer->filePath().isEmpty() && existingBuffer->content().isEmpty() && !existingBuffer->isModified()) {

            tabIndex = 0;
        }
    }

    if (tabIndex == -1) {
        tabIndex = createNewTab();
    }
    Buffer* buffer = m_buffers[tabIndex];
    DEBUG_LOG_EDITOR("Created tab" << tabIndex << "buffer content before load:" << buffer->content().length());

    DEBUG_LOG_EDITOR("About to call buffer->load() for:" << filePath);
    if (buffer->load(filePath)) {

        m_tabWidget->setCurrentIndex(tabIndex);

        CodeEditor* textEdit = m_textEditors[tabIndex];

        disconnect(textEdit, &CodeEditor::textChanged, this, &EditorWindow::onTextChanged);

        QString content = buffer->content();
        textEdit->setPlainText(content);

        detectAndSetLanguage(filePath);

        connect(textEdit, &CodeEditor::textChanged, this, &EditorWindow::onTextChanged);

        buffer->setModified(false);

        updateTabTitle(tabIndex);

        updateWindowTitle();
        updateStatusBar();

        if (m_luaBridge) {
            QVariantList args;
            args << filePath;
            m_luaBridge->emitEvent("file_opened", args);
        }
    }
}

void EditorWindow::onTextChanged()
{
    Buffer* buffer = getCurrentBuffer();
    CodeEditor* textEdit = getCurrentTextEditor();

    if (!buffer || !textEdit) {
        return;
    }

    buffer->setContent(textEdit->toPlainText());

    int currentIndex = getCurrentTabIndex();
    if (currentIndex >= 0) {
        updateTabModificationIndicator(currentIndex);
    }

    updateWindowTitle();
    updateStatusBar();
    updateLuaEditorState();

    if (m_luaBridge) {
        QVariantList args;
        args << buffer->content();
        args << buffer->filePath();
        m_luaBridge->emitEvent("text_changed", args);
    }
}

void EditorWindow::onCursorPositionChanged()
{
    updateStatusBar();
    updateLuaEditorState();

    if (m_luaBridge) {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) {
            QTextCursor cursor = textEdit->textCursor();
            int line = cursor.blockNumber() + 1; 
            int column = cursor.columnNumber() + 1; 

            QVariantList args;
            args << line;
            args << column;
            m_luaBridge->emitEvent("cursor_moved", args);
        }
    }
}

void EditorWindow::updateStatusBar(int line, int column, bool )
{
    CodeEditor* textEdit = getCurrentTextEditor();
    Buffer* buffer = getCurrentBuffer();

    if (!textEdit || !buffer) {
        m_statusBar->showMessage("No file open");
        return;
    }

    if (line == -1 || column == -1) {
        QTextCursor cursor = textEdit->textCursor();
        line = cursor.blockNumber() + 1; 
        column = cursor.columnNumber() + 1; 
    }

    QString status = QString("Line: %1, Column: %2").arg(line).arg(column);

    if (buffer->isModified()) {
        status += " [Modified]";
    }

    if (!buffer->filePath().isEmpty()) {
        status += QString(" - %1").arg(buffer->fileName());
    }

    int currentTab = getCurrentTabIndex();
    int totalTabs = m_tabWidget->count();
    if (totalTabs > 1) {
        status += QString(" (Tab %1 of %2)").arg(currentTab + 1).arg(totalTabs);
    }

    m_statusBar->showMessage(status);
}

void EditorWindow::updateStatusBar()
{
    updateStatusBar(-1, -1, false); 
}

void EditorWindow::onNewFile()
{
    newFile();
}

void EditorWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        "Open File", QString(), 
        "All Files (*);;"
        "Text Files (*.txt);;"
        "C/C++ Files (*.c *.cpp *.cxx *.cc *.h *.hpp *.hxx);;"
        "Python Files (*.py *.pyw);;"
        "JavaScript Files (*.js *.jsx *.ts *.tsx);;"
        "Lua Files (*.lua);;"
        "Java Files (*.java);;"
        "Markdown Files (*.md *.markdown);;"
        "JSON Files (*.json);;"
        "XML Files (*.xml);;"
        "HTML Files (*.html *.htm);;"
        "CSS Files (*.css *.scss *.sass);;"
        "Configuration Files (*.conf *.config *.ini *.cfg)");

    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
}

void EditorWindow::onSaveFile()
{
    saveFile();
}

void EditorWindow::onSaveFileAs()
{
    saveFileAs();
}

void EditorWindow::onCloseFile()
{
    closeCurrentFile();
}

void EditorWindow::onExit()
{
    close();
}

void EditorWindow::newFile()
{
    int tabIndex = createNewTab();
    m_tabWidget->setCurrentIndex(tabIndex);
    updateWindowTitle();
    updateStatusBar();
}

void EditorWindow::ensureAtLeastOneTab()
{
    if (m_tabWidget->count() == 0) {
        createNewTab();
    }
}

void EditorWindow::saveFile()
{
    Buffer* buffer = getCurrentBuffer();
    if (!buffer) {
        return;
    }

    if (m_luaBridge) {
        QString formatScript = R"(
            if autoformat and autoformat.enabled and autoformat.format_on_save then
                autoformat.format_document()
            end
        )";
        m_luaBridge->executeString(formatScript);
    }

    if (buffer->filePath().isEmpty()) {
        saveFileAs();
    } else {
        saveCurrentFile();
    }
}

void EditorWindow::saveFileAs()
{
    Buffer* buffer = getCurrentBuffer();
    if (!buffer) {
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this,
        "Save File", QString(), 
        "All Files (*);;"
        "Text Files (*.txt);;"
        "C/C++ Files (*.c *.cpp *.cxx *.cc *.h *.hpp *.hxx);;"
        "Python Files (*.py *.pyw);;"
        "JavaScript Files (*.js *.jsx *.ts *.tsx);;"
        "Lua Files (*.lua);;"
        "Java Files (*.java);;"
        "Markdown Files (*.md *.markdown);;"
        "JSON Files (*.json);;"
        "XML Files (*.xml);;"
        "HTML Files (*.html *.htm);;"
        "CSS Files (*.css *.scss *.sass);;"
        "Configuration Files (*.conf *.config *.ini *.cfg)");

    if (!filePath.isEmpty()) {

        if (m_luaBridge) {
            QString formatScript = R"(
                if autoformat and autoformat.enabled and autoformat.format_on_save then
                    autoformat.format_document()
                end
            )";
            m_luaBridge->executeString(formatScript);
        }

        if (buffer->save(filePath)) {
            int currentIndex = getCurrentTabIndex();
            if (currentIndex >= 0) {
                updateTabTitle(currentIndex);
                updateTabModificationIndicator(currentIndex);
            }
            updateWindowTitle();
            updateStatusBar();

            if (m_luaBridge) {
                QVariantList args;
                args << filePath;
                m_luaBridge->emitEvent("file_saved", args);
            }
        }
    }
}

bool EditorWindow::saveCurrentFile()
{
    Buffer* buffer = getCurrentBuffer();
    if (!buffer) {
        return false;
    }

    if (buffer->save()) {
        int currentIndex = getCurrentTabIndex();
        if (currentIndex >= 0) {
            updateTabTitle(currentIndex);
            updateTabModificationIndicator(currentIndex);
        }
        updateWindowTitle();
        updateStatusBar();

        if (m_luaBridge) {
            QVariantList args;
            args << buffer->filePath();
            m_luaBridge->emitEvent("file_saved", args);
        }

        return true;
    }
    return false;
}

void EditorWindow::updateWindowTitle()
{
    QString title = "Loom";

    Buffer* buffer = getCurrentBuffer();
    if (buffer && !buffer->filePath().isEmpty()) {
        title += QString(" - %1").arg(buffer->fileName());
    }

    if (buffer && buffer->isModified()) {
        title += " *";
    }

    setWindowTitle(title);
}

void EditorWindow::loadConfiguration()
{
    if (!m_luaBridge) {
        LOG_ERROR("loadConfiguration: luaBridge is null");
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString configPath = appDir + "/config/config.lua";

    if (!QFile::exists(configPath)) {
        configPath = "config/config.lua";
    }

    if (!QFile::exists(configPath)) {
        configPath = "/usr/share/loom/config/config.lua";
    }

    if (QFile::exists(configPath)) {
        DEBUG_LOG_EDITOR("Loading configuration from:" << configPath);
        if (m_luaBridge->loadConfig(configPath)) {
            DEBUG_LOG_EDITOR("Configuration loaded successfully from:" << configPath);
        } else {
            LOG_ERROR("Failed to load configuration:" << m_luaBridge->lastError());
        }
    } else {
        DEBUG_LOG_EDITOR("No configuration file found at any location, using defaults");
    }

}

void EditorWindow::loadPlugins()
{
    if (!m_pluginManager) {
        LOG_ERROR("loadPlugins: pluginManager is null");
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString pluginDir = appDir + "/plugins";

    if (!QDir(pluginDir).exists()) {
        pluginDir = "plugins";
    }

    if (!QDir(pluginDir).exists()) {
        pluginDir = "/usr/share/loom/plugins";
    }

    if (QDir(pluginDir).exists()) {
        DEBUG_LOG_EDITOR("Loading plugins from directory:" << pluginDir);
        if (m_pluginManager->loadPlugins(pluginDir)) {
            QStringList loadedPlugins = m_pluginManager->loadedPlugins();
            LOG_INFO("Plugins loaded successfully:" << loadedPlugins);

            if (!loadedPlugins.isEmpty()) {
                m_statusBar->showMessage(QString("Loaded %1 plugin(s)").arg(loadedPlugins.size()), 3000);
            }
        } else {
            LOG_ERROR("Failed to load plugins:" << m_pluginManager->lastError());
        }
    } else {
        DEBUG_LOG_EDITOR("No plugin directory found, continuing without plugins");
    }

}

void EditorWindow::applyConfiguration()
{
    if (!m_luaBridge) {
        LOG_ERROR("applyConfiguration: missing luaBridge");
        return;
    }

    QString fontFamily = m_luaBridge->getConfigString("editor.font_family", "JetBrains Mono");
    int fontSize = m_luaBridge->getConfigInt("editor.font_size", 12);

    QFont font(fontFamily, fontSize);
    font.setStyleHint(QFont::Monospace);

    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setFont(font);
        textEdit->update();
        textEdit->repaint();
    }

    int tabWidth = m_luaBridge->getConfigInt("editor.tab_width", 4);

    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setTabStopDistance(tabWidth * 10);
    }

    bool wordWrap = m_luaBridge->getConfigBool("editor.word_wrap", false);
    QPlainTextEdit::LineWrapMode wrapMode = wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap;

    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setLineWrapMode(wrapMode);
    }

    bool showLineNumbers = m_luaBridge->getConfigBool("editor.show_line_numbers", true);
    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setLineNumbersVisible(showLineNumbers);
    }

    bool autoIndent = m_luaBridge->getConfigBool("editor.auto_indent", true);
    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setAutoIndentEnabled(autoIndent);
    }

    bool highlightCurrentLine = m_luaBridge->getConfigBool("editor.highlight_current_line", true);
    for (CodeEditor* textEdit : m_textEditors) {
        textEdit->setCurrentLineHighlightEnabled(highlightCurrentLine);
    }

    int windowWidth = m_luaBridge->getConfigInt("window.width", 1024);
    int windowHeight = m_luaBridge->getConfigInt("window.height", 768);

    resize(windowWidth, windowHeight);

    applyTheme();

}

void EditorWindow::onLuaFileOpenRequested(const QString &filePath)
{
    openFile(filePath);
}

void EditorWindow::onLuaFileSaveRequested(const QString &filePath)
{
    if (filePath.isEmpty()) {
        saveFile();
    } else {
        Buffer* buffer = getCurrentBuffer();
        if (buffer && buffer->save(filePath)) {
            int currentIndex = getCurrentTabIndex();
            if (currentIndex >= 0) {
                updateTabTitle(currentIndex);
                updateTabModificationIndicator(currentIndex);
            }
            updateWindowTitle();
            updateStatusBar();

            if (m_luaBridge) {
                QVariantList args;
                args << filePath;
                m_luaBridge->emitEvent("file_saved", args);
            }
        }
    }
}

void EditorWindow::onLuaTextChangeRequested(const QString &content)
{
    DEBUG_LOG_EDITOR("EditorWindow::onLuaTextChangeRequested called with content:" << content);

    CodeEditor* textEdit = getCurrentTextEditor();
    Buffer* buffer = getCurrentBuffer();
    if (!textEdit || !buffer) {
        DEBUG_LOG_EDITOR("No current text editor or buffer available");
        return;
    }

    disconnect(textEdit, &CodeEditor::textChanged, this, &EditorWindow::onTextChanged);

    textEdit->setPlainText(content);

    buffer->setContent(content);

    connect(textEdit, &CodeEditor::textChanged, this, &EditorWindow::onTextChanged);

    int currentIndex = getCurrentTabIndex();
    if (currentIndex >= 0) {
        updateTabModificationIndicator(currentIndex);
    }
    updateWindowTitle();
    updateStatusBar();

    textEdit->update();
    QString actualText = textEdit->toPlainText();
    DEBUG_LOG_EDITOR("Text actually set in editor. Length:" << actualText.length());
    DEBUG_LOG_EDITOR("Text matches requested:" << (actualText == content));
}

void EditorWindow::onLuaCursorMoveRequested(int line, int column)
{
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        DEBUG_LOG_EDITOR("No current text editor available");
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);

    for (int i = 1; i < line && cursor.movePosition(QTextCursor::Down); ++i) {

    }

    for (int i = 1; i < column && cursor.movePosition(QTextCursor::Right); ++i) {

    }

    textEdit->setTextCursor(cursor);
}

void EditorWindow::onLuaStatusMessageRequested(const QString &message)
{
    m_statusBar->showMessage(message);
}

void EditorWindow::setupKeybindings()
{
    if (!m_luaBridge) {
        DEBUG_LOG_EDITOR("setupKeybindings: m_luaBridge is null");
        return;
    }

    for (auto shortcut : m_shortcuts) {
        delete shortcut;
    }
    m_shortcuts.clear();

    QMap<QString, QString> keybindings = m_luaBridge->getKeybindings();

    DEBUG_LOG_EDITOR("Loading keybindings from config:");
    for (auto it = keybindings.begin(); it != keybindings.end(); ++it) {
        const QString &keySequence = it.key();
        const QString &action = it.value();

        DEBUG_LOG_EDITOR("Found keybinding:" << keySequence << "->" << action);

        QShortcut *shortcut = new QShortcut(QKeySequence(keySequence), this);
        shortcut->setContext(Qt::ApplicationShortcut); 

        connect(shortcut, &QShortcut::activated, [this, action, keySequence]() {
            DEBUG_LOG_EDITOR("Shortcut activated:" << keySequence << "->" << action);
            executeAction(action);
        });

        m_shortcuts[keySequence] = shortcut;

        DEBUG_LOG_EDITOR("✓ Registered keybinding:" << keySequence << "->" << action);
    }

    if (keybindings.contains("F12")) {
        DEBUG_LOG_EDITOR("✓ toggle_file_tree shortcut found in keybindings");
    } else {
        DEBUG_LOG_EDITOR("✗ toggle_file_tree shortcut NOT found in keybindings");
    }

}

bool EditorWindow::isPluginActionEnabled(const QString &pluginName) const
{
    if (!m_luaBridge) {
        return false;
    }

    bool pluginEnabled = m_luaBridge->getConfigBool(QString("plugins.%1.enabled").arg(pluginName), false);
    bool autoLoad = m_luaBridge->getConfigBool(QString("plugins.%1.auto_load").arg(pluginName), false);

    return pluginEnabled && autoLoad;
}

void EditorWindow::executeAction(const QString &action)
{

    m_statusBar->showMessage(QString("Action: %1").arg(action), 1000);

    if (action == "save_file") {
        saveFile();
    } else if (action == "open_file") {
        onOpenFile();
    } else if (action == "new_file") {
        newFile();
    } else if (action == "close_file") {
        closeCurrentFile();
    } else if (action == "quit_application") {
        close();
    } else if (action == "undo") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->undo();
    } else if (action == "redo") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->redo();
    } else if (action == "copy") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->copy();
    } else if (action == "paste") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->paste();
    } else if (action == "cut") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->cut();
    } else if (action == "select_all") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->selectAll();
    } else if (action == "new_tab") {
        newFile();
    } else if (action == "find") {
        showFindDialog();
    } else if (action == "replace") {
        showReplaceDialog();
    } else if (action == "toggle_fullscreen") {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    } else if (action == "set_language") {

        m_statusBar->showMessage("Language dialog feature is currently disabled", 2000);
    } else if (action == "redetect_language") {

        m_statusBar->showMessage("Language redetection feature is currently disabled", 2000);
    } else if (action == "toggle_theme") {
        if (isPluginActionEnabled("theme_switcher")) {
            if (m_luaBridge) {
                m_luaBridge->executeString("toggle_theme()");
            }
        } else {
            m_statusBar->showMessage("Theme switcher plugin is disabled", 2000);
        }
    } else if (action == "format_document") {
        if (isPluginActionEnabled("autoformat")) {
            if (m_luaBridge) {
                QString formatScript = "if autoformat then autoformat.format_document() end";
                if (m_luaBridge->executeString(formatScript)) {
                    m_statusBar->showMessage("Document formatted", 2000);
                } else {
                    m_statusBar->showMessage("Format failed: Auto-formatter error", 3000);
                }
            }
        } else {
            m_statusBar->showMessage("Auto-formatter plugin is disabled", 2000);
        }
    } else if (action == "toggle_file_tree") {
        DEBUG_LOG_EDITOR("Toggle file tree action triggered");
        if (m_fileTreeWidget) {
            bool wasVisible = m_fileTreeWidget->isVisible();
            m_fileTreeWidget->toggleVisibility();
            bool nowVisible = m_fileTreeWidget->isVisible();

            if (wasVisible != nowVisible) {
                m_statusBar->showMessage(nowVisible ? "File tree shown" : "File tree hidden", 2000);
            } else if (!m_fileTreeWidget->rootPath().isEmpty()) {
                m_statusBar->showMessage("File tree toggled", 2000);
            } else {
                m_statusBar->showMessage("No project open - open a project folder first", 3000);
            }
        } else {
            DEBUG_LOG_EDITOR("File tree widget is null");
            m_statusBar->showMessage("File tree not available", 2000);
        }
    } else {
        DEBUG_LOG_EDITOR("Unknown action:" << action);
    }
}

void EditorWindow::updateLuaEditorState()
{
    if (!m_luaBridge) {
        return;
    }

    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        return;
    }

    QString text = textEdit->toPlainText();
    QTextCursor cursor = textEdit->textCursor();
    int line = cursor.blockNumber() + 1; 
    int column = cursor.columnNumber() + 1; 

    m_luaBridge->updateEditorState(text, line, column);
}

void EditorWindow::setupSyntaxHighlighting()
{

    for (int i = 0; i < m_textEditors.size(); ++i) {
        setupSyntaxHighlightingForTab(i);
    }

}

void EditorWindow::detectAndSetLanguage(const QString &filePath)
{
    if (!m_luaBridge) {
        return;
    }

    int currentIndex = getCurrentTabIndex();
    if (currentIndex < 0) {
        return;
    }

    QString language = detectLanguageFromExtension(filePath);
    DEBUG_LOG_EDITOR("Detected language:" << language << "for file:" << filePath);

    // For HTML, CSS, and JSON files, use BasicHighlighter
    if (language == "html" || language == "css" || language == "json") {
        if (currentIndex >= m_basicHighlighters.size()) {
            return;
        }

        BasicHighlighter* basicHighlighter = m_basicHighlighters[currentIndex];
        if (!basicHighlighter) {
            return;
        }

        basicHighlighter->setLanguage(language);

        // Ensure the highlighter is properly connected to the document
        CodeEditor* textEdit = m_textEditors[currentIndex];
        if (textEdit && textEdit->document()) {
            // Reconnect the highlighter to ensure it's active
            delete basicHighlighter;
            basicHighlighter = new BasicHighlighter(textEdit->document());
            m_basicHighlighters[currentIndex] = basicHighlighter;

            if (m_luaBridge) {
                basicHighlighter->setLuaBridge(m_luaBridge);
            }

            basicHighlighter->setLanguage(language);
        }

        basicHighlighter->rehighlight();

        DEBUG_LOG_EDITOR("Using BasicHighlighter for tab" << currentIndex << "language:" << language);
    }
    // For Markdown files, use MarkdownHighlighter
    else if (language == "markdown") {
        if (currentIndex >= m_markdownHighlighters.size()) {
            return;
        }

        MarkdownHighlighter* markdownHighlighter = m_markdownHighlighters[currentIndex];
        if (!markdownHighlighter) {
            return;
        }

        markdownHighlighter->setLanguage(language);
        markdownHighlighter->rehighlight();

        DEBUG_LOG_EDITOR("Using MarkdownHighlighter for tab" << currentIndex);
    } else {
        // For other languages, use TreeSitterHighlighter
        if (currentIndex >= m_syntaxHighlighters.size()) {
            return;
        }

        TreeSitterHighlighter* highlighter = m_syntaxHighlighters[currentIndex];
        if (!highlighter) {
            return;
        }

        highlighter->setLanguage(language);
        m_luaBridge->loadSyntaxRulesForLanguage(language);
        highlighter->rehighlight();

        DEBUG_LOG_EDITOR("Using TreeSitterHighlighter for tab" << currentIndex);
    }
}

QString EditorWindow::detectLanguageFromExtension(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName().toLower();

    static QMap<QString, QString> extensionMap = {
        // C/C++
        {"cpp", "cpp"},
        {"cxx", "cpp"},
        {"cc", "cpp"},
        {"c++", "cpp"},
        {"c", "c"},
        {"h", "cpp"},
        {"hpp", "cpp"},
        {"hxx", "cpp"},
        {"h++", "cpp"},

        // Web languages
        {"js", "javascript"},
        {"jsx", "javascript"},
        {"ts", "javascript"}, // TypeScript treated as JavaScript for now
        {"tsx", "javascript"},
        {"html", "html"},
        {"htm", "html"},
        {"css", "css"},
        {"scss", "css"},
        {"sass", "css"},
        {"less", "css"},

        // Other languages
        {"py", "python"},
        {"pyw", "python"},
        {"java", "java"},
        {"rs", "rust"},
        {"go", "go"},
        {"lua", "lua"},
        {"rb", "ruby"},
        {"php", "php"},
        {"cs", "csharp"},
        {"sh", "bash"},
        {"bash", "bash"},
        {"zsh", "bash"},
        {"fish", "bash"},
        {"ps1", "powershell"},

        // Data formats
        {"json", "json"},
        {"xml", "xml"},
        {"yaml", "yaml"},
        {"yml", "yaml"},
        {"toml", "toml"},
        {"ini", "ini"},
        {"cfg", "ini"},
        {"conf", "ini"},

        // Documentation
        {"md", "markdown"},
        {"markdown", "markdown"},

        // Other
        {"txt", "text"},
        {"log", "text"}
    };

    QString language = extensionMap.value(extension, "text");

    // Special file name detection
    if (language == "text") {
        if (fileName == "makefile" || fileName == "cmake" || fileName.startsWith("cmake")) {
            language = "cmake";
        } else if (fileName == "dockerfile" || fileName.startsWith("dockerfile")) {
            language = "dockerfile";
        } else if (fileName.endsWith(".qss")) {
            language = "css";
        } else if (fileName == "cargo.toml") {
            language = "toml";
        } else if (fileName == "package.json") {
            language = "json";
        }
    }

    return language;
}

int EditorWindow::createNewTab(const QString &title)
{

    Buffer* buffer = new Buffer();
    m_buffers.append(buffer);

    CodeEditor* textEdit = new CodeEditor();
    m_textEditors.append(textEdit);

    if (m_luaBridge) {
        QString currentTheme = m_luaBridge->getConfigString("theme.name", "gruvbox");

        QColor background, currentLine, normalLine;
        if (currentTheme == "gruvbox") {
            background = QColor(40, 37, 34);
            currentLine = QColor(251, 241, 199);
            normalLine = QColor(146, 131, 116);
        } else if (currentTheme == "dracula") {
            background = QColor(33, 34, 44);
            currentLine = QColor(248, 248, 242);
            normalLine = QColor(98, 114, 164);
        } else if (currentTheme == "catppuccin-mocha") {
            background = QColor(24, 24, 37);
            currentLine = QColor(205, 214, 244);
            normalLine = QColor(166, 173, 200);
        } else {

            background = QColor(40, 37, 34);
            currentLine = QColor(251, 241, 199);
            normalLine = QColor(146, 131, 116);
        }

        textEdit->setThemeColors(background, currentLine, normalLine);
    }

    if (m_luaBridge) {
        QString fontFamily = m_luaBridge->getConfigString("editor.font_family", "JetBrains Mono");
        int fontSize = m_luaBridge->getConfigInt("editor.font_size", 12);
        int tabWidth = m_luaBridge->getConfigInt("editor.tab_width", 4);
        bool wordWrap = m_luaBridge->getConfigBool("editor.word_wrap", false);
        bool showLineNumbers = m_luaBridge->getConfigBool("editor.show_line_numbers", true);
        bool autoIndent = m_luaBridge->getConfigBool("editor.auto_indent", true);
        bool highlightCurrentLine = m_luaBridge->getConfigBool("editor.highlight_current_line", true);

        QFont font(fontFamily, fontSize);
        font.setStyleHint(QFont::Monospace);
        textEdit->setFont(font);
        textEdit->setTabStopDistance(tabWidth * 10);
        textEdit->setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        textEdit->setLineNumbersVisible(showLineNumbers);
        textEdit->setAutoIndentEnabled(autoIndent);
        textEdit->setCurrentLineHighlightEnabled(highlightCurrentLine);

        if (!m_textEditors.isEmpty()) {
            textEdit->setRelativeLineNumbers(m_textEditors.first()->relativeLineNumbers());
        }
    }

    TreeSitterHighlighter* highlighter = new TreeSitterHighlighter(textEdit->document());
    m_syntaxHighlighters.append(highlighter);

    MarkdownHighlighter* markdownHighlighter = new MarkdownHighlighter(textEdit->document());
    m_markdownHighlighters.append(markdownHighlighter);

    BasicHighlighter* basicHighlighter = new BasicHighlighter(textEdit->document());
    m_basicHighlighters.append(basicHighlighter);

    if (m_luaBridge) {
        markdownHighlighter->setLuaBridge(m_luaBridge);
        basicHighlighter->setLuaBridge(m_luaBridge);
    }

    if (m_luaBridge) {
        m_luaBridge->setSyntaxHighlighter(highlighter);
        highlighter->setLuaBridge(m_luaBridge);
    }

    highlighter->setLanguage("text");

    DEBUG_LOG_EDITOR("Created syntax highlighters for tab" << (m_tabWidget->count() - 1) << "with language: text");

    connect(textEdit, &CodeEditor::textChanged, 
            this, &EditorWindow::onTextChanged);
    connect(textEdit, &CodeEditor::cursorPositionChanged,
            this, &EditorWindow::onCursorPositionChanged);

    int tabIndex = m_tabWidget->addTab(textEdit, title);

    DEBUG_LOG_EDITOR("Created new tab" << tabIndex << "with title:" << title);

    return tabIndex;
}

void EditorWindow::updateTabTitle(int index)
{
    if (index < 0 || index >= m_buffers.size()) {
        return;
    }

    Buffer* buffer = m_buffers[index];
    QString title = buffer->fileName();

    m_tabWidget->setTabText(index, title);
}

void EditorWindow::updateTabModificationIndicator(int index)
{
    if (index < 0 || index >= m_buffers.size()) {
        return;
    }

    Buffer* buffer = m_buffers[index];
    QString title = buffer->fileName();

    if (buffer->isModified()) {
        title += " *";
    }

    m_tabWidget->setTabText(index, title);
}

Buffer* EditorWindow::getCurrentBuffer()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_buffers.size()) {
        return m_buffers[currentIndex];
    }
    return nullptr;
}

CodeEditor* EditorWindow::getCurrentTextEditor()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_textEditors.size()) {
        return m_textEditors[currentIndex];
    }
    return nullptr;
}

TreeSitterHighlighter* EditorWindow::getCurrentSyntaxHighlighter()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_syntaxHighlighters.size()) {
        return m_syntaxHighlighters[currentIndex];
    }
    return nullptr;
}

int EditorWindow::getCurrentTabIndex()
{
    return m_tabWidget->currentIndex();
}

void EditorWindow::setupSyntaxHighlightingForTab(int index)
{
    if (index < 0 || index >= m_textEditors.size() || index >= m_syntaxHighlighters.size()) {
        return;
    }

    CodeEditor* textEdit = m_textEditors[index];
    TreeSitterHighlighter* highlighter = m_syntaxHighlighters[index];

    if (!textEdit || !highlighter) {
        DEBUG_LOG_EDITOR("Cannot setup syntax highlighting for tab" << index << ": missing components");
        return;
    }

    if (m_luaBridge) {
        m_luaBridge->setSyntaxHighlighter(highlighter);
    }

}

void EditorWindow::onTabChanged(int index)
{
    DEBUG_LOG_EDITOR("Tab changed to index:" << index);

    updateWindowTitle();
    updateStatusBar();
    updateLuaEditorState();
}

void EditorWindow::onTabCloseRequested(int index)
{
    DEBUG_LOG_EDITOR("Tab close requested for index:" << index);

    if (index < 0 || index >= m_buffers.size()) {
        return;
    }

    Buffer* buffer = m_buffers[index];

    if (buffer->isModified()) {
        QString fileName = buffer->fileName();
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Unsaved Changes",
            QString("The file '%1' has unsaved changes. Do you want to save before closing?").arg(fileName),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {

            if (buffer->filePath().isEmpty()) {

                QString filePath = QFileDialog::getSaveFileName(this,
                    "Save File", QString(), 
                    "All Files (*);;"
                    "Text Files (*.txt);;"
                    "C/C++ Files (*.c *.cpp *.cxx *.cc *.h *.hpp *.hxx);;"
                    "Python Files (*.py *.pyw);;"
                    "JavaScript Files (*.js *.jsx *.ts *.tsx);;"
                    "Lua Files (*.lua);;"
                    "Java Files (*.java);;"
                    "Markdown Files (*.md *.markdown);;"
                    "JSON Files (*.json);;"
                    "XML Files (*.xml);;"
                    "HTML Files (*.html *.htm);;"
                    "CSS Files (*.css *.scss *.sass);;"
                    "Configuration Files (*.conf *.config *.ini *.cfg)");

                if (filePath.isEmpty()) {
                    return; 
                }

                if (!buffer->save(filePath)) {
                    QMessageBox::warning(this, "Save Error", "Failed to save the file.");
                    return;
                }
            } else {
                if (!buffer->save()) {
                    QMessageBox::warning(this, "Save Error", "Failed to save the file.");
                    return;
                }
            }
        } else if (reply == QMessageBox::Cancel) {
            return; 
        }

    }

    closeFile(index);
}

void EditorWindow::closeFile(int index)
{
    if (index < 0 || index >= m_buffers.size()) {
        return;
    }

    m_tabWidget->removeTab(index);

    delete m_buffers[index];
    m_buffers.removeAt(index);

    m_textEditors.removeAt(index);
    m_syntaxHighlighters.removeAt(index);
    m_markdownHighlighters.removeAt(index);
    m_basicHighlighters.removeAt(index);

    if (m_tabWidget->count() == 0) {
        createNewTab();
    }

    updateWindowTitle();
    updateStatusBar();

    DEBUG_LOG_EDITOR("Closed tab" << index << "- remaining tabs:" << m_tabWidget->count());
}

void EditorWindow::closeCurrentFile()
{
    int currentIndex = getCurrentTabIndex();
    if (currentIndex >= 0) {
        onTabCloseRequested(currentIndex);
    }
}

void EditorWindow::closeEvent(QCloseEvent *event)
{

    QStringList unsavedFiles;
    for (int i = 0; i < m_buffers.size(); ++i) {
        Buffer* buffer = m_buffers[i];
        if (buffer->isModified()) {
            unsavedFiles.append(buffer->fileName());
        }
    }

    if (!unsavedFiles.isEmpty()) {
        QString message = "The following files have unsaved changes:\n\n";
        for (const QString &fileName : unsavedFiles) {
            message += "• " + fileName + "\n";
        }
        message += "\nDo you want to save all changes before closing?";

        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Unsaved Changes",
            message,
            QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::SaveAll) {

            bool allSaved = true;
            for (int i = 0; i < m_buffers.size(); ++i) {
                Buffer* buffer = m_buffers[i];
                if (buffer->isModified()) {
                    if (buffer->filePath().isEmpty()) {

                        QMessageBox::information(this, "Save Required",
                            QString("Please save '%1' manually before closing.").arg(buffer->fileName()));
                        allSaved = false;
                        break;
                    } else {
                        if (!buffer->save()) {
                            QMessageBox::warning(this, "Save Error",
                                QString("Failed to save '%1'.").arg(buffer->fileName()));
                            allSaved = false;
                            break;
                        }
                    }
                }
            }

            if (allSaved) {
                event->accept();
            } else {
                event->ignore();
            }
        } else if (reply == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {

        event->accept();
    }
}

void EditorWindow::keyPressEvent(QKeyEvent *event)
{

    if (m_luaBridge) {
        QVariantList args;
        args << event->key();
        args << static_cast<int>(event->modifiers());
        args << event->text();

        m_luaBridge->emitEvent("key_pressed", args);
    }

    QMainWindow::keyPressEvent(event);
}

void EditorWindow::showFindDialog()
{
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        m_statusBar->showMessage("No text editor available", 2000);
        return;
    }

    bool ok;
    QString searchText = QInputDialog::getText(this, "Find", "Find text:", QLineEdit::Normal, "", &ok);

    if (ok && !searchText.isEmpty()) {
        findText(searchText);
    }
}

void EditorWindow::showReplaceDialog()
{
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        m_statusBar->showMessage("No text editor available", 2000);
        return;
    }

    bool ok;
    QString searchText = QInputDialog::getText(this, "Replace", "Find text:", QLineEdit::Normal, "", &ok);

    if (!ok || searchText.isEmpty()) {
        return;
    }

    QString replaceWith = QInputDialog::getText(this, "Replace", "Replace with:", QLineEdit::Normal, "", &ok);

    if (ok) {

        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Replace", "Replace all occurrences?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            replaceText(searchText, replaceWith, true);
        } else if (reply == QMessageBox::No) {
            replaceText(searchText, replaceWith, false);
        }
    }
}

void EditorWindow::findText(const QString &searchText, bool caseSensitive)
{
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        return;
    }

    QTextDocument::FindFlags flags = QTextDocument::FindFlags();
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    QTextCursor cursor = textEdit->textCursor();
    QTextCursor found = textEdit->document()->find(searchText, cursor, flags);

    if (!found.isNull()) {
        textEdit->setTextCursor(found);
        m_statusBar->showMessage(QString("Found: %1").arg(searchText), 2000);
    } else {

        cursor.movePosition(QTextCursor::Start);
        found = textEdit->document()->find(searchText, cursor, flags);

        if (!found.isNull()) {
            textEdit->setTextCursor(found);
            m_statusBar->showMessage(QString("Found: %1 (wrapped to beginning)").arg(searchText), 2000);
        } else {
            m_statusBar->showMessage(QString("Not found: %1").arg(searchText), 2000);
        }
    }
}

void EditorWindow::replaceText(const QString &searchText, const QString &replaceText, bool replaceAll)
{
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    int replacements = 0;

    if (replaceAll) {

        cursor.movePosition(QTextCursor::Start);

        while (true) {
            QTextCursor found = textEdit->document()->find(searchText, cursor);
            if (found.isNull()) {
                break;
            }

            found.insertText(replaceText);
            cursor = found;
            replacements++;
        }

        m_statusBar->showMessage(QString("Replaced %1 occurrences").arg(replacements), 3000);
    } else {

        if (cursor.hasSelection() && cursor.selectedText() == searchText) {
            cursor.insertText(replaceText);
            replacements = 1;
            m_statusBar->showMessage("Replaced current occurrence", 2000);
        } else {

            findText(searchText);
        }
    }
}

void EditorWindow::setCurrentLanguage(const QString &language)
{
    int currentIndex = getCurrentTabIndex();
    if (currentIndex < 0 || currentIndex >= m_syntaxHighlighters.size()) {
        m_statusBar->showMessage("No active tab to set language", 2000);
        return;
    }

    TreeSitterHighlighter* highlighter = m_syntaxHighlighters[currentIndex];
    if (!highlighter) {
        m_statusBar->showMessage("No syntax highlighter available", 2000);
        return;
    }

    DEBUG_LOG_EDITOR("Setting language to:" << language << "for tab" << currentIndex);

    highlighter->setLanguage(language);

    if (m_luaBridge) {
        m_luaBridge->loadSyntaxRulesForLanguage(language);
    }

    highlighter->rehighlight();

    m_statusBar->showMessage(QString("Syntax highlighting set to: %1").arg(language.toUpper()), 3000);
}

void EditorWindow::loadTheme(const QString &themeName)
{
    DEBUG_LOG_EDITOR("loadTheme called with theme:" << themeName);
    QString themeFile = QString(":/themes/%1.qss").arg(themeName);
    DEBUG_LOG_EDITOR("Theme file path:" << themeFile);
    QFile styleFile(themeFile);

    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        qApp->setStyleSheet(styleSheet);

        DEBUG_LOG_EDITOR("Theme loaded successfully:" << themeName);
        m_statusBar->showMessage(QString("Theme changed to: %1").arg(themeName), 3000);

        if (m_luaBridge) {
            QString setConfigCode = QString("set_config('theme.name', '%1')").arg(themeName);
            m_luaBridge->executeString(setConfigCode);
        }

        updateEditorThemeColors(themeName);

        if (m_luaBridge) {
            QVariantList args;
            args << themeName;
            m_luaBridge->emitEvent("theme_changed", args);
        }
    } else {
        DEBUG_LOG_EDITOR("Failed to load theme:" << themeName);
        m_statusBar->showMessage(QString("Failed to load theme: %1").arg(themeName), 3000);
    }
}

void EditorWindow::applyTheme()
{
    if (!m_luaBridge) {
        DEBUG_LOG_EDITOR("applyTheme: m_luaBridge is null");
        return;
    }

    QString themeName = m_luaBridge->getConfigString("theme.name", "gruvbox");
    DEBUG_LOG_EDITOR("applyTheme: Loading theme:" << themeName);
    loadTheme(themeName);
}

void EditorWindow::onLuaThemeChangeRequested(const QString &themeName)
{
    loadTheme(themeName);
}

void EditorWindow::onFileTreeFileOpenRequested(const QString &filePath)
{
    DEBUG_LOG_EDITOR("File tree requested to open file:" << filePath);
    openFile(filePath);
}

void EditorWindow::onFileTreeVisibilityChanged(bool visible)
{
    DEBUG_LOG_EDITOR("File tree visibility changed:" << visible);
    m_statusBar->showMessage(visible ? "File tree shown" : "File tree hidden", 2000);
}

void EditorWindow::updateEditorThemeColors(const QString &themeName)
{

    QColor background, currentLine, normalLine;

    if (themeName == "gruvbox") {
        background = QColor(40, 37, 34);
        currentLine = QColor(251, 241, 199);
        normalLine = QColor(146, 131, 116);
    } else if (themeName == "dracula") {
        background = QColor(33, 34, 44);
        currentLine = QColor(248, 248, 242);
        normalLine = QColor(98, 114, 164);
    } else if (themeName == "catppuccin-mocha") {
        background = QColor(24, 24, 37);
        currentLine = QColor(205, 214, 244);
        normalLine = QColor(166, 173, 200);
    } else {

        for (CodeEditor* editor : m_textEditors) {
            if (editor) {
                editor->updateThemeColors();
            }
        }

        if (m_fileTreeWidget) {
            m_fileTreeWidget->updateThemeColors();
        }
        return;
    }

    for (CodeEditor* editor : m_textEditors) {
        if (editor) {
            editor->setThemeColors(background, currentLine, normalLine);
        }
    }

    if (m_fileTreeWidget) {
        QColor fileTreeBackground = background.darker(105); 
        QColor fileTreeText = normalLine; 
        QColor fileTreeHighlight = currentLine; 

        m_fileTreeWidget->setThemeColors(fileTreeBackground, fileTreeText, fileTreeHighlight);
    }

    QColor lineNumberBg = background.darker(110);  
    QString handleStyle = QString(
        "QSplitter::handle {"
        "    background-color: %1;"  
        "    border: none;"
        "    margin: 0px;"
        "    padding: 0px;"
        "    width: 1px;"  
        "}"
        "QSplitter::handle:hover {"
        "    background-color: %2;"  
        "}"
    ).arg(lineNumberBg.name(), lineNumberBg.lighter(120).name());

    m_mainSplitter->setStyleSheet(handleStyle);
}
