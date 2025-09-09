#include "editor_window.h"


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

    KSyntaxHighlighter* highlighter = new KSyntaxHighlighter(textEdit->document());
    m_syntaxHighlighters.append(highlighter);

    if (m_luaBridge) {
        highlighter->setLuaBridge(m_luaBridge);
    }

    if (m_luaBridge) {
        m_luaBridge->setSyntaxHighlighter(highlighter);
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

KSyntaxHighlighter* EditorWindow::getCurrentSyntaxHighlighter()
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
    KSyntaxHighlighter* highlighter = m_syntaxHighlighters[index];

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

    CodeEditor* textEdit = getCurrentTextEditor();
    if (textEdit) {
        textEdit->setFocus();
    }
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
