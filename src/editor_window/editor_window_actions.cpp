#include "editor_window.h"

void EditorWindow::setupKeybindings()
{
    if (!m_luaBridge) {
        DEBUG_LOG_EDITOR("setupKeybindings: m_luaBridge is null");
        return;
    }

    // Clear existing shortcuts
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

        // Create shortcut with higher priority to override KTextEditor defaults
        QShortcut *shortcut = new QShortcut(QKeySequence(keySequence), this);
        shortcut->setContext(Qt::ApplicationShortcut);
        
        // Set autoRepeat to false to avoid conflicts
        shortcut->setAutoRepeat(false);

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
    DEBUG_LOG_EDITOR("Executing action:" << action);

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
        if (textEdit && textEdit->view()) {
            QAction* undoAction = textEdit->view()->action("edit_undo");
            if (undoAction) {
                undoAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor undo action");
            } else {
                textEdit->undo();
                DEBUG_LOG_EDITOR("Triggered fallback undo action");
            }
        }
    } else if (action == "redo") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* redoAction = textEdit->view()->action("edit_redo");
            if (redoAction) {
                redoAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor redo action");
            } else {
                textEdit->redo();
                DEBUG_LOG_EDITOR("Triggered fallback redo action");
            }
        }
    } else if (action == "copy") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* copyAction = textEdit->view()->action("edit_copy");
            if (copyAction) {
                copyAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor copy action");
            } else {
                textEdit->copy();
                DEBUG_LOG_EDITOR("Triggered fallback copy action");
            }
        }
    } else if (action == "paste") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* pasteAction = textEdit->view()->action("edit_paste");
            if (pasteAction) {
                pasteAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor paste action");
            } else {
                textEdit->paste();
                DEBUG_LOG_EDITOR("Triggered fallback paste action");
            }
        }
    } else if (action == "cut") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* cutAction = textEdit->view()->action("edit_cut");
            if (cutAction) {
                cutAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor cut action");
            } else {
                textEdit->cut();
                DEBUG_LOG_EDITOR("Triggered fallback cut action");
            }
        }
    } else if (action == "select_all") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* selectAllAction = textEdit->view()->action("edit_select_all");
            if (selectAllAction) {
                selectAllAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor select all action");
            } else {
                textEdit->selectAll();
                DEBUG_LOG_EDITOR("Triggered fallback select all action");
            }
        }
    } else if (action == "new_tab") {
        newFile();
    } else if (action == "find") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* findAction = textEdit->view()->action("edit_find");
            if (findAction) {
                findAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor find action");
            } else {
                showFindDialog();
                DEBUG_LOG_EDITOR("Triggered fallback find dialog");
            }
        } else {
            showFindDialog();
        }
    } else if (action == "replace") {
        CodeEditor* textEdit = getCurrentTextEditor();
        if (textEdit && textEdit->view()) {
            QAction* replaceAction = textEdit->view()->action("edit_replace");
            if (replaceAction) {
                replaceAction->trigger();
                DEBUG_LOG_EDITOR("Triggered KTextEditor replace action");
            } else {
                showReplaceDialog();
                DEBUG_LOG_EDITOR("Triggered fallback replace dialog");
            }
        } else {
            showReplaceDialog();
        }
    } else if (action == "toggle_fullscreen") {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
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

    // Try to use KTextEditor's find functionality if available
    if (textEdit->view()) {
        KTextEditor::View* view = textEdit->view();
        QAction* findAction = view->action("edit_find");
        if (findAction) {
            findAction->trigger();
            return;
        }
    }

    // Fallback to basic find functionality
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
        // Wrap to beginning
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

void EditorWindow::setupKTextEditorShortcuts()
{
    // Get the current text editor to setup KTextEditor-specific shortcuts
    CodeEditor* textEdit = getCurrentTextEditor();
    if (!textEdit || !textEdit->view() || !m_luaBridge) {
        DEBUG_LOG_EDITOR("Cannot setup KTextEditor shortcuts - missing components");
        return;
    }

    KTextEditor::View* view = textEdit->view();
    
    DEBUG_LOG_EDITOR("Disabling conflicting KTextEditor shortcuts");
    
    // List of KTextEditor actions that conflict with our shortcuts
    QStringList conflictingActions = {
        "file_save",
        "file_save_as", 
        "file_open",
        "file_new",
        "edit_undo",
        "edit_redo",
        "edit_copy",
        "edit_paste",
        "edit_cut",
        "edit_select_all",
        "edit_find",
        "edit_replace"
    };
    
    // Disable conflicting shortcuts in KTextEditor
    for (const QString& actionName : conflictingActions) {
        QAction* action = view->action(actionName.toUtf8().constData());
        if (action) {
            // Store original shortcut if we need to restore it later
            QKeySequence originalShortcut = action->shortcut();
            
            // Clear the shortcut to prevent conflicts
            action->setShortcut(QKeySequence());
            
            DEBUG_LOG_EDITOR("✓ Disabled KTextEditor shortcut for:" << actionName << "was:" << originalShortcut.toString());
        }
    }
}
