#ifndef LINE_NUMBER_AREA_H
#define LINE_NUMBER_AREA_H

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor;
};

#endif 