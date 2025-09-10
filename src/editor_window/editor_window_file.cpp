#include "editor_window.h"

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
            CodeEditor* textEdit = m_textEditors[i];
            if (textEdit) {
                textEdit->setFocus();
            }
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
        if (textEdit) {
            textEdit->setFocus();
        }

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
    CodeEditor* textEdit = m_textEditors[tabIndex];
    if (textEdit) {
        textEdit->setFocus();
    }
    updateWindowTitle();
    updateStatusBar();
}

void EditorWindow::ensureAtLeastOneTab()
{
    if (m_tabWidget->count() == 0) {
        int tabIndex = createNewTab();
        m_tabWidget->setCurrentIndex(tabIndex);
        CodeEditor* textEdit = m_textEditors[tabIndex];
        if (textEdit) {
            textEdit->setFocus();
        }
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
            LOG_ERROR("Failed to load plugins:" << m_luaBridge->lastError());
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
    CodeEditor::LineWrapMode wrapMode = wordWrap ? CodeEditor::WidgetWidth : CodeEditor::NoWrap;

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
