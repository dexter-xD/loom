#include "editor_window.h"

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

    // Use SyntaxHighlighter for all languages
    if (currentIndex >= m_syntaxHighlighters.size()) {
        return;
    }

    SyntaxHighlighter* highlighter = m_syntaxHighlighters[currentIndex];
    if (!highlighter) {
        return;
    }

    highlighter->setLanguage(language);

    // Ensure the highlighter is properly connected to the document
    CodeEditor* textEdit = m_textEditors[currentIndex];
    if (textEdit && textEdit->document()) {
        // Reconnect the highlighter to ensure it's active
        delete highlighter;
        highlighter = new SyntaxHighlighter(textEdit->document());
        m_syntaxHighlighters[currentIndex] = highlighter;

        if (m_luaBridge) {
            highlighter->setLuaBridge(m_luaBridge);
        }

        highlighter->setLanguage(language);
    }

    highlighter->rehighlight();

    DEBUG_LOG_EDITOR("Using SyntaxHighlighter for tab" << currentIndex << "language:" << language);
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

void EditorWindow::setCurrentLanguage(const QString &language)
{
    int currentIndex = getCurrentTabIndex();
    if (currentIndex < 0 || currentIndex >= m_syntaxHighlighters.size()) {
        m_statusBar->showMessage("No active tab to set language", 2000);
        return;
    }

    SyntaxHighlighter* highlighter = m_syntaxHighlighters[currentIndex];
    if (!highlighter) {
        m_statusBar->showMessage("No syntax highlighter available", 2000);
        return;
    }

    DEBUG_LOG_EDITOR("Setting language to:" << language << "for tab" << currentIndex);

    highlighter->setLanguage(language);
    highlighter->rehighlight();

    m_statusBar->showMessage(QString("Syntax highlighting set to: %1").arg(language.toUpper()), 3000);
}
