#include "editor_window.h"

void EditorWindow::showAboutDialog()
{
    QString aboutText = 
        "<h2>Loom Text Editor</h2>"
        "<p>Version 1.2.6</p>"
        "<p>A fast, minimal, and customizable text editor built with C++ and Qt</p>"
        "<p>GitHub: <a href='https://github.com/dexter-xD/loom'>https://github.com/dexter-xD/loom</a></p>"
        "<p>License: MIT</p>";

    QMessageBox::about(this, "About Loom", aboutText);
}

void EditorWindow::setupMenus()
{
    // Get keybindings from Lua configuration
    QMap<QString, QString> keybindings;
    if (m_luaBridge) {
        keybindings = m_luaBridge->getKeybindings();
    }

    // Helper function to find shortcut for action
    auto findShortcut = [&keybindings](const QString& action) -> QString {
        for (auto it = keybindings.begin(); it != keybindings.end(); ++it) {
            if (it.value() == action) {
                return it.key();
            }
        }
        return QString();
    };

    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *newAction = new QAction("&New", this);
    QString newShortcut = findShortcut("new_file");
    if (!newShortcut.isEmpty()) {
        newAction->setText(QString("&New\t%1").arg(newShortcut));
    }
    newAction->setStatusTip("Create a new file");
    connect(newAction, &QAction::triggered, this, &EditorWindow::onNewFile);
    fileMenu->addAction(newAction);

    QAction *newTabAction = new QAction("New &Tab", this);
    QString newTabShortcut = findShortcut("new_tab");
    if (!newTabShortcut.isEmpty()) {
        newTabAction->setText(QString("New &Tab\t%1").arg(newTabShortcut));
    }
    newTabAction->setStatusTip("Create a new tab");
    connect(newTabAction, &QAction::triggered, this, &EditorWindow::onNewFile);
    fileMenu->addAction(newTabAction);

    QAction *openAction = new QAction("&Open", this);
    QString openShortcut = findShortcut("open_file");
    if (!openShortcut.isEmpty()) {
        openAction->setText(QString("&Open\t%1").arg(openShortcut));
    }
    openAction->setStatusTip("Open an existing file");
    connect(openAction, &QAction::triggered, this, &EditorWindow::onOpenFile);
    fileMenu->addAction(openAction);

    fileMenu->addSeparator();

    QAction *saveAction = new QAction("&Save", this);
    QString saveShortcut = findShortcut("save_file");
    if (!saveShortcut.isEmpty()) {
        saveAction->setText(QString("&Save\t%1").arg(saveShortcut));
    }
    saveAction->setStatusTip("Save the current file");
    connect(saveAction, &QAction::triggered, this, &EditorWindow::onSaveFile);
    fileMenu->addAction(saveAction);

    QAction *saveAsAction = new QAction("Save &As...", this);
    saveAsAction->setStatusTip("Save the file with a new name");
    connect(saveAsAction, &QAction::triggered, this, &EditorWindow::onSaveFileAs);
    fileMenu->addAction(saveAsAction);

    fileMenu->addSeparator();

    QAction *closeAction = new QAction("&Close", this);
    QString closeShortcut = findShortcut("close_file");
    if (!closeShortcut.isEmpty()) {
        closeAction->setText(QString("&Close\t%1").arg(closeShortcut));
    }
    closeAction->setStatusTip("Close the current file");
    connect(closeAction, &QAction::triggered, this, &EditorWindow::onCloseFile);
    fileMenu->addAction(closeAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("E&xit", this);
    QString exitShortcut = findShortcut("quit_application");
    if (!exitShortcut.isEmpty()) {
        exitAction->setText(QString("E&xit\t%1").arg(exitShortcut));
    }
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &EditorWindow::onExit);
    fileMenu->addAction(exitAction);

    QMenu *editMenu = menuBar()->addMenu("&Edit");

    QAction *undoAction = new QAction("&Undo", this);
    QString undoShortcut = findShortcut("undo");
    if (!undoShortcut.isEmpty()) {
        undoAction->setText(QString("&Undo\t%1").arg(undoShortcut));
    }
    undoAction->setStatusTip("Undo the last action");
    connect(undoAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->undo();
    });
    editMenu->addAction(undoAction);

    QAction *redoAction = new QAction("&Redo", this);
    QString redoShortcut = findShortcut("redo");
    if (!redoShortcut.isEmpty()) {
        redoAction->setText(QString("&Redo\t%1").arg(redoShortcut));
    }
    redoAction->setStatusTip("Redo the last undone action");
    connect(redoAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->redo();
    });
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    QAction *cutAction = new QAction("Cu&t", this);
    QString cutShortcut = findShortcut("cut");
    if (!cutShortcut.isEmpty()) {
        cutAction->setText(QString("Cu&t\t%1").arg(cutShortcut));
    }
    cutAction->setStatusTip("Cut the selected text");
    connect(cutAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->cut();
    });
    editMenu->addAction(cutAction);

    QAction *copyAction = new QAction("&Copy", this);
    QString copyShortcut = findShortcut("copy");
    if (!copyShortcut.isEmpty()) {
        copyAction->setText(QString("&Copy\t%1").arg(copyShortcut));
    }
    copyAction->setStatusTip("Copy the selected text");
    connect(copyAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->copy();
    });
    editMenu->addAction(copyAction);

    QAction *pasteAction = new QAction("&Paste", this);
    QString pasteShortcut = findShortcut("paste");
    if (!pasteShortcut.isEmpty()) {
        pasteAction->setText(QString("&Paste\t%1").arg(pasteShortcut));
    }
    pasteAction->setStatusTip("Paste text from clipboard");
    connect(pasteAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->paste();
    });
    editMenu->addAction(pasteAction);

    editMenu->addSeparator();

    QAction *selectAllAction = new QAction("Select &All", this);
    QString selectAllShortcut = findShortcut("select_all");
    if (!selectAllShortcut.isEmpty()) {
        selectAllAction->setText(QString("Select &All\t%1").arg(selectAllShortcut));
    }
    selectAllAction->setStatusTip("Select all text");
    connect(selectAllAction, &QAction::triggered, [this]() {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit) textEdit->selectAll();
    });
    editMenu->addAction(selectAllAction);

    editMenu->addSeparator();

    QAction *findAction = new QAction("&Find", this);
    QString findShortcutKey = findShortcut("find");
    if (!findShortcutKey.isEmpty()) {
        findAction->setText(QString("&Find\t%1").arg(findShortcutKey));
    }
    findAction->setStatusTip("Find text in the document");
    connect(findAction, &QAction::triggered, this, &EditorWindow::showFindDialog);
    editMenu->addAction(findAction);

    QAction *replaceAction = new QAction("&Replace", this);
    QString replaceShortcutKey = findShortcut("replace");
    if (!replaceShortcutKey.isEmpty()) {
        replaceAction->setText(QString("&Replace\t%1").arg(replaceShortcutKey));
    }
    replaceAction->setStatusTip("Find and replace text in the document");
    connect(replaceAction, &QAction::triggered, this, &EditorWindow::showReplaceDialog);
    editMenu->addAction(replaceAction);

    QMenu *viewMenu = menuBar()->addMenu("&View");

    QAction *fullscreenAction = new QAction("Toggle &Fullscreen", this);
    QString fullscreenShortcut = findShortcut("toggle_fullscreen");
    if (!fullscreenShortcut.isEmpty()) {
        fullscreenAction->setText(QString("Toggle &Fullscreen\t%1").arg(fullscreenShortcut));
    }
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
    QString fileTreeShortcut = findShortcut("toggle_file_tree");
    if (!fileTreeShortcut.isEmpty()) {
        toggleFileTreeAction->setText(QString("Toggle &File Tree\t%1").arg(fileTreeShortcut));
    }
    toggleFileTreeAction->setStatusTip("Toggle file tree visibility");
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
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = new QAction("&About Loom", this);
    aboutAction->setStatusTip("Show application information");
    connect(aboutAction, &QAction::triggered, this, &EditorWindow::showAboutDialog);
    helpMenu->addAction(aboutAction);
}

void EditorWindow::refreshToolsMenu()
{
    // Get keybindings from Lua configuration
    QMap<QString, QString> keybindings;
    if (m_luaBridge) {
        keybindings = m_luaBridge->getKeybindings();
    }

    // Helper function to find shortcut for action
    auto findShortcut = [&keybindings](const QString& action) -> QString {
        for (auto it = keybindings.begin(); it != keybindings.end(); ++it) {
            if (it.value() == action) {
                return it.key();
            }
        }
        return QString();
    };

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
        QString formatShortcut = findShortcut("format_document");
        if (!formatShortcut.isEmpty()) {
            formatAction->setText(QString("&Format Document\t%1").arg(formatShortcut));
        }
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
        QString themeShortcut = findShortcut("toggle_theme");
        if (!themeShortcut.isEmpty()) {
            toggleThemeAction->setText(QString("&Toggle Theme\t%1").arg(themeShortcut));
        }
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
