#include "editor_window.h"


void EditorWindow::loadTheme(const QString &themeName)
{
    DEBUG_LOG_EDITOR("loadTheme called with theme:" << themeName);
    QString themeFile = QString(":/themes/%1.qss").arg(themeName);
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

    QString themeName = m_luaBridge->getConfigString("theme.name", "gruvbox");
    DEBUG_LOG_EDITOR("applyTheme: Loading theme:" << themeName);
    loadTheme(themeName);
}

void EditorWindow::onLuaThemeChangeRequested(const QString &themeName)
{
    loadTheme(themeName);
}

void EditorWindow::updateEditorThemeColors(const QString &themeName)
{

    QColor background, currentLine, normalLine;

    if (themeName == "gruvbox") {
        background = QColor(40, 37, 34);
        currentLine = QColor(251, 241, 199);
        normalLine = QColor(146, 131, 116);
    } else if (themeName == "dracula") {
        background = QColor(33, 34, 44);
        currentLine = QColor(248, 248, 242);
        normalLine = QColor(98, 114, 164);
    } else if (themeName == "catppuccin-mocha") {
        background = QColor(24, 24, 37);
        currentLine = QColor(205, 214, 244);
        normalLine = QColor(166, 173, 200);
    } else {

        for (CodeEditor* editor : m_textEditors) {
            if (editor) {
                editor->updateThemeColors();
            }
        }

        if (m_fileTreeWidget) {
            m_fileTreeWidget->updateThemeColors();
        }
        return;
    }

    for (CodeEditor* editor : m_textEditors) {
        if (editor) {
            editor->setThemeColors(background, currentLine, normalLine);
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
