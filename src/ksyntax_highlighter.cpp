#include "ksyntax_highlighter.h"
#include "lua_bridge.h"
#include "debug_log.h"

KSyntaxHighlighter::KSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("text")
    , m_luaBridge(nullptr)
    , m_repository()
    , m_highlighter()
{

    m_highlighter.setDocument(parent);

    loadConfiguredTheme();

    DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Initialized with built-in KSyntaxHighlighting");
}

void KSyntaxHighlighter::loadConfiguredTheme()
{
    QString configuredTheme = "ayu Dark"; 

    if (m_luaBridge) {
        configuredTheme = m_luaBridge->getConfigString("theme.syntax_theme", "gruvbox Dark");
    }

    m_theme = m_repository.theme(configuredTheme);

    if (!m_theme.isValid()) {

        m_theme = m_repository.theme("gruvbox Dark");

        if (!m_theme.isValid()) {

            m_theme = m_repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme);
            DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Using fallback default dark theme");
        } else {
            DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Using fallback gruvbox Dark theme");
        }
    } else {
        DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Using configured theme:" << configuredTheme);
    }

    m_highlighter.setTheme(m_theme);
}

void KSyntaxHighlighter::setLanguage(const QString &language)
{
    if (m_currentLanguage != language) {
        DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Setting language to:" << language);
        m_currentLanguage = language;

        auto definition = m_repository.definitionForName(language);

        if (!definition.isValid()) {
            if (language == "javascript") {
                definition = m_repository.definitionForName("JavaScript");
            } else if (language == "python") {
                definition = m_repository.definitionForName("Python");
            } else if (language == "cpp" || language == "c") {
                definition = m_repository.definitionForName("C++");
            } else if (language == "html") {
                definition = m_repository.definitionForName("HTML");
            } else if (language == "css") {
                definition = m_repository.definitionForName("CSS");
            } else if (language == "java") {
                definition = m_repository.definitionForName("Java");
            } else if (language == "rust") {
                definition = m_repository.definitionForName("Rust");
            } else if (language == "go") {
                definition = m_repository.definitionForName("Go");
            } else if (language == "lua") {
                definition = m_repository.definitionForName("Lua");
            } else if (language == "markdown") {
                definition = m_repository.definitionForName("Markdown");
            }
        }

        m_highlighter.setDefinition(definition);
        DEBUG_LOG_SYNTAX("KSyntaxHighlighter: Applied definition for" << language << "(valid:" << definition.isValid() << ")");
    }
}

QString KSyntaxHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

void KSyntaxHighlighter::setLuaBridge(LuaBridge *bridge)
{
    bool wasNull = (m_luaBridge == nullptr);
    m_luaBridge = bridge;

    if (wasNull && bridge) {
        loadConfiguredTheme();
    }
}

void KSyntaxHighlighter::addRule(const QString &pattern, const QString &colorName)
{
    Q_UNUSED(pattern)
    Q_UNUSED(colorName)

}

void KSyntaxHighlighter::clearRules()
{

}

void KSyntaxHighlighter::loadTheme()
{

}

void KSyntaxHighlighter::setupGruvboxColors()
{

}

void KSyntaxHighlighter::highlightBlock(const QString &text)
{

    Q_UNUSED(text)
}

QColor KSyntaxHighlighter::gruvboxColor(const QString &colorName) const
{
    Q_UNUSED(colorName)
    return QColor(); 
}

void KSyntaxHighlighter::listAvailableThemes()
{

    auto themes = m_repository.themes();

    qDebug() << "KSyntaxHighlighter: Available themes:";
    for (const auto &theme : themes) {
        qDebug() << "  -" << theme.name();
    }
    qDebug() << "KSyntaxHighlighter: Total themes available:" << themes.size();
}