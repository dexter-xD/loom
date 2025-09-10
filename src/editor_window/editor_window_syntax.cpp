#include "editor_window.h"

void EditorWindow::setupSyntaxHighlighting()
{

    for (int i = 0; i < m_textEditors.size(); ++i) {
        CodeEditor* editor = m_textEditors[i];
        if (editor && m_luaBridge) {
            QString syntaxTheme = m_luaBridge->getConfigString("theme.syntax_theme", "ayu Dark");
            editor->setSyntaxTheme(syntaxTheme);
        }
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

    CodeEditor* editor = getCurrentTextEditor();

    if (!editor) {
        return;
    }

    editor->setLanguage(language);

    DEBUG_LOG_EDITOR("Applied KTextEditor syntax highlighting for tab" << currentIndex << "language:" << language);
}

QString EditorWindow::detectLanguageFromExtension(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName().toLower();

    static QMap<QString, QString> extensionMap = {

        {"cpp", "cpp"},
        {"cxx", "cpp"},
        {"cc", "cpp"},
        {"c++", "cpp"},
        {"c", "c"},
        {"h", "cpp"},
        {"hpp", "cpp"},
        {"hxx", "cpp"},
        {"h++", "cpp"},

        {"js", "javascript"},
        {"jsx", "javascript"},
        {"ts", "javascript"}, 
        {"tsx", "javascript"},
        {"html", "html"},
        {"htm", "html"},
        {"css", "css"},
        {"scss", "css"},
        {"sass", "css"},
        {"less", "css"},

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

        {"json", "json"},
        {"xml", "xml"},
        {"yaml", "yaml"},
        {"yml", "yaml"},
        {"toml", "toml"},
        {"ini", "ini"},
        {"cfg", "ini"},
        {"conf", "ini"},

        {"md", "markdown"},
        {"markdown", "markdown"},

        {"txt", "text"},
        {"log", "text"}
    };

    QString language = extensionMap.value(extension, "text");

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
    if (currentIndex < 0 || currentIndex >= m_textEditors.size()) {
        m_statusBar->showMessage("No active tab to set language", 2000);
        return;
    }

    CodeEditor* editor = m_textEditors[currentIndex];
    if (!editor) {
        m_statusBar->showMessage("No editor available", 2000);
        return;
    }

    DEBUG_LOG_EDITOR("Setting language to:" << language << "for tab" << currentIndex);

    editor->setLanguage(language);

    m_statusBar->showMessage(QString("Syntax highlighting set to: %1").arg(language.toUpper()), 3000);
}