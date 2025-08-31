// syntax rules definitions for different programming languages
// provides clean separation of language-specific highlighting rules
// supports multiple programming languages with organized rule sets

#ifndef SYNTAX_RULES_H
#define SYNTAX_RULES_H

#include <QString>
#include <QVector>
#include <QPair>

class SyntaxHighlighter; 

class SyntaxRules
{
public:

    struct Rule {
        QString pattern;
        QString colorName;
        QString description;

        Rule(const QString &p, const QString &c, const QString &d = QString())
            : pattern(p), colorName(c), description(d) {}
    };

    static QVector<Rule> getCppRules();
    static QVector<Rule> getPythonRules();
    static QVector<Rule> getLuaRules();
    static QVector<Rule> getJavaScriptRules();
    static QVector<Rule> getTypeScriptRules();
    static QVector<Rule> getJavaRules();
    static QVector<Rule> getRustRules();
    static QVector<Rule> getGoRules();
    static QVector<Rule> getJsonRules();
    static QVector<Rule> getXmlHtmlRules();
    static QVector<Rule> getCssRules();
    static QVector<Rule> getGenericRules();

    static void applyRules(SyntaxHighlighter *highlighter, const QString &language);

private:

    static QVector<Rule> getCommonCStyleRules();
    static QVector<Rule> getCommonNumberRules();
    static QVector<Rule> getCommonStringRules();
};

#endif 