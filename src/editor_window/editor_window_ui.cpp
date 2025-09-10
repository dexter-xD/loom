#include "editor_window.h"

void EditorWindow::setupUI()
{

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    m_fileTreeWidget = new FileTreeWidget(this);
    m_fileTreeWidget->setVisible(false); 

    m_tabWidget = new NoMnemonicTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabBarAutoHide(false);

    QTabBar* tabBar = m_tabWidget->tabBar();
    tabBar->setUsesScrollButtons(false);
    tabBar->setElideMode(Qt::ElideRight);

    m_tabWidget->setProperty("focusPolicy", Qt::NoFocus);

    tabBar->setProperty("_q_paintWithoutMnemonics", true);
    tabBar->setProperty("textFormat", Qt::PlainText);
    m_tabWidget->setProperty("textFormat", Qt::PlainText);

    tabBar->setFocusPolicy(Qt::NoFocus);

    m_mainSplitter->addWidget(m_fileTreeWidget);
    m_mainSplitter->addWidget(m_tabWidget);

    m_mainSplitter->setStretchFactor(0, 0); 
    m_mainSplitter->setStretchFactor(1, 1); 
    m_mainSplitter->setSizes({200, 800}); 

    QColor lineNumberBg = QColor(40, 37, 34).darker(110);  
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
    m_mainSplitter->setHandleWidth(1);  
}

void EditorWindow::setupStatusBar()
{

    m_statusBar = statusBar();
    m_statusBar->showMessage("Ready");
}

void EditorWindow::connectSignals()
{

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &EditorWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &EditorWindow::onTabCloseRequested);

    if (m_fileTreeWidget) {
        connect(m_fileTreeWidget, &FileTreeWidget::fileOpenRequested,
                this, &EditorWindow::onFileTreeFileOpenRequested);
        connect(m_fileTreeWidget, &FileTreeWidget::visibilityChanged,
                this, &EditorWindow::onFileTreeVisibilityChanged);
    }

    if (m_luaBridge) {
        connect(m_luaBridge, &LuaBridge::fileOpenRequested,
                this, &EditorWindow::onLuaFileOpenRequested);
        connect(m_luaBridge, &LuaBridge::fileSaveRequested,
                this, &EditorWindow::onLuaFileSaveRequested);
        connect(m_luaBridge, &LuaBridge::textChangeRequested,
                this, &EditorWindow::onLuaTextChangeRequested);
        connect(m_luaBridge, &LuaBridge::cursorMoveRequested,
                this, &EditorWindow::onLuaCursorMoveRequested);
        connect(m_luaBridge, &LuaBridge::statusMessageRequested,
                this, &EditorWindow::onLuaStatusMessageRequested);
        connect(m_luaBridge, &LuaBridge::themeChangeRequested,
                this, &EditorWindow::onLuaThemeChangeRequested);
    }
}