#include "editor_window.h"

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
