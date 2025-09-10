#include "editor_window.h"

void EditorWindow::loadTheme(const QString &themeName)
{
    DEBUG_LOG_EDITOR("loadTheme called with theme:" << themeName);

    QString qssFileName;
    if (themeName == "Atom One Dark") {
        qssFileName = "atom-one-dark";
    } else if (themeName == "Atom One Light") {
        qssFileName = "atom-one-light";
    } else if (themeName == "Breeze Dark") {
        qssFileName = "breeze-dark";
    } else if (themeName == "Breeze Light") {
        qssFileName = "breeze-light";
    } else if (themeName == "Dracula") {
        qssFileName = "dracula";
    } else if (themeName == "Falcon") {
        qssFileName = "falcon";
    } else if (themeName == "GitHub Dark") {
        qssFileName = "github-dark";
    } else if (themeName == "GitHub Light") {
        qssFileName = "github-light";
    } else if (themeName == "Monokai") {
        qssFileName = "monokai";
    } else if (themeName == "Nord") {
        qssFileName = "nord";
    } else if (themeName == "Oblivion") {
        qssFileName = "oblivion";
    } else if (themeName == "Printing") {
        qssFileName = "printing";
    } else if (themeName == "Radical") {
        qssFileName = "radical";
    } else if (themeName == "Solarized Dark") {
        qssFileName = "solarized-dark";
    } else if (themeName == "Solarized Light") {
        qssFileName = "solarized-light";
    } else if (themeName == "Vim Dark") {
        qssFileName = "vim-dark";
    } else if (themeName == "ayu Dark") {
        qssFileName = "ayu-dark";
    } else if (themeName == "ayu Light") {
        qssFileName = "ayu-light";
    } else if (themeName == "ayu Mirage") {
        qssFileName = "ayu-mirage";
    } else if (themeName == "gruvbox Dark") {
        qssFileName = "gruvbox-dark";
    } else if (themeName == "gruvbox Light") {
        qssFileName = "gruvbox-light";
    } else {

        qssFileName = "gruvbox-dark";
    }

    QString themeFile = QString(":/themes/%1.qss").arg(qssFileName);
    DEBUG_LOG_EDITOR("Theme file path:" << themeFile);
    QFile styleFile(themeFile);

    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        qApp->setStyleSheet(styleSheet);

        DEBUG_LOG_EDITOR("Theme loaded successfully:" << themeName);
        m_statusBar->showMessage(QString("Theme changed to: %1").arg(themeName), 3000);

        if (m_luaBridge) {
            QString setConfigCode = QString("set_config('theme.name', '%1')").arg(themeName);
            m_luaBridge->executeString(setConfigCode);
        }

        updateEditorThemeColors(themeName);

        if (m_luaBridge) {
            QVariantList args;
            args << themeName;
            m_luaBridge->emitEvent("theme_changed", args);
        }
    } else {
        DEBUG_LOG_EDITOR("Failed to load theme:" << themeName);
        m_statusBar->showMessage(QString("Failed to load theme: %1").arg(themeName), 3000);
    }
}

void EditorWindow::applyTheme()
{
    if (!m_luaBridge) {
        DEBUG_LOG_EDITOR("applyTheme: m_luaBridge is null");
        return;
    }

    QString themeName = m_luaBridge->getConfigString("theme.name", "gruvbox Dark");
    DEBUG_LOG_EDITOR("applyTheme: Loading theme:" << themeName);
    loadTheme(themeName);
}

void EditorWindow::onLuaThemeChangeRequested(const QString &themeName)
{
    loadTheme(themeName);
}

void EditorWindow::updateEditorThemeColors(const QString &themeName)
{

    for (CodeEditor* editor : m_textEditors) {
        if (editor) {
            editor->applyCustomTheme(themeName);

            editor->setSyntaxTheme(themeName);
        }
    }

    QColor background, currentLine, normalLine;

    if (themeName == "Atom One Dark") {
        background = QColor(40, 44, 52);
        currentLine = QColor(171, 178, 191);
        normalLine = QColor(130, 137, 151);
    } else if (themeName == "Atom One Light") {
        background = QColor(250, 250, 250);
        currentLine = QColor(36, 41, 46);
        normalLine = QColor(88, 96, 105);
    } else if (themeName == "Breeze Dark") {
        background = QColor(35, 38, 41);
        currentLine = QColor(252, 252, 252);
        normalLine = QColor(189, 195, 199);
    } else if (themeName == "Breeze Light") {
        background = QColor(252, 252, 252);
        currentLine = QColor(35, 38, 41);
        normalLine = QColor(49, 54, 59);
    } else if (themeName == "Dracula") {
        background = QColor(40, 42, 54);
        currentLine = QColor(248, 248, 242);
        normalLine = QColor(98, 114, 164);
    } else if (themeName == "Falcon") {
        background = QColor(2, 2, 33);
        currentLine = QColor(180, 180, 185);
        normalLine = QColor(144, 144, 153);
    } else if (themeName == "GitHub Dark") {
        background = QColor(13, 17, 23);
        currentLine = QColor(240, 246, 252);
        normalLine = QColor(125, 133, 144);
    } else if (themeName == "GitHub Light") {
        background = QColor(255, 255, 255);
        currentLine = QColor(36, 41, 46);
        normalLine = QColor(88, 96, 105);
    } else if (themeName == "Monokai") {
        background = QColor(39, 40, 34);
        currentLine = QColor(248, 248, 242);
        normalLine = QColor(117, 113, 94);
    } else if (themeName == "Nord") {
        background = QColor(46, 52, 64);
        currentLine = QColor(216, 222, 233);
        normalLine = QColor(143, 188, 187);
    } else if (themeName == "Oblivion") {
        background = QColor(46, 52, 54);
        currentLine = QColor(211, 215, 207);
        normalLine = QColor(136, 138, 133);
    } else if (themeName == "Printing") {
        background = QColor(255, 255, 255);
        currentLine = QColor(0, 0, 0);
        normalLine = QColor(66, 66, 66);
    } else if (themeName == "Radical") {
        background = QColor(13, 17, 23);
        currentLine = QColor(0, 255, 255);
        normalLine = QColor(125, 133, 144);
    } else if (themeName == "Solarized Dark") {
        background = QColor(0, 43, 54);
        currentLine = QColor(253, 246, 227);
        normalLine = QColor(131, 148, 150);
    } else if (themeName == "Solarized Light") {
        background = QColor(253, 246, 227);
        currentLine = QColor(0, 43, 54);
        normalLine = QColor(101, 123, 131);
    } else if (themeName == "Vim Dark") {
        background = QColor(0, 0, 0);
        currentLine = QColor(255, 255, 255);
        normalLine = QColor(192, 192, 192);
    } else if (themeName == "ayu Dark") {
        background = QColor(15, 20, 25);
        currentLine = QColor(230, 225, 207);
        normalLine = QColor(92, 97, 102);
    } else if (themeName == "ayu Light") {
        background = QColor(250, 250, 250);
        currentLine = QColor(92, 97, 102);
        normalLine = QColor(130, 140, 153);
    } else if (themeName == "ayu Mirage") {
        background = QColor(31, 35, 41);
        currentLine = QColor(203, 204, 198);
        normalLine = QColor(92, 97, 102);
    } else if (themeName == "gruvbox Dark") {
        background = QColor(40, 40, 40);  
        currentLine = QColor(251, 241, 199);  
        normalLine = QColor(235, 219, 178);  
    } else if (themeName == "gruvbox Light") {
        background = QColor(251, 241, 199);
        currentLine = QColor(40, 37, 34);
        normalLine = QColor(146, 131, 116);
    } else {

        background = QColor(40, 40, 40);  
        currentLine = QColor(251, 241, 199);  
        normalLine = QColor(235, 219, 178);  
    }

    for (CodeEditor* editor : m_textEditors) {
        if (editor) {
            editor->updateThemeColors();
        }
    }

    if (m_fileTreeWidget) {
        QColor fileTreeBackground = background.darker(105); 
        QColor fileTreeText = normalLine; 
        QColor fileTreeHighlight = currentLine; 

        m_fileTreeWidget->setThemeColors(fileTreeBackground, fileTreeText, fileTreeHighlight);
    }

    QColor lineNumberBg = background.darker(110);  
    QString handleStyle = QString(
        "QSplitter::handle {"
        "    background-color: %1;"  
        "    border: none;"
        "    margin: 0px;"
        "    padding: 0px;"
        "    width: 1px;"  
        "}"
        "QSplitter::handle:hover {"
        "    background-color: %2;"  
        "}"
    ).arg(lineNumberBg.name(), lineNumberBg.lighter(120).name());

    m_mainSplitter->setStyleSheet(handleStyle);
}