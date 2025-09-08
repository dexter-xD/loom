// syntax highlighter that uses lua-defined rules
// applies gruvbox colors to different syntax elements
// supports multiple programming languages through lua configuration

#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QColor>
#include <QVector>
#include <QString>
#include <QMap>

class SyntaxRules;
class LuaBridge;

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    void setLanguage(const QString &language);
    QString currentLanguage() const;

    void setLuaBridge(LuaBridge *bridge);

    void loadRulesFromLua();
    void clearRules();
    void addRule(const QString &pattern, const QString &colorName);

    void setupGruvboxColors();
    QTextCharFormat getFormat(const QString &colorName) const;

protected:
    void highlightBlock(const QString &text) override;

private:

    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
        QString colorName;
        QString name;

        HighlightRule() = default;
        HighlightRule(const QRegularExpression &p, const QTextCharFormat &f, const QString &c = QString(), const QString &n = QString())
            : pattern(p), format(f), colorName(c), name(n) {}
    };

    QVector<HighlightRule> m_rules;
    QString m_currentLanguage;
    QMap<QString, QTextCharFormat> m_colorFormats;
    LuaBridge *m_luaBridge;

    // Multi-line rules for embedded content
    QRegularExpression m_styleBlockStartExpression;
    QRegularExpression m_styleBlockEndExpression;
    QTextCharFormat m_styleBlockFormat;

    QRegularExpression m_scriptBlockStartExpression;
    QRegularExpression m_scriptBlockEndExpression;
    QTextCharFormat m_scriptBlockFormat;

    // Markdown multi-line rules
    QRegularExpression m_codeBlockStartExpression;
    QRegularExpression m_codeBlockEndExpression;
    QTextCharFormat m_codeBlockFormat;

    QRegularExpression m_blockquoteStartExpression;
    QTextCharFormat m_blockquoteFormat;

    void initializeDefaultRules();
    void setupDefaultColors();
    void setupBasicRules();
    void setupMarkdownRules();
    QColor gruvboxColor(const QString &colorName) const;

    void highlightMultiLineComments(const QString &text);
    void highlightCStyleComments(const QString &text);
    void highlightPythonDocstrings(const QString &text);
    void highlightPythonTripleQuotes(const QString &text, const QString &quote, const QTextCharFormat &format);
    void highlightLuaMultiLineComments(const QString &text);
    void highlightXmlComments(const QString &text);
    void getMultiLineCommentPatterns(QString &startPattern, QString &endPattern);
    bool isAlreadyFormatted(int startIndex, int length);

    // BasicHighlighter methods
    void handleMultiLineBlocks(const QString &text);
    void highlightCssContent(const QString &text, int start, int length);
    void highlightJsContent(const QString &text, int start, int length);

    // MarkdownHighlighter methods
    void handleMarkdownMultiLineBlocks(const QString &text);
    QString getHeadingColor(const QString &markers);
};

#endif
