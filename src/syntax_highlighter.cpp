// syntax highlighter that uses lua-defined rules
// applies gruvbox colors to different syntax elements
// supports multiple programming languages through lua configuration

#include "syntax_highlighter.h"
#include "syntax_rules.h"
#include "lua_bridge.h"
#include "debug_log.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("text")
    , m_luaBridge(nullptr)
{
    setupGruvboxColors();
    initializeDefaultRules();
}

void SyntaxHighlighter::setLanguage(const QString &language)
{
    if (m_currentLanguage != language) {
        DEBUG_LOG_SYNTAX("SyntaxHighlighter: Changing language from" << m_currentLanguage << "to" << language);
        m_currentLanguage = language;
        clearRules();

        initializeDefaultRules();
        DEBUG_LOG_SYNTAX("SyntaxHighlighter: Language changed, rules loaded:" << m_rules.size());
    }
}

void SyntaxHighlighter::setLuaBridge(LuaBridge *bridge)
{
    m_luaBridge = bridge;
    if (m_luaBridge) {
        setupDefaultColors(); // Reload colors from config
        clearRules();
        initializeDefaultRules(); // Update rules with new color formats
        rehighlight(); // Ensure highlighting is updated with new colors
    }
}

QString SyntaxHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

void SyntaxHighlighter::loadRulesFromLua()
{

    initializeDefaultRules();
}

void SyntaxHighlighter::clearRules()
{
    m_rules.clear();
}

void SyntaxHighlighter::addRule(const QString &pattern, const QString &colorName)
{
    QRegularExpression regex(pattern);
    if (!regex.isValid()) {
        DEBUG_LOG_SYNTAX("Invalid regex pattern:" << pattern << "Error:" << regex.errorString());
        return;
    }

    QTextCharFormat format = getFormat(colorName);
    if (format == QTextCharFormat()) {
        DEBUG_LOG_SYNTAX("Unknown color name:" << colorName);
        return;
    }

    m_rules.append(HighlightRule(regex, format, colorName, pattern));
    DEBUG_LOG_SYNTAX("Added highlighting rule:" << pattern << "with color:" << colorName);
}

void SyntaxHighlighter::setupGruvboxColors()
{
    setupDefaultColors();
}

QTextCharFormat SyntaxHighlighter::getFormat(const QString &colorName) const
{
    return m_colorFormats.value(colorName, QTextCharFormat());
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Handle special languages first
    if (m_currentLanguage == "html" || m_currentLanguage == "css" || m_currentLanguage == "json") {
        handleMultiLineBlocks(text);
    } else if (m_currentLanguage == "markdown") {
        handleMarkdownMultiLineBlocks(text);
    }

    highlightMultiLineComments(text);

    for (const HighlightRule &rule : m_rules) {

        if (rule.colorName == "multiline_comment") {
            continue;
        }

        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);

        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            int startIndex = match.capturedStart();
            int length = match.capturedLength();

            // Special handling for Markdown headers
            if (m_currentLanguage == "markdown" && rule.pattern.pattern().startsWith("^(#{1,6})")) {
                QString markers = match.captured(1);
                QString colorName = getHeadingColor(markers);
                QTextCharFormat headerFormat = getFormat(colorName);
                setFormat(startIndex, length, headerFormat);
            } else if (!isAlreadyFormatted(startIndex, length)) {
                setFormat(startIndex, length, rule.format);

                static int formatCount = 0;
                if (formatCount < 5) {
                    DEBUG_LOG_SYNTAX("Applied format for" << rule.colorName << "to:" << text.mid(startIndex, length) << "with color:" << rule.format.foreground().color().name());
                    formatCount++;
                }
            }
        }
    }
}

void SyntaxHighlighter::highlightMultiLineComments(const QString &text)
{

    if (m_currentLanguage == "cpp" || m_currentLanguage == "c" || 
        m_currentLanguage == "javascript" || m_currentLanguage == "typescript" ||
        m_currentLanguage == "java" || m_currentLanguage == "rust" || 
        m_currentLanguage == "go" || m_currentLanguage == "css") {
        highlightCStyleComments(text);
    } else if (m_currentLanguage == "python") {
        highlightPythonDocstrings(text);
    } else if (m_currentLanguage == "lua") {
        highlightLuaMultiLineComments(text);
    } else if (m_currentLanguage == "xml" || m_currentLanguage == "html") {
        highlightXmlComments(text);
    }
}

void SyntaxHighlighter::highlightCStyleComments(const QString &text)
{
    QTextCharFormat commentFormat = getFormat("comment");
    QRegularExpression startExpression("/\\*");
    QRegularExpression endExpression("\\*/");

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        QRegularExpressionMatch match = startExpression.match(text);
        startIndex = match.capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = endExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, commentFormat);

        QRegularExpressionMatch nextMatch = startExpression.match(text, startIndex + commentLength);
        startIndex = nextMatch.capturedStart();
    }
}

void SyntaxHighlighter::highlightPythonDocstrings(const QString &text)
{
    QTextCharFormat commentFormat = getFormat("comment");

    highlightPythonTripleQuotes(text, "\"\"\"", commentFormat);

    highlightPythonTripleQuotes(text, "'''", commentFormat);
}

void SyntaxHighlighter::highlightPythonTripleQuotes(const QString &text, const QString &quote, const QTextCharFormat &format)
{
    QRegularExpression quoteExpression(QRegularExpression::escape(quote));

    int blockState = (quote == "\"\"\"") ? 1 : 2;
    int currentState = previousBlockState();

    int startIndex = 0;
    if (currentState != blockState) {
        QRegularExpressionMatch match = quoteExpression.match(text);
        startIndex = match.capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = quoteExpression.match(text, startIndex + quote.length());
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(blockState);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + quote.length();
            setCurrentBlockState(0);
        }

        setFormat(startIndex, commentLength, format);

        if (endIndex != -1) {
            QRegularExpressionMatch nextMatch = quoteExpression.match(text, endIndex + quote.length());
            startIndex = nextMatch.capturedStart();
        } else {
            break;
        }
    }
}

void SyntaxHighlighter::highlightLuaMultiLineComments(const QString &text)
{
    QTextCharFormat commentFormat = getFormat("comment");
    QTextCharFormat stringFormat = getFormat("string");

    QRegularExpression commentStartExpression("--\\[\\[");
    QRegularExpression commentEndExpression("\\]\\]");

    QRegularExpression stringStartExpression("(?<!-)\\[\\[");
    QRegularExpression stringEndExpression("\\]\\]");

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 3) {
        QRegularExpressionMatch match = commentStartExpression.match(text);
        startIndex = match.capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = commentEndExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(3);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, commentFormat);

        QRegularExpressionMatch nextMatch = commentStartExpression.match(text, startIndex + commentLength);
        startIndex = nextMatch.capturedStart();
    }

    if (previousBlockState() != 3) {
        startIndex = 0;
        if (previousBlockState() != 5) {
            QRegularExpressionMatch match = stringStartExpression.match(text);
            startIndex = match.capturedStart();
        }

        while (startIndex >= 0) {

            if (!isAlreadyFormatted(startIndex, 2)) {
                QRegularExpressionMatch endMatch = stringEndExpression.match(text, startIndex + 2);
                int endIndex = endMatch.capturedStart();
                int stringLength = 0;

                if (endIndex == -1) {
                    setCurrentBlockState(5);
                    stringLength = text.length() - startIndex;
                } else {
                    stringLength = endIndex - startIndex + endMatch.capturedLength();
                }

                setFormat(startIndex, stringLength, stringFormat);

                QRegularExpressionMatch nextMatch = stringStartExpression.match(text, startIndex + stringLength);
                startIndex = nextMatch.capturedStart();
            } else {
                break;
            }
        }
    }
}

void SyntaxHighlighter::highlightXmlComments(const QString &text)
{
    QTextCharFormat commentFormat = getFormat("comment");
    QRegularExpression startExpression("<!--");
    QRegularExpression endExpression("-->");

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 4) {
        QRegularExpressionMatch match = startExpression.match(text);
        startIndex = match.capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = endExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(4);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, commentFormat);

        QRegularExpressionMatch nextMatch = startExpression.match(text, startIndex + commentLength);
        startIndex = nextMatch.capturedStart();
    }
}

void SyntaxHighlighter::getMultiLineCommentPatterns(QString &startPattern, QString &endPattern)
{

    startPattern = "";
    endPattern = "";
}

bool SyntaxHighlighter::isAlreadyFormatted(int startIndex, int length)
{

    QColor commentColor = getFormat("comment").foreground().color();
    QColor stringColor = getFormat("string").foreground().color();

    for (int i = startIndex; i < startIndex + length && i < currentBlock().text().length(); ++i) {
        QTextCharFormat format = this->format(i);
        QColor currentColor = format.foreground().color();

        if (currentColor == commentColor || currentColor == stringColor) {
            return true;
        }
    }
    return false;
}

void SyntaxHighlighter::initializeDefaultRules()
{
    clearRules();

    // Handle special languages that need custom rules
    if (m_currentLanguage == "html" || m_currentLanguage == "css" || m_currentLanguage == "json") {
        setupBasicRules();
    } else if (m_currentLanguage == "markdown") {
        setupMarkdownRules();
    } else {
        // Use the standard syntax rules for other languages
        SyntaxRules::applyRules(this, m_currentLanguage);
    }

    DEBUG_LOG_SYNTAX("Initialized" << m_rules.size() << "default highlighting rules for language:" << m_currentLanguage);
}

void SyntaxHighlighter::setupDefaultColors()
{
    // Try to get colors from config first
    QMap<QString, QString> configColors;
    if (m_luaBridge) {
        configColors = m_luaBridge->getSyntaxColors();
    }

    QMap<QString, QColor> gruvboxColors;

    gruvboxColors["bg"] = QColor("#282828");
    gruvboxColors["fg"] = QColor("#ebdbb2");
    gruvboxColors["red"] = QColor("#cc241d");
    gruvboxColors["green"] = QColor("#98971a");
    gruvboxColors["yellow"] = QColor("#d79921");
    gruvboxColors["blue"] = QColor("#458588");
    gruvboxColors["purple"] = QColor("#b16286");
    gruvboxColors["aqua"] = QColor("#689d6a");
    gruvboxColors["orange"] = QColor("#d65d0e");
    gruvboxColors["gray"] = QColor("#928374");

    gruvboxColors["bright_red"] = QColor("#fb4934");
    gruvboxColors["bright_green"] = QColor("#b8bb26");
    gruvboxColors["bright_yellow"] = QColor("#fabd2f");
    gruvboxColors["bright_blue"] = QColor("#83a598");
    gruvboxColors["bright_purple"] = QColor("#d3869b");
    gruvboxColors["bright_aqua"] = QColor("#8ec07c");
    gruvboxColors["bright_orange"] = QColor("#fe8019");

    // Helper function to get color from config or default
    auto getColor = [&](const QString &key) -> QColor {
        if (configColors.contains(key) && !configColors[key].isEmpty()) {
            return QColor(configColors[key]);
        }
        return gruvboxColors.value(key, QColor("#ebdbb2"));
    };

    // Standard syntax colors
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(getColor("bright_red"));
    keywordFormat.setFontWeight(QFont::Bold);
    m_colorFormats["keyword"] = keywordFormat;

    QTextCharFormat controlFormat;
    controlFormat.setForeground(getColor("bright_red"));
    controlFormat.setFontWeight(QFont::Bold);
    m_colorFormats["control"] = controlFormat;

    QTextCharFormat commentFormat;
    commentFormat.setForeground(getColor("gray"));
    commentFormat.setFontItalic(true);
    m_colorFormats["comment"] = commentFormat;

    QTextCharFormat stringFormat;
    stringFormat.setForeground(getColor("bright_green"));
    m_colorFormats["string"] = stringFormat;

    QTextCharFormat numberFormat;
    numberFormat.setForeground(getColor("bright_purple"));
    m_colorFormats["number"] = numberFormat;

    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(getColor("bright_aqua"));
    preprocessorFormat.setFontWeight(QFont::Bold);
    m_colorFormats["preprocessor"] = preprocessorFormat;

    QTextCharFormat functionFormat;
    functionFormat.setForeground(getColor("bright_blue"));
    functionFormat.setFontWeight(QFont::Bold);
    m_colorFormats["function"] = functionFormat;

    QTextCharFormat typeFormat;
    typeFormat.setForeground(getColor("bright_yellow"));
    typeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["type"] = typeFormat;

    QTextCharFormat operatorFormat;
    operatorFormat.setForeground(getColor("bright_orange"));
    m_colorFormats["operator"] = operatorFormat;

    QTextCharFormat constantFormat;
    constantFormat.setForeground(getColor("purple"));
    constantFormat.setFontWeight(QFont::Bold);
    m_colorFormats["constant"] = constantFormat;

    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(getColor("blue"));
    m_colorFormats["builtin"] = builtinFormat;

    QTextCharFormat annotationFormat;
    annotationFormat.setForeground(getColor("yellow"));
    m_colorFormats["annotation"] = annotationFormat;

    QTextCharFormat escapeFormat;
    escapeFormat.setForeground(getColor("orange"));
    escapeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["escape"] = escapeFormat;

    // BasicHighlighter colors (HTML, CSS, JSON)
    QTextCharFormat tagFormat;
    tagFormat.setForeground(QColor("#fb4934"));
    tagFormat.setFontWeight(QFont::Bold);
    m_colorFormats["tag"] = tagFormat;

    QTextCharFormat attributeFormat;
    attributeFormat.setForeground(QColor("#fabd2f"));
    m_colorFormats["attribute"] = attributeFormat;

    QTextCharFormat attributeValueFormat;
    attributeValueFormat.setForeground(QColor("#b8bb26"));
    m_colorFormats["attribute_value"] = attributeValueFormat;

    QTextCharFormat entityFormat;
    entityFormat.setForeground(QColor("#fe8019"));
    m_colorFormats["entity"] = entityFormat;

    QTextCharFormat cssPropertyFormat;
    cssPropertyFormat.setForeground(QColor("#83a598"));
    m_colorFormats["css_property"] = cssPropertyFormat;

    QTextCharFormat cssValueFormat;
    cssValueFormat.setForeground(QColor("#d3869b"));
    m_colorFormats["css_value"] = cssValueFormat;

    QTextCharFormat cssSelectorFormat;
    cssSelectorFormat.setForeground(QColor("#fabd2f"));
    m_colorFormats["css_selector"] = cssSelectorFormat;

    QTextCharFormat jsKeywordFormat;
    jsKeywordFormat.setForeground(QColor("#fb4934"));
    jsKeywordFormat.setFontWeight(QFont::Bold);
    m_colorFormats["js_keyword"] = jsKeywordFormat;

    QTextCharFormat jsStringFormat;
    jsStringFormat.setForeground(QColor("#b8bb26"));
    m_colorFormats["js_string"] = jsStringFormat;

    QTextCharFormat jsCommentFormat;
    jsCommentFormat.setForeground(QColor("#928374"));
    jsCommentFormat.setFontItalic(true);
    m_colorFormats["js_comment"] = jsCommentFormat;

    QTextCharFormat jsFunctionFormat;
    jsFunctionFormat.setForeground(QColor("#83a598"));
    m_colorFormats["js_function"] = jsFunctionFormat;

    QTextCharFormat jsonKeyFormat;
    jsonKeyFormat.setForeground(QColor("#b8bb26"));
    jsonKeyFormat.setFontWeight(QFont::Bold);
    m_colorFormats["json_key"] = jsonKeyFormat;

    QTextCharFormat jsonStringFormat;
    jsonStringFormat.setForeground(QColor("#b8bb26"));
    m_colorFormats["json_string"] = jsonStringFormat;

    QTextCharFormat jsonNumberFormat;
    jsonNumberFormat.setForeground(QColor("#d3869b"));
    m_colorFormats["json_number"] = jsonNumberFormat;

    QTextCharFormat jsonBooleanFormat;
    jsonBooleanFormat.setForeground(QColor("#fb4934"));
    jsonBooleanFormat.setFontWeight(QFont::Bold);
    m_colorFormats["json_boolean"] = jsonBooleanFormat;

    QTextCharFormat jsonNullFormat;
    jsonNullFormat.setForeground(QColor("#fe8019"));
    jsonNullFormat.setFontItalic(true);
    m_colorFormats["json_null"] = jsonNullFormat;

    // MarkdownHighlighter colors
    QTextCharFormat h1Format;
    h1Format.setForeground(QColor("#fb4934"));
    h1Format.setFontWeight(QFont::Bold);
    h1Format.setFontPointSize(18);
    m_colorFormats["heading1"] = h1Format;

    QTextCharFormat h2Format;
    h2Format.setForeground(QColor("#fabd2f"));
    h2Format.setFontWeight(QFont::Bold);
    h2Format.setFontPointSize(16);
    m_colorFormats["heading2"] = h2Format;

    QTextCharFormat h3Format;
    h3Format.setForeground(QColor("#b8bb26"));
    h3Format.setFontWeight(QFont::Bold);
    h3Format.setFontPointSize(14);
    m_colorFormats["heading3"] = h3Format;

    QTextCharFormat h4Format;
    h4Format.setForeground(QColor("#83a598"));
    h4Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading4"] = h4Format;

    QTextCharFormat h5Format;
    h5Format.setForeground(QColor("#d3869b"));
    h5Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading5"] = h5Format;

    QTextCharFormat h6Format;
    h6Format.setForeground(QColor("#8ec07c"));
    h6Format.setFontWeight(QFont::Bold);
    m_colorFormats["heading6"] = h6Format;

    QTextCharFormat codeFormat;
    codeFormat.setForeground(QColor("#fe8019"));
    codeFormat.setFontFamily("Monaco, 'Courier New', monospace");
    m_colorFormats["code"] = codeFormat;

    QTextCharFormat codeBlockFormat;
    codeBlockFormat.setForeground(QColor("#fe8019"));
    codeBlockFormat.setFontFamily("Monaco, 'Courier New', monospace");
    m_colorFormats["code_block"] = codeBlockFormat;

    QTextCharFormat linkFormat;
    linkFormat.setForeground(QColor("#83a598"));
    linkFormat.setFontUnderline(true);
    m_colorFormats["link"] = linkFormat;

    QTextCharFormat emphasisFormat;
    emphasisFormat.setForeground(QColor("#d3869b"));
    emphasisFormat.setFontItalic(true);
    m_colorFormats["emphasis"] = emphasisFormat;

    QTextCharFormat strongFormat;
    strongFormat.setForeground(QColor("#fb4934"));
    strongFormat.setFontWeight(QFont::Bold);
    m_colorFormats["strong"] = strongFormat;

    QTextCharFormat listFormat;
    listFormat.setForeground(QColor("#b8bb26"));
    m_colorFormats["list"] = listFormat;

    QTextCharFormat blockquoteFormat;
    blockquoteFormat.setForeground(QColor("#928374"));
    blockquoteFormat.setFontItalic(true);
    m_colorFormats["blockquote"] = blockquoteFormat;

    QTextCharFormat hrFormat;
    hrFormat.setForeground(QColor("#665c54"));
    m_colorFormats["hr"] = hrFormat;

    DEBUG_LOG_SYNTAX("Gruvbox color scheme initialized with" << m_colorFormats.size() << "color formats");
}

void SyntaxHighlighter::setupBasicRules()
{
    // Clear existing rules
    m_rules.clear();

    // HTML comments
    HighlightRule rule;
    rule.pattern = QRegularExpression("<!--[^>]*-->");
    rule.format = getFormat("comment");
    m_rules.append(rule);

    // HTML entities
    rule.pattern = QRegularExpression("&[a-zA-Z0-9#]+;");
    rule.format = getFormat("entity");
    m_rules.append(rule);

    // HTML tags
    rule.pattern = QRegularExpression("</?\\w+");
    rule.format = getFormat("tag");
    m_rules.append(rule);

    // HTML attributes
    rule.pattern = QRegularExpression("\\b\\w+(?=\\s*=)");
    rule.format = getFormat("attribute");
    m_rules.append(rule);

    // HTML attribute values (quoted)
    rule.pattern = QRegularExpression("=\\s*\"[^\"]*\"|=\\s*'[^']*'");
    rule.format = getFormat("attribute_value");
    m_rules.append(rule);

    // Multi-line rules for embedded content
    m_styleBlockStartExpression = QRegularExpression("<style[^>]*>");
    m_styleBlockEndExpression = QRegularExpression("</style>");
    m_styleBlockFormat = getFormat("css_selector");

    m_scriptBlockStartExpression = QRegularExpression("<script[^>]*>");
    m_scriptBlockEndExpression = QRegularExpression("</script>");
    m_scriptBlockFormat = getFormat("js_keyword");

    // CSS rules for standalone CSS files and embedded CSS
    HighlightRule cssRule;

    // CSS selectors (class, id, element, pseudo-classes, etc.)
    cssRule.pattern = QRegularExpression("^\\s*([.#]?[a-zA-Z][a-zA-Z0-9_-]*|\\*|::?[a-zA-Z][a-zA-Z0-9_-]*|\\[.*\\]|:[a-zA-Z][a-zA-Z0-9_-]*|[a-zA-Z][a-zA-Z0-9_-]*\\s*[~+>])");
    cssRule.format = getFormat("css_selector");
    m_rules.append(cssRule);

    // CSS properties
    cssRule.pattern = QRegularExpression("\\b([a-zA-Z-]+)\\s*:");
    cssRule.format = getFormat("css_property");
    m_rules.append(cssRule);

    // CSS values (numbers, colors, units, etc.)
    cssRule.pattern = QRegularExpression(":\\s*([^;{}]+);");
    cssRule.format = getFormat("css_value");
    m_rules.append(cssRule);

    // CSS comments
    cssRule.pattern = QRegularExpression("/\\*.*?\\*/");
    cssRule.format = getFormat("comment");
    m_rules.append(cssRule);

    // CSS at-rules (@media, @import, etc.)
    cssRule.pattern = QRegularExpression("^\\s*@([a-zA-Z-]+)");
    cssRule.format = getFormat("css_property");
    m_rules.append(cssRule);

    // CSS functions
    cssRule.pattern = QRegularExpression("\\b([a-zA-Z-]+)\\s*\\(");
    cssRule.format = getFormat("js_function");
    m_rules.append(cssRule);

    // JSON rules for standalone JSON files
    HighlightRule jsonRule;

    // JSON object keys (strings followed by colon)
    jsonRule.pattern = QRegularExpression("\"[^\"]*\"\\s*:");
    jsonRule.format = getFormat("json_key");
    m_rules.append(jsonRule);

    // JSON string values (but not keys)
    jsonRule.pattern = QRegularExpression(":\\s*\"[^\"]*\"");
    jsonRule.format = getFormat("json_string");
    m_rules.append(jsonRule);

    // JSON numbers
    jsonRule.pattern = QRegularExpression(":\\s*\\b\\d+(\\.\\d+)?\\b");
    jsonRule.format = getFormat("json_number");
    m_rules.append(jsonRule);

    // JSON booleans
    jsonRule.pattern = QRegularExpression("\\b(true|false)\\b");
    jsonRule.format = getFormat("json_boolean");
    m_rules.append(jsonRule);

    // JSON null
    jsonRule.pattern = QRegularExpression("\\bnull\\b");
    jsonRule.format = getFormat("json_null");
    m_rules.append(jsonRule);
}

void SyntaxHighlighter::setupMarkdownRules()
{
    // Clear existing rules
    m_rules.clear();

    // Headers (ATX style: # ## ### etc.)
    HighlightRule rule;
    rule.pattern = QRegularExpression("^(#{1,6})\\s+(.+)$");
    rule.format = getFormat("heading1"); // Will be overridden in highlightBlock
    m_rules.append(rule);

    // Inline code
    rule.pattern = QRegularExpression("`([^`\n]+)`");
    rule.format = getFormat("code");
    m_rules.append(rule);

    // Bold text (**text** or __text__)
    rule.pattern = QRegularExpression("(\\*\\*[^*\n]+\\*\\*|__[^_\n]+__)");
    rule.format = getFormat("strong");
    m_rules.append(rule);

    // Italic text (*text* or _text_)
    rule.pattern = QRegularExpression("(?<!\\*)(\\*[^*\n]+\\*|(?<!_)_[^_\n]+_)(?!\\*)");
    rule.format = getFormat("emphasis");
    m_rules.append(rule);

    // Links [text](url)
    rule.pattern = QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    rule.format = getFormat("link");
    m_rules.append(rule);

    // List markers
    rule.pattern = QRegularExpression("^\\s*([*+-]|\\d+\\.)\\s+");
    rule.format = getFormat("list");
    m_rules.append(rule);

    // Horizontal rules
    rule.pattern = QRegularExpression("^\\s*([-*_])\\s*\\1\\s*\\1[\\s\\1]*$");
    rule.format = getFormat("hr");
    m_rules.append(rule);

    // Multi-line rules
    m_codeBlockStartExpression = QRegularExpression("^(```|~~~)");
    m_codeBlockEndExpression = QRegularExpression("^(```|~~~)");
    m_codeBlockFormat = getFormat("code_block");

    m_blockquoteStartExpression = QRegularExpression("^>\\s*");
    m_blockquoteFormat = getFormat("blockquote");
}

void SyntaxHighlighter::handleMultiLineBlocks(const QString &text)
{
    // Handle CSS in <style> blocks
    if (previousBlockState() == 1 || text.contains(QRegularExpression("<style[^>]*>"))) {
        if (previousBlockState() == 1) {
            // We're inside a <style> block
            highlightCssContent(text, 0, text.length());
            if (text.contains("</style>")) {
                setCurrentBlockState(0); // End of style block
            } else {
                setCurrentBlockState(1); // Continue style block
            }
        } else if (text.contains(QRegularExpression("<style[^>]*>"))) {
            // Start of style block
            QRegularExpressionMatch match = QRegularExpression("<style[^>]*>").match(text);
            if (match.hasMatch()) {
                int styleStart = match.capturedEnd();
                QString remainingText = text.mid(styleStart);
                if (!remainingText.contains("</style>")) {
                    setCurrentBlockState(1);
                    highlightCssContent(remainingText, styleStart, remainingText.length());
                }
            }
        }
    }

    // Handle JavaScript in <script> blocks
    if (previousBlockState() == 2 || text.contains(QRegularExpression("<script[^>]*>"))) {
        if (previousBlockState() == 2) {
            // We're inside a <script> block
            highlightJsContent(text, 0, text.length());
            if (text.contains("</script>")) {
                setCurrentBlockState(0); // End of script block
            } else {
                setCurrentBlockState(2); // Continue script block
            }
        } else if (text.contains(QRegularExpression("<script[^>]*>"))) {
            // Start of script block
            QRegularExpressionMatch match = QRegularExpression("<script[^>]*>").match(text);
            if (match.hasMatch()) {
                int scriptStart = match.capturedEnd();
                QString remainingText = text.mid(scriptStart);
                if (!remainingText.contains("</script>")) {
                    setCurrentBlockState(2);
                    highlightJsContent(remainingText, scriptStart, remainingText.length());
                }
            }
        }
    }
}

void SyntaxHighlighter::highlightCssContent(const QString &text, int start, int length)
{
    if (length <= 0) return;

    QString cssText = text.mid(start, length);

    // CSS selectors
    QRegularExpression selectorRegex("([.#]?[a-zA-Z][a-zA-Z0-9_-]*|\\*)");
    QRegularExpressionMatchIterator selectorIt = selectorRegex.globalMatch(cssText);
    while (selectorIt.hasNext()) {
        QRegularExpressionMatch match = selectorIt.next();
        setFormat(start + match.capturedStart(), match.capturedLength(), getFormat("css_selector"));
    }

    // CSS properties
    QRegularExpression propertyRegex("\\b([a-zA-Z-]+)\\s*:");
    QRegularExpressionMatchIterator propertyIt = propertyRegex.globalMatch(cssText);
    while (propertyIt.hasNext()) {
        QRegularExpressionMatch match = propertyIt.next();
        setFormat(start + match.capturedStart(), match.capturedLength() - 1, getFormat("css_property"));
    }

    // CSS values
    QRegularExpression valueRegex(":\\s*([^;]+);");
    QRegularExpressionMatchIterator valueIt = valueRegex.globalMatch(cssText);
    while (valueIt.hasNext()) {
        QRegularExpressionMatch match = valueIt.next();
        setFormat(start + match.capturedStart() + 1, match.capturedLength() - 1, getFormat("css_value"));
    }

    // CSS comments
    QRegularExpression cssCommentRegex("/\\*.*?\\*/");
    QRegularExpressionMatchIterator cssCommentIt = cssCommentRegex.globalMatch(cssText);
    while (cssCommentIt.hasNext()) {
        QRegularExpressionMatch match = cssCommentIt.next();
        setFormat(start + match.capturedStart(), match.capturedLength(), getFormat("comment"));
    }
}

void SyntaxHighlighter::highlightJsContent(const QString &text, int start, int length)
{
    if (length <= 0) return;

    QString jsText = text.mid(start, length);

    // JavaScript keywords
    QStringList jsKeywords = {"function", "var", "let", "const", "if", "else", "for", "while",
                             "do", "switch", "case", "default", "break", "continue", "return",
                             "try", "catch", "finally", "throw", "new", "this", "typeof",
                             "instanceof", "in", "delete", "void", "class", "extends", "super",
                             "import", "export", "from", "async", "await", "true", "false", "null"};

    for (const QString &keyword : jsKeywords) {
        QRegularExpression keywordRegex("\\b" + keyword + "\\b");
        QRegularExpressionMatchIterator keywordIt = keywordRegex.globalMatch(jsText);
        while (keywordIt.hasNext()) {
            QRegularExpressionMatch match = keywordIt.next();
            setFormat(start + match.capturedStart(), match.capturedLength(), getFormat("js_keyword"));
        }
    }

    // JavaScript strings
    QRegularExpression stringRegex("\"[^\"]*\"|'[^']*'");
    QRegularExpressionMatchIterator stringIt = stringRegex.globalMatch(jsText);
    while (stringIt.hasNext()) {
        QRegularExpressionMatch match = stringIt.next();
        setFormat(start + match.capturedStart(), match.capturedLength(), getFormat("js_string"));
    }

    // JavaScript comments
    QRegularExpression commentRegex("//[^\n]*|/\\*.*?\\*/");
    QRegularExpressionMatchIterator commentIt = commentRegex.globalMatch(jsText);
    while (commentIt.hasNext()) {
        QRegularExpressionMatch match = commentIt.next();
        setFormat(start + match.capturedStart(), match.capturedLength(), getFormat("js_comment"));
    }

    // JavaScript function names
    QRegularExpression functionRegex("\\bfunction\\s+([a-zA-Z_$][a-zA-Z0-9_$]*)");
    QRegularExpressionMatchIterator functionIt = functionRegex.globalMatch(jsText);
    while (functionIt.hasNext()) {
        QRegularExpressionMatch match = functionIt.next();
        if (match.capturedLength() > 1) {
            setFormat(start + match.capturedStart(1), match.capturedLength(1), getFormat("js_function"));
        }
    }
}

void SyntaxHighlighter::handleMarkdownMultiLineBlocks(const QString &text)
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

QString SyntaxHighlighter::getHeadingColor(const QString &markers)
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

QColor SyntaxHighlighter::gruvboxColor(const QString &colorName) const
{

    static QMap<QString, QColor> colors = {
        {"bg", QColor("#282828")},
        {"fg", QColor("#ebdbb2")},
        {"red", QColor("#cc241d")},
        {"green", QColor("#98971a")},
        {"yellow", QColor("#d79921")},
        {"blue", QColor("#458588")},
        {"purple", QColor("#b16286")},
        {"aqua", QColor("#689d6a")},
        {"orange", QColor("#d65d0e")},
        {"gray", QColor("#928374")},
        {"bright_red", QColor("#fb4934")},
        {"bright_green", QColor("#b8bb26")},
        {"bright_yellow", QColor("#fabd2f")},
        {"bright_blue", QColor("#83a598")},
        {"bright_purple", QColor("#d3869b")},
        {"bright_aqua", QColor("#8ec07c")},
        {"bright_orange", QColor("#fe8019")}
    };

    return colors.value(colorName, QColor("#ebdbb2"));
}
