#include "code_editor.h"
#include "line_number_area.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QKeyEvent>

CodeEditor::CodeEditor(QWidget *parent) 
    : QPlainTextEdit(parent)
    , m_relativeLineNumbers(false)
    , m_lineNumbersVisible(true)
    , m_autoIndentEnabled(true)
    , m_currentLineHighlightEnabled(true)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    if (m_lineNumbersVisible) {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly() && m_currentLineHighlightEnabled) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(60, 56, 54, 60);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
    if (m_lineNumbersVisible) {
        lineNumberArea->update();
    }
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_lineNumbersVisible) {
        return;
    }

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(40, 37, 34));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number;
            
            if (m_relativeLineNumbers && blockNumber != currentLine) {
                int distance = qAbs(blockNumber - currentLine);
                number = QString::number(distance);
            } else {
                number = QString::number(blockNumber + 1);
            }
            
            if (blockNumber == currentLine) {
                painter.setPen(QColor(251, 241, 199));
            } else {
                painter.setPen(QColor(146, 131, 116));
            }
            
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (m_lineNumbersVisible != visible) {
        m_lineNumbersVisible = visible;
        lineNumberArea->setVisible(visible);
        updateLineNumberAreaWidth(0);
        update();
    }
}

void CodeEditor::setCurrentLineHighlightEnabled(bool enabled)
{
    if (m_currentLineHighlightEnabled != enabled) {
        m_currentLineHighlightEnabled = enabled;
        highlightCurrentLine();
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_autoIndentEnabled && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        QTextCursor cursor = textCursor();
        QTextBlock currentBlock = cursor.block();
        QString currentLine = currentBlock.text();
        
        int indentLevel = 0;
        for (QChar ch : currentLine) {
            if (ch == ' ') {
                indentLevel++;
            } else if (ch == '\t') {
                indentLevel += 4; 
            } else {
                break;
            }
        }
        
        QPlainTextEdit::keyPressEvent(event);
        
        QString indentString = QString(indentLevel, ' ');
        insertPlainText(indentString);
    } else {
        QPlainTextEdit::keyPressEvent(event);
    }
}