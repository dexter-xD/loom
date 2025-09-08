#include "editor_window.h"

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
