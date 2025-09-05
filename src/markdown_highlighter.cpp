#include "markdown_highlighter.h"
#include "lua_bridge.h"
#include <QDebug>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("markdown")
    , m_luaBridge(nullptr)
{
    setupDefaultColors();
    setupRules();
}

MarkdownHighlighter::~MarkdownHighlighter()
{
}

void MarkdownHighlighter::setLanguage(const QString &language)
{
    if (m_currentLanguage != language) {
        m_currentLanguage = language;
    }
}

QString MarkdownHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

void MarkdownHighlighter::setLuaBridge(LuaBridge *bridge)
{
    m_luaBridge = bridge;
    if (m_luaBridge) {
        setupDefaultColors(); // Reload colors from config
    }
}

void MarkdownHighlighter::setupDefaultColors()
{
    // Try to get colors from config first
    QMap<QString, QString> configColors;
    if (m_luaBridge) {
        configColors = m_luaBridge->getMarkdownSyntaxColors();
    }

    // Default Gruvbox colors for Markdown (specific palette)
    QMap<QString, QColor> defaultColors;
    defaultColors["heading1"] = QColor("#fb4934");       // Gruvbox bright red for H1
    defaultColors["heading2"] = QColor("#fabd2f");       // Gruvbox bright yellow for H2
    defaultColors["heading3"] = QColor("#b8bb26");       // Gruvbox bright green for H3
    defaultColors["heading4"] = QColor("#83a598");       // Gruvbox bright blue for H4
    defaultColors["heading5"] = QColor("#d3869b");       // Gruvbox bright purple for H5
    defaultColors["heading6"] = QColor("#8ec07c");       // Gruvbox bright aqua for H6
    defaultColors["code"] = QColor("#fe8019");           // Gruvbox bright orange for code
    defaultColors["code_block"] = QColor("#fe8019");     // Gruvbox bright orange for code blocks
    defaultColors["link"] = QColor("#83a598");           // Gruvbox bright blue for links
    defaultColors["emphasis"] = QColor("#d3869b");       // Gruvbox bright purple for emphasis
    defaultColors["strong"] = QColor("#fb4934");         // Gruvbox bright red for strong
    defaultColors["list"] = QColor("#b8bb26");           // Gruvbox bright green for lists
    defaultColors["blockquote"] = QColor("#928374");     // Gruvbox gray for blockquotes
    defaultColors["hr"] = QColor("#665c54");             // Gruvbox dark gray for horizontal rules

    // Helper function to get color from config or default
    auto getColor = [&](const QString &key) -> QColor {
        if (configColors.contains(key) && !configColors[key].isEmpty()) {
            return QColor(configColors[key]);
        }
        return defaultColors.value(key, QColor("#ebdbb2"));
    };

    // Markdown-specific colors (using markdown_syntax config with Gruvbox defaults)
    QTextCharFormat h1Format;
    h1Format.setForeground(getColor("heading1")); // Individual heading colors
    h1Format.setFontWeight(QFont::Bold);
    h1Format.setFontPointSize(18);
    m_colorFormats["heading1"] = h1Format;

    QTextCharFormat h2Format;
    h2Format.setForeground(getColor("heading2"));
    h2Format.setFontWeight(QFont::Bold);
    h2Format.setFontPointSize(16);
    m_colorFormats["heading2"] = h2Format;

    QTextCharFormat h3Format;
    h3Format.setForeground(getColor("heading3"));
    h3Format.setFontWeight(QFont::Bold);
    h3Format.setFontPointSize(14);
    m_colorFormats["heading3"] = h3Format;

    QTextCharFormat h4Format;
    h4Format.setForeground(getColor("heading4"));
    h4Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading4"] = h4Format;

    QTextCharFormat h5Format;
    h5Format.setForeground(getColor("heading5"));
    h5Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading5"] = h5Format;

    QTextCharFormat h6Format;
    h6Format.setForeground(getColor("heading6"));
    h6Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading6"] = h6Format;

    QTextCharFormat codeFormat;
    codeFormat.setForeground(getColor("code"));
    codeFormat.setFontFamily("Monaco, 'Courier New', monospace");
    // codeFormat.setBackground(QColor("#f3f4f6")); // Removed background color
    m_colorFormats["code"] = codeFormat;

    QTextCharFormat codeBlockFormat;
    codeBlockFormat.setForeground(getColor("code_block"));
    codeBlockFormat.setFontFamily("Monaco, 'Courier New', monospace");
    // codeBlockFormat.setBackground(QColor("#f3f4f6")); // Removed background color
    m_colorFormats["code_block"] = codeBlockFormat;

    QTextCharFormat linkFormat;
    linkFormat.setForeground(getColor("link"));
    linkFormat.setFontUnderline(true);
    m_colorFormats["link"] = linkFormat;

    QTextCharFormat emphasisFormat;
    emphasisFormat.setForeground(getColor("emphasis"));
    emphasisFormat.setFontItalic(true);
    m_colorFormats["emphasis"] = emphasisFormat;

    QTextCharFormat strongFormat;
    strongFormat.setForeground(getColor("strong")); // Use strong color for bold text
    strongFormat.setFontWeight(QFont::Bold);
    m_colorFormats["strong"] = strongFormat;

    QTextCharFormat listFormat;
    listFormat.setForeground(getColor("list"));
    m_colorFormats["list"] = listFormat;

    QTextCharFormat blockquoteFormat;
    blockquoteFormat.setForeground(getColor("quote"));
    blockquoteFormat.setFontItalic(true);
    m_colorFormats["blockquote"] = blockquoteFormat;

    QTextCharFormat hrFormat;
    hrFormat.setForeground(getColor("separator"));
    m_colorFormats["hr"] = hrFormat;
}

QTextCharFormat MarkdownHighlighter::getFormat(const QString &colorName) const
{
    return m_colorFormats.value(colorName, QTextCharFormat());
}

void MarkdownHighlighter::setupRules()
{
    // Headers (ATX style: # ## ### etc.)
    HighlightingRule rule;
    rule.pattern = QRegularExpression("^(#{1,6})\\s+(.+)$");
    rule.format = getFormat("heading1"); // Will be overridden in highlightBlock
    m_highlightingRules.append(rule);

    // Inline code
    rule.pattern = QRegularExpression("`([^`\n]+)`");
    rule.format = getFormat("code");
    m_highlightingRules.append(rule);

    // Bold text (**text** or __text__)
    rule.pattern = QRegularExpression("(\\*\\*[^*\n]+\\*\\*|__[^_\n]+__)");
    rule.format = getFormat("strong");
    m_highlightingRules.append(rule);

    // Italic text (*text* or _text_)
    rule.pattern = QRegularExpression("(?<!\\*)(\\*[^*\n]+\\*|(?<!_)_[^_\n]+_)(?!\\*)");
    rule.format = getFormat("emphasis");
    m_highlightingRules.append(rule);

    // Links [text](url)
    rule.pattern = QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    rule.format = getFormat("link");
    m_highlightingRules.append(rule);

    // List markers
    rule.pattern = QRegularExpression("^\\s*([*+-]|\\d+\\.)\\s+");
    rule.format = getFormat("list");
    m_highlightingRules.append(rule);

    // Horizontal rules
    rule.pattern = QRegularExpression("^\\s*([-*_])\\s*\\1\\s*\\1[\\s\\1]*$");
    rule.format = getFormat("hr");
    m_highlightingRules.append(rule);

    // Multi-line rules
    m_codeBlockStartExpression = QRegularExpression("^(```|~~~)");
    m_codeBlockEndExpression = QRegularExpression("^(```|~~~)");
    m_codeBlockFormat = getFormat("code_block");

    m_blockquoteStartExpression = QRegularExpression("^>\\s*");
    m_blockquoteFormat = getFormat("blockquote");
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // Handle multi-line blocks first
    handleMultiLineBlocks(text);

    // Apply single-line highlighting rules
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();

            // Special handling for headers
            if (rule.pattern.pattern().startsWith("^(#{1,6})")) {
                QString colorName = getHeadingColor(match.captured(1));
                setFormat(match.capturedStart(), match.capturedLength(), getFormat(colorName));
            } else {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
}

void MarkdownHighlighter::handleMultiLineBlocks(const QString &text)
{
    // Handle code blocks
    if (previousBlockState() == 1 || text.startsWith("```") || text.startsWith("~~~")) {
        if (previousBlockState() == 1) {
            // We're inside a code block
            setFormat(0, text.length(), m_codeBlockFormat);
            if (text.contains(QRegularExpression("^(```|~~~)"))) {
                setCurrentBlockState(0); // End of code block
            } else {
                setCurrentBlockState(1); // Continue code block
            }
        } else if (text.startsWith("```") || text.startsWith("~~~")) {
            // Start of code block
            setFormat(0, text.length(), m_codeBlockFormat);
            setCurrentBlockState(1);
        }
    }

    // Handle blockquotes
    if (text.startsWith(">")) {
        setFormat(0, text.length(), m_blockquoteFormat);
        setCurrentBlockState(2); // Blockquote state
    } else if (previousBlockState() == 2) {
        // Continue blockquote if previous line was blockquote
        setFormat(0, text.length(), m_blockquoteFormat);
        setCurrentBlockState(2);
    }
}

QString MarkdownHighlighter::getHeadingColor(const QString &markers)
{
    int level = markers.length();
    switch (level) {
        case 1: return "heading1";
        case 2: return "heading2";
        case 3: return "heading3";
        case 4: return "heading4";
        case 5: return "heading5";
        case 6: return "heading6";
        default: return "heading1";
    }
}
