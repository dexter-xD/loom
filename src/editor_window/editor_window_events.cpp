#include "editor_window.h"

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
