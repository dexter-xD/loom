#ifndef MARKDOWN_HIGHLIGHTER_H
#define MARKDOWN_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QColor>
#include <QMap>
#include <QString>
#include <QRegularExpression>

class LuaBridge;

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);
    ~MarkdownHighlighter();

    void setLanguage(const QString &language);
    QString currentLanguage() const;

    void setLuaBridge(LuaBridge *bridge);

    QTextCharFormat getFormat(const QString &colorName) const;

protected:
    void highlightBlock(const QString &text) override;

private:
    QString m_currentLanguage;
    QMap<QString, QTextCharFormat> m_colorFormats;
    LuaBridge *m_luaBridge;

    void setupDefaultColors();
    void setupRules();
    void handleMultiLineBlocks(const QString &text);

    // Highlighting rules
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    // Multi-line rules
    QRegularExpression m_codeBlockStartExpression;
    QRegularExpression m_codeBlockEndExpression;
    QTextCharFormat m_codeBlockFormat;

    QRegularExpression m_blockquoteStartExpression;
    QTextCharFormat m_blockquoteFormat;

    QString getHeadingColor(const QString &text);
};

#endif // MARKDOWN_HIGHLIGHTER_H
