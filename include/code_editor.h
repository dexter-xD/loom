#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include <QPlainTextEdit>
#include <QObject>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    
    void setRelativeLineNumbers(bool enabled) { m_relativeLineNumbers = enabled; }
    bool relativeLineNumbers() const { return m_relativeLineNumbers; }
    
    void setLineNumbersVisible(bool visible);
    bool lineNumbersVisible() const { return m_lineNumbersVisible; }
    
    void setAutoIndentEnabled(bool enabled) { m_autoIndentEnabled = enabled; }
    bool autoIndentEnabled() const { return m_autoIndentEnabled; }
    
    void setCurrentLineHighlightEnabled(bool enabled);
    bool currentLineHighlightEnabled() const { return m_currentLineHighlightEnabled; }
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    bool m_relativeLineNumbers;
    bool m_lineNumbersVisible;
    bool m_autoIndentEnabled;
    bool m_currentLineHighlightEnabled;
};

#endif 