#ifndef KSYNTAX_HIGHLIGHTER_H
#define KSYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/SyntaxHighlighter>

class LuaBridge;

class KSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit KSyntaxHighlighter(QTextDocument *parent = nullptr);

    void setLanguage(const QString &language);
    QString currentLanguage() const;

    void setLuaBridge(LuaBridge *bridge);

    void loadTheme();
    void loadConfiguredTheme();
    void setupGruvboxColors();
    void listAvailableThemes();

    // Methods for Lua compatibility
    void addRule(const QString &pattern, const QString &colorName);
    void clearRules();

protected:
    void highlightBlock(const QString &text) override;

private:
    QString m_currentLanguage;
    LuaBridge *m_luaBridge;
    KSyntaxHighlighting::Repository m_repository;
    KSyntaxHighlighting::SyntaxHighlighter m_highlighter;
    KSyntaxHighlighting::Theme m_theme;

    void initializeDefaultTheme();
    QColor gruvboxColor(const QString &colorName) const;
};

#endif 
