#include "code_editor.h"

#include <QtWidgets/QVBoxLayout>
#include <QtGui/QTextBlock>
#include <QtWidgets/QScrollBar>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAction>
#include <QtCore/QDebug>
#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/MarkInterface>
#include <KTextEditor/MovingInterface>

CodeEditor::CodeEditor(QWidget *parent) 
    : QWidget(parent)
    , m_editor(nullptr)
    , m_document(nullptr)
    , m_view(nullptr)
    , m_layout(nullptr)
    , m_relativeLineNumbers(false)
    , m_autoIndentEnabled(true)
    , m_currentLineHighlightEnabled(true)
    , m_lineNumberBackground(40, 37, 34)     
    , m_lineNumberCurrentLine(251, 241, 199)
    , m_lineNumberNormal(146, 131, 116)
{
    setupEditor();
    setupConnections();
    configureEditor();
}

CodeEditor::~CodeEditor()
{
    if (m_document) {
        delete m_document;
    }
}

void CodeEditor::setupEditor()
{

    m_editor = KTextEditor::Editor::instance();
    if (!m_editor) {
        qWarning("Failed to create KTextEditor instance");
        return;
    }

    m_document = m_editor->createDocument(this);
    m_view = m_document->createView(this);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_view);
    setLayout(m_layout);
}

void CodeEditor::setupConnections()
{
    if (!m_document || !m_view) return;

    connect(m_document, &KTextEditor::Document::textChanged,
            this, &CodeEditor::onTextChanged);
    connect(m_view, &KTextEditor::View::cursorPositionChanged,
            this, &CodeEditor::onCursorPositionChanged);
}

void CodeEditor::configureEditor()
{
    if (!m_view || !m_document) return;

    if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {

        config->setConfigValue(QStringLiteral("line-numbers"), true);

        config->setConfigValue(QStringLiteral("highlight-current-line"), m_currentLineHighlightEnabled);

        config->setConfigValue(QStringLiteral("auto-indent"), m_autoIndentEnabled);

        config->setConfigValue(QStringLiteral("tab-width"), 4);

        config->setConfigValue(QStringLiteral("dynamic-word-wrap"), false);

        config->setConfigValue(QStringLiteral("show-tabs"), false);
        config->setConfigValue(QStringLiteral("show-trailing-spaces"), false);
    }

    applyBuiltinSyntaxHighlighting();
    applyEditorSettings();
}

void CodeEditor::applyEditorSettings()
{
    if (!m_view) return;

    m_view->setStatusBarEnabled(false);
}

void CodeEditor::applyBuiltinSyntaxHighlighting()
{
    if (!m_document) return;

}

QString CodeEditor::toPlainText() const
{
    return m_document ? m_document->text() : QString();
}

void CodeEditor::setPlainText(const QString &text)
{
    if (m_document) {
        m_document->setText(text);
    }
}

void CodeEditor::insertPlainText(const QString &text)
{
    if (m_view) {
        m_document->insertText(m_view->cursorPosition(), text);
    }
}

QTextDocument* CodeEditor::document() const
{

    return nullptr; 
}

QTextCursor CodeEditor::textCursor() const
{

    return QTextCursor();
}

void CodeEditor::setTextCursor(const QTextCursor &cursor)
{

    Q_UNUSED(cursor)
}

void CodeEditor::setFont(const QFont &font)
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            config->setConfigValue(QStringLiteral("font"), font);
        }
    }
}

QFont CodeEditor::font() const
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            return config->configValue(QStringLiteral("font")).value<QFont>();
        }
    }
    return QFont();
}

void CodeEditor::setTabStopDistance(int distance)
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {

            int tabWidth = distance / 10; 
            config->setConfigValue(QStringLiteral("tab-width"), tabWidth);
        }
    }
}

void CodeEditor::setLineWrapMode(LineWrapMode mode)
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            bool wrap = (mode == WidgetWidth);
            config->setConfigValue(QStringLiteral("dynamic-word-wrap"), wrap);
        }
    }
}

void CodeEditor::setFocus()
{
    if (m_view) {
        m_view->setFocus();
    }
}

bool CodeEditor::isReadOnly() const
{
    return m_document ? m_document->isReadWrite() == false : false;
}

void CodeEditor::setReadOnly(bool readOnly)
{
    if (m_document) {
        m_document->setReadWrite(!readOnly);
    }
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            config->setConfigValue(QStringLiteral("line-numbers"), visible);
        }
    }
}

bool CodeEditor::lineNumbersVisible() const
{
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            return config->configValue(QStringLiteral("line-numbers")).toBool();
        }
    }
    return true;
}

void CodeEditor::setAutoIndentEnabled(bool enabled)
{
    m_autoIndentEnabled = enabled;
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            config->setConfigValue(QStringLiteral("auto-indent"), enabled);
        }
    }
}

void CodeEditor::setCurrentLineHighlightEnabled(bool enabled)
{
    m_currentLineHighlightEnabled = enabled;
    if (m_view) {
        if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {
            config->setConfigValue(QStringLiteral("highlight-current-line"), enabled);
        }
    }
}

void CodeEditor::setThemeColors(const QColor &background, const QColor &currentLine, const QColor &normalLine)
{
    m_lineNumberBackground = background;
    m_lineNumberCurrentLine = currentLine;
    m_lineNumberNormal = normalLine;

}

void CodeEditor::updateThemeColors()
{

    QPalette palette = this->palette();
    QColor baseColor = palette.color(QPalette::Base);
    QColor textColor = palette.color(QPalette::Text);

    m_lineNumberBackground = baseColor.darker(110);
    m_lineNumberCurrentLine = textColor;
    m_lineNumberNormal = textColor.darker(150);

    setThemeColors(m_lineNumberBackground, m_lineNumberCurrentLine, m_lineNumberNormal);
}

void CodeEditor::applyCustomTheme(const QString& themeName)
{

    if (themeName == "gruvbox") {
        setThemeColors(QColor(40, 37, 34), QColor(251, 241, 199), QColor(146, 131, 116));
    } else if (themeName == "dracula") {
        setThemeColors(QColor(33, 34, 44), QColor(248, 248, 242), QColor(98, 114, 164));
    } else if (themeName == "catppuccin-mocha") {
        setThemeColors(QColor(24, 24, 37), QColor(205, 214, 244), QColor(166, 173, 200));
    } else {

        setThemeColors(QColor(40, 37, 34), QColor(251, 241, 199), QColor(146, 131, 116));
    }
}

void CodeEditor::setSyntaxTheme(const QString& syntaxTheme)
{
    if (!m_view) return;

    if (auto config = qobject_cast<KTextEditor::ConfigInterface*>(m_view)) {

        config->setConfigValue(QStringLiteral("theme"), syntaxTheme);
        qDebug() << "Applied syntax theme:" << syntaxTheme;
    }

    if (auto docConfig = qobject_cast<KTextEditor::ConfigInterface*>(m_document)) {
        docConfig->setConfigValue(QStringLiteral("theme"), syntaxTheme);
    }

    if (m_document) {
        QString currentMode = m_document->highlightingMode();
        m_document->setHighlightingMode(currentMode);
    }
}

void CodeEditor::setLanguage(const QString& language)
{
    if (!m_document) return;

    QString mode = language;
    if (language == "cpp" || language == "c") {
        mode = "C++";
    } else if (language == "javascript") {
        mode = "JavaScript";
    } else if (language == "python") {
        mode = "Python";
    } else if (language == "html") {
        mode = "HTML";
    } else if (language == "css") {
        mode = "CSS";
    } else if (language == "java") {
        mode = "Java";
    } else if (language == "rust") {
        mode = "Rust";
    } else if (language == "go") {
        mode = "Go";
    } else if (language == "lua") {
        mode = "Lua";
    } else if (language == "markdown") {
        mode = "Markdown";
    } else if (language == "json") {
        mode = "JSON";
    } else if (language == "xml") {
        mode = "XML";
    } else if (language == "yaml") {
        mode = "YAML";
    }

    m_document->setHighlightingMode(mode);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

}

void CodeEditor::onTextChanged()
{
    emit textChanged();
}

void CodeEditor::onCursorPositionChanged()
{
    emit cursorPositionChanged();
}

void CodeEditor::undo()
{
    if (m_view) {
        m_view->action("edit_undo")->trigger();
    }
}

void CodeEditor::redo()
{
    if (m_view) {
        m_view->action("edit_redo")->trigger();
    }
}

void CodeEditor::cut()
{
    if (m_view) {
        m_view->action("edit_cut")->trigger();
    }
}

void CodeEditor::copy()
{
    if (m_view) {
        m_view->action("edit_copy")->trigger();
    }
}

void CodeEditor::paste()
{
    if (m_view) {
        m_view->action("edit_paste")->trigger();
    }
}

void CodeEditor::selectAll()
{
    if (m_view) {
        m_view->action("edit_select_all")->trigger();
    }
}