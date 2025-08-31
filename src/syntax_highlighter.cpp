// syntax highlighter that uses lua-defined rules
// applies gruvbox colors to different syntax elements
// supports multiple programming languages through lua configuration

#include "syntax_highlighter.h"
#include "syntax_rules.h"
#include "debug_log.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_currentLanguage("text")
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

            if (!isAlreadyFormatted(startIndex, length)) {
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

    SyntaxRules::applyRules(this, m_currentLanguage);

    DEBUG_LOG_SYNTAX("Initialized" << m_rules.size() << "default highlighting rules for language:" << m_currentLanguage);
}

void SyntaxHighlighter::setupDefaultColors()
{

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

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(gruvboxColors["bright_red"]);
    keywordFormat.setFontWeight(QFont::Bold);
    m_colorFormats["keyword"] = keywordFormat;

    QTextCharFormat controlFormat;
    controlFormat.setForeground(gruvboxColors["bright_red"]);
    controlFormat.setFontWeight(QFont::Bold);
    m_colorFormats["control"] = controlFormat;

    QTextCharFormat commentFormat;
    commentFormat.setForeground(gruvboxColors["gray"]);
    commentFormat.setFontItalic(true);
    m_colorFormats["comment"] = commentFormat;

    QTextCharFormat stringFormat;
    stringFormat.setForeground(gruvboxColors["bright_green"]);
    m_colorFormats["string"] = stringFormat;

    QTextCharFormat numberFormat;
    numberFormat.setForeground(gruvboxColors["bright_purple"]);
    m_colorFormats["number"] = numberFormat;

    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(gruvboxColors["bright_aqua"]);
    preprocessorFormat.setFontWeight(QFont::Bold);
    m_colorFormats["preprocessor"] = preprocessorFormat;

    QTextCharFormat functionFormat;
    functionFormat.setForeground(gruvboxColors["bright_blue"]);
    functionFormat.setFontWeight(QFont::Bold);
    m_colorFormats["function"] = functionFormat;

    QTextCharFormat typeFormat;
    typeFormat.setForeground(gruvboxColors["bright_yellow"]);
    typeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["type"] = typeFormat;

    QTextCharFormat operatorFormat;
    operatorFormat.setForeground(gruvboxColors["bright_orange"]);
    m_colorFormats["operator"] = operatorFormat;

    QTextCharFormat constantFormat;
    constantFormat.setForeground(gruvboxColors["purple"]);
    constantFormat.setFontWeight(QFont::Bold);
    m_colorFormats["constant"] = constantFormat;

    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(gruvboxColors["blue"]);
    m_colorFormats["builtin"] = builtinFormat;

    QTextCharFormat annotationFormat;
    annotationFormat.setForeground(gruvboxColors["yellow"]);
    m_colorFormats["annotation"] = annotationFormat;

    QTextCharFormat escapeFormat;
    escapeFormat.setForeground(gruvboxColors["orange"]);
    escapeFormat.setFontWeight(QFont::Bold);
    m_colorFormats["escape"] = escapeFormat;

    DEBUG_LOG_SYNTAX("Gruvbox color scheme initialized with" << m_colorFormats.size() << "color formats");
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