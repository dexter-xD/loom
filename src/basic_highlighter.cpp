#include "basic_highlighter.h"
#include "lua_bridge.h"
#include <QDebug>

BasicHighlighter::BasicHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("html")
    , m_luaBridge(nullptr)
{
    setupDefaultColors();
    setupRules();
}

BasicHighlighter::~BasicHighlighter()
{
}

void BasicHighlighter::setLanguage(const QString &language)
{
    if (m_currentLanguage != language) {
        m_currentLanguage = language;
    }
}

QString BasicHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

void BasicHighlighter::setLuaBridge(LuaBridge *bridge)
{
    m_luaBridge = bridge;
    if (m_luaBridge) {
        setupDefaultColors(); // Reload colors from config
        setupRules(); // Update rules with new color formats
        rehighlight(); // Ensure highlighting is updated with new colors
    }
}

void BasicHighlighter::setupDefaultColors()
{
    // Try to get colors from config first
    QMap<QString, QString> configColors;
    if (m_luaBridge) {
        configColors = m_luaBridge->getBasicHighlighterColors();
    }

    // Default Gruvbox colors for HTML (specific palette)
    QMap<QString, QColor> defaultColors;
    defaultColors["tag"] = QColor("#fb4934");           // Gruvbox bright red for HTML tags
    defaultColors["attribute"] = QColor("#fabd2f");     // Gruvbox bright yellow for attributes
    defaultColors["attribute_value"] = QColor("#b8bb26"); // Gruvbox bright green for attribute values
    defaultColors["string"] = QColor("#b8bb26");        // Gruvbox bright green for strings
    defaultColors["comment"] = QColor("#928374");       // Gruvbox gray for comments
    defaultColors["entity"] = QColor("#fe8019");        // Gruvbox bright orange for entities
    defaultColors["css_property"] = QColor("#83a598");  // Gruvbox bright blue for CSS properties
    defaultColors["css_value"] = QColor("#d3869b");     // Gruvbox bright purple for CSS values
    defaultColors["css_selector"] = QColor("#fabd2f");  // Gruvbox bright yellow for CSS selectors
    defaultColors["js_keyword"] = QColor("#fb4934");    // Gruvbox bright red for JS keywords
    defaultColors["js_string"] = QColor("#b8bb26");     // Gruvbox bright green for JS strings
    defaultColors["js_comment"] = QColor("#928374");    // Gruvbox gray for JS comments
    defaultColors["js_function"] = QColor("#83a598");   // Gruvbox bright blue for JS functions

    // Helper function to get color from config or default
    auto getColor = [&](const QString &key) -> QColor {
        if (configColors.contains(key) && !configColors[key].isEmpty()) {
            return QColor(configColors[key]);
        }
        return defaultColors.value(key, QColor("#ebdbb2"));
    };

    // HTML-specific colors (using specific Gruvbox palette)
    QTextCharFormat tagFormat;
    tagFormat.setForeground(getColor("tag"));
    tagFormat.setFontWeight(QFont::Bold);
    m_colorFormats["tag"] = tagFormat;

    QTextCharFormat attributeFormat;
    attributeFormat.setForeground(getColor("attribute"));
    m_colorFormats["attribute"] = attributeFormat;

    QTextCharFormat attributeValueFormat;
    attributeValueFormat.setForeground(getColor("attribute_value"));
    m_colorFormats["attribute_value"] = attributeValueFormat;

    QTextCharFormat stringFormat;
    stringFormat.setForeground(getColor("string"));
    m_colorFormats["string"] = stringFormat;

    QTextCharFormat commentFormat;
    commentFormat.setForeground(getColor("comment"));
    commentFormat.setFontItalic(true);
    m_colorFormats["comment"] = commentFormat;

    QTextCharFormat entityFormat;
    entityFormat.setForeground(getColor("entity"));
    m_colorFormats["entity"] = entityFormat;

    // CSS formats (using specific Gruvbox palette)
    QTextCharFormat cssPropertyFormat;
    cssPropertyFormat.setForeground(getColor("css_property"));
    m_colorFormats["css_property"] = cssPropertyFormat;

    QTextCharFormat cssValueFormat;
    cssValueFormat.setForeground(getColor("css_value"));
    m_colorFormats["css_value"] = cssValueFormat;

    QTextCharFormat cssSelectorFormat;
    cssSelectorFormat.setForeground(getColor("css_selector"));
    m_colorFormats["css_selector"] = cssSelectorFormat;

    // JavaScript formats (using specific Gruvbox palette)
    QTextCharFormat jsKeywordFormat;
    jsKeywordFormat.setForeground(getColor("js_keyword"));
    jsKeywordFormat.setFontWeight(QFont::Bold);
    m_colorFormats["js_keyword"] = jsKeywordFormat;

    QTextCharFormat jsStringFormat;
    jsStringFormat.setForeground(getColor("js_string"));
    m_colorFormats["js_string"] = jsStringFormat;

    QTextCharFormat jsCommentFormat;
    jsCommentFormat.setForeground(getColor("js_comment"));
    jsCommentFormat.setFontItalic(true);
    m_colorFormats["js_comment"] = jsCommentFormat;

    QTextCharFormat jsFunctionFormat;
    jsFunctionFormat.setForeground(getColor("js_function"));
    m_colorFormats["js_function"] = jsFunctionFormat;

    // JSON formats
    QTextCharFormat jsonKeyFormat;
    jsonKeyFormat.setForeground(getColor("json_key"));
    jsonKeyFormat.setFontWeight(QFont::Bold);
    m_colorFormats["json_key"] = jsonKeyFormat;

    QTextCharFormat jsonStringFormat;
    jsonStringFormat.setForeground(getColor("json_string"));
    m_colorFormats["json_string"] = jsonStringFormat;

    QTextCharFormat jsonNumberFormat;
    jsonNumberFormat.setForeground(getColor("json_number"));
    m_colorFormats["json_number"] = jsonNumberFormat;

    QTextCharFormat jsonBooleanFormat;
    jsonBooleanFormat.setForeground(getColor("json_boolean"));
    jsonBooleanFormat.setFontWeight(QFont::Bold);
    m_colorFormats["json_boolean"] = jsonBooleanFormat;

    QTextCharFormat jsonNullFormat;
    jsonNullFormat.setForeground(getColor("json_null"));
    jsonNullFormat.setFontItalic(true);
    m_colorFormats["json_null"] = jsonNullFormat;
}

QTextCharFormat BasicHighlighter::getFormat(const QString &colorName) const
{
    return m_colorFormats.value(colorName, QTextCharFormat());
}

void BasicHighlighter::setupRules()
{
    // Clear existing rules
    m_highlightingRules.clear();

    // HTML comments
    HighlightingRule rule;
    rule.pattern = QRegularExpression("<!--[^>]*-->");
    rule.format = getFormat("comment");
    m_highlightingRules.append(rule);

    // HTML entities
    rule.pattern = QRegularExpression("&[a-zA-Z0-9#]+;");
    rule.format = getFormat("entity");
    m_highlightingRules.append(rule);

    // HTML tags
    rule.pattern = QRegularExpression("</?\\w+");
    rule.format = getFormat("tag");
    m_highlightingRules.append(rule);

    // HTML attributes
    rule.pattern = QRegularExpression("\\b\\w+(?=\\s*=)");
    rule.format = getFormat("attribute");
    m_highlightingRules.append(rule);

    // HTML attribute values (quoted)
    rule.pattern = QRegularExpression("=\\s*\"[^\"]*\"|=\\s*'[^']*'");
    rule.format = getFormat("attribute_value");
    m_highlightingRules.append(rule);

    // Multi-line rules for embedded content
    m_styleBlockStartExpression = QRegularExpression("<style[^>]*>");
    m_styleBlockEndExpression = QRegularExpression("</style>");
    m_styleBlockFormat = getFormat("css_selector"); // Default format for style blocks

    m_scriptBlockStartExpression = QRegularExpression("<script[^>]*>");
    m_scriptBlockEndExpression = QRegularExpression("</script>");
    m_scriptBlockFormat = getFormat("js_keyword"); // Default format for script blocks

    // CSS rules for standalone CSS files and embedded CSS
    HighlightingRule cssRule;

    // CSS selectors (class, id, element, pseudo-classes, etc.)
    cssRule.pattern = QRegularExpression("^\\s*([.#]?[a-zA-Z][a-zA-Z0-9_-]*|\\*|::?[a-zA-Z][a-zA-Z0-9_-]*|\\[.*\\]|:[a-zA-Z][a-zA-Z0-9_-]*|[a-zA-Z][a-zA-Z0-9_-]*\\s*[~+>])");
    cssRule.format = getFormat("css_selector");
    m_highlightingRules.append(cssRule);

    // CSS properties
    cssRule.pattern = QRegularExpression("\\b([a-zA-Z-]+)\\s*:");
    cssRule.format = getFormat("css_property");
    m_highlightingRules.append(cssRule);

    // CSS values (numbers, colors, units, etc.)
    cssRule.pattern = QRegularExpression(":\\s*([^;{}]+);");
    cssRule.format = getFormat("css_value");
    m_highlightingRules.append(cssRule);

    // CSS comments
    cssRule.pattern = QRegularExpression("/\\*.*?\\*/");
    cssRule.format = getFormat("comment");
    m_highlightingRules.append(cssRule);

    // CSS at-rules (@media, @import, etc.)
    cssRule.pattern = QRegularExpression("^\\s*@([a-zA-Z-]+)");
    cssRule.format = getFormat("css_property");
    m_highlightingRules.append(cssRule);

    // CSS functions
    cssRule.pattern = QRegularExpression("\\b([a-zA-Z-]+)\\s*\\(");
    cssRule.format = getFormat("js_function");
    m_highlightingRules.append(cssRule);

    // JSON rules for standalone JSON files
    HighlightingRule jsonRule;

    // JSON object keys (strings followed by colon)
    jsonRule.pattern = QRegularExpression("\"[^\"]*\"\\s*:");
    jsonRule.format = getFormat("json_key");
    m_highlightingRules.append(jsonRule);

    // JSON string values (but not keys)
    jsonRule.pattern = QRegularExpression(":\\s*\"[^\"]*\"");
    jsonRule.format = getFormat("json_string");
    m_highlightingRules.append(jsonRule);

    // JSON numbers
    jsonRule.pattern = QRegularExpression(":\\s*\\b\\d+(\\.\\d+)?\\b");
    jsonRule.format = getFormat("json_number");
    m_highlightingRules.append(jsonRule);

    // JSON booleans
    jsonRule.pattern = QRegularExpression("\\b(true|false)\\b");
    jsonRule.format = getFormat("json_boolean");
    m_highlightingRules.append(jsonRule);

    // JSON null
    jsonRule.pattern = QRegularExpression("\\bnull\\b");
    jsonRule.format = getFormat("json_null");
    m_highlightingRules.append(jsonRule);
}

void BasicHighlighter::highlightBlock(const QString &text)
{
    // Handle multi-line blocks first (CSS and JavaScript)
    handleMultiLineBlocks(text);

    // Apply single-line highlighting rules
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

void BasicHighlighter::handleMultiLineBlocks(const QString &text)
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

void BasicHighlighter::highlightCssContent(const QString &text, int start, int length)
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

void BasicHighlighter::highlightJsContent(const QString &text, int start, int length)
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

QString BasicHighlighter::getHeadingColor(const QString &text)
{
    Q_UNUSED(text)
    return "tag"; // HTML doesn't have headings like Markdown
}
