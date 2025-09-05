#ifndef TREE_SITTER_HIGHLIGHTER_H
#define TREE_SITTER_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QColor>
#include <QMap>
#include <QString>
#include <QSet>
#include <QRegExp>
#include <tree_sitter/api.h>

class LuaBridge;

extern "C" {
    TSLanguage *tree_sitter_c();
    TSLanguage *tree_sitter_cpp();
    TSLanguage *tree_sitter_javascript();
    TSLanguage *tree_sitter_python();
    TSLanguage *tree_sitter_rust();
    TSLanguage *tree_sitter_java();
    TSLanguage *tree_sitter_go();
    TSLanguage *tree_sitter_lua();
    TSLanguage *tree_sitter_html();
    TSLanguage *tree_sitter_css();
    TSLanguage *tree_sitter_markdown();
    TSLanguage *tree_sitter_markdown_inline();
}

class TreeSitterHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit TreeSitterHighlighter(QTextDocument *parent = nullptr);
    ~TreeSitterHighlighter();

    void setLanguage(const QString &language);
    QString currentLanguage() const;

    void setupGruvboxColors();
    void setLuaBridge(LuaBridge *bridge);
    QTextCharFormat getFormat(const QString &colorName) const;

    void addRule(const QString &pattern, const QString &colorName);
    void clearRules();

    // Embedded language support
    bool isInsideStyleTag(TSNode node, const QString &fullText);
    bool isInsideScriptTag(TSNode node, const QString &fullText);
    void highlightEmbeddedLanguage(TSNode node, const QString &fullText,
                                  const QString &embeddedLanguage,
                                  int blockStart, int blockEnd);
    void highlightEmbeddedTree(TSNode node, const QString &content,
                              const QString &language, int offset, int blockStart);

    // Markdown code block support
    void highlightMarkdownCodeBlock(TSNode node, const QString &fullText,
                                   int blockStart, int blockEnd);
    QString detectMarkdownCodeLanguage(const QString &codeBlock);

    // Markdown inline support
    void highlightMarkdownInline(const QString &fullText);
    void highlightInlineNode(TSNode node, const QString &blockText, int offset);
    QString getColorForInlineNode(const QString &nodeType, const QString &nodeText);

protected:
    void highlightBlock(const QString &text) override;

private:
    QString m_currentLanguage;
    QMap<QString, QTextCharFormat> m_colorFormats;
    LuaBridge *m_luaBridge;

    // Tree-sitter components
    TSParser *m_parser;
    TSTree *m_tree;
    TSLanguage *m_language;

    void initializeParser();
    void updateTree(const QString &text);
    void walkTreeAndHighlight(TSNode node, const QString &text, int offset = 0);

    QColor gruvboxColor(const QString &colorName) const;
    QString nodeTypeToColorName(const QString &nodeType) const;
    
    // Enhanced helper methods for improved highlighting
    void highlightNode(TSNode node, const QString &fullText);
    QString getColorForNode(const QString &nodeType, const QString &nodeText, TSNode node);
    bool isTypeIdentifier(const QString &text, TSNode node);
    bool isFunctionIdentifier(TSNode node);
    bool isMacroCall(const QString &text, TSNode node);
};

#endif // TREE_SITTER_HIGHLIGHTER_H
