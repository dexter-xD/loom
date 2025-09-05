#ifndef BASIC_HIGHLIGHTER_H
#define BASIC_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QColor>
#include <QMap>
#include <QString>
#include <QRegularExpression>

class LuaBridge;

class BasicHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit BasicHighlighter(QTextDocument *parent = nullptr);
    ~BasicHighlighter();

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

    // Multi-line rules for embedded content
    QRegularExpression m_styleBlockStartExpression;
    QRegularExpression m_styleBlockEndExpression;
    QTextCharFormat m_styleBlockFormat;

    QRegularExpression m_scriptBlockStartExpression;
    QRegularExpression m_scriptBlockEndExpression;
    QTextCharFormat m_scriptBlockFormat;

    // Embedded language highlighting
    void highlightCssContent(const QString &text, int start, int length);
    void highlightJsContent(const QString &text, int start, int length);

    QString getHeadingColor(const QString &text);
};

#endif // BASIC_HIGHLIGHTER_H
