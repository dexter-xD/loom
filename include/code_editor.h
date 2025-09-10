#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include <QtWidgets/QWidget>
#include <QtCore/QObject>
#include <QtGui/QColor>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QFont>
#include <QtCore/QString>
#include <QtGui/QTextDocument>
#include <QtGui/QTextCursor>
#include <QtGui/QResizeEvent>
#include <QtGui/QPaintEvent>
#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KTextEditor/ConfigInterface>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

class CodeEditor : public QWidget
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor();

    QString toPlainText() const;
    void setPlainText(const QString &text);
    void insertPlainText(const QString &text);

    QTextDocument* document() const;
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

    void setFont(const QFont &font);
    QFont font() const;
    void setTabStopDistance(int distance);

    enum LineWrapMode { NoWrap, WidgetWidth };
    void setLineWrapMode(LineWrapMode mode);

    void setFocus();
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    void setRelativeLineNumbers(bool enabled) { m_relativeLineNumbers = enabled; }
    bool relativeLineNumbers() const { return m_relativeLineNumbers; }

    void setLineNumbersVisible(bool visible);
    bool lineNumbersVisible() const;

    void setAutoIndentEnabled(bool enabled);
    bool autoIndentEnabled() const { return m_autoIndentEnabled; }

    void setCurrentLineHighlightEnabled(bool enabled);
    bool currentLineHighlightEnabled() const { return m_currentLineHighlightEnabled; }

    void setThemeColors(const QColor &background, const QColor &currentLine, const QColor &normalLine);
    void updateThemeColors();

    KTextEditor::View* view() const { return m_view; }
    KTextEditor::Document* ktextDocument() const { return m_document; }

    void applyCustomTheme(const QString& themeName);

    void setSyntaxTheme(const QString& syntaxTheme);
    void setLanguage(const QString& language);
    void applyBuiltinSyntaxHighlighting();

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();

signals:
    void textChanged();
    void cursorPositionChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();

private:

    KTextEditor::Editor* m_editor;
    KTextEditor::Document* m_document;
    KTextEditor::View* m_view;

    QVBoxLayout* m_layout;

    bool m_relativeLineNumbers;
    bool m_autoIndentEnabled;
    bool m_currentLineHighlightEnabled;

    QColor m_lineNumberBackground;
    QColor m_lineNumberCurrentLine;
    QColor m_lineNumberNormal;

    void setupEditor();
    void setupConnections();
    void configureEditor();
    void applyEditorSettings();
};

#endif