#include "editor_window.h"

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_statusBar(nullptr)
    , m_luaBridge(nullptr)
{

    m_luaBridge = new LuaBridge(this);
    if (!m_luaBridge->initialize()) {
        LOG_ERROR("Failed to initialize Lua bridge:" << m_luaBridge->lastError());
    }

    m_pluginManager = new PluginManager(m_luaBridge, this);

    m_luaBridge->setPluginManager(m_pluginManager);

    connect(m_pluginManager, &PluginManager::pluginLoaded,
            this, [this](const QString &pluginName) {
                m_statusBar->showMessage(QString("Plugin loaded: %1").arg(pluginName), 3000);
            });
    connect(m_pluginManager, &PluginManager::pluginError,
            this, [this](const QString &pluginName, const QString &error) {
                m_statusBar->showMessage(QString("Plugin error (%1): %2").arg(pluginName, error), 5000);
            });

    setupUI();
    setupStatusBar();
    connectSignals();

    loadConfiguration();

    applyConfiguration();
    setupKeybindings();

    setupSyntaxHighlighting();

    loadPlugins();

    setupMenus();

    updateLuaEditorState();

    setWindowTitle("Loom");

    setMinimumSize(400, 300);
}

EditorWindow::~EditorWindow()
{

    for (auto shortcut : m_shortcuts) {
        delete shortcut;
    }
    m_shortcuts.clear();

    for (Buffer* buffer : m_buffers) {
        delete buffer;
    }
    m_buffers.clear();

    m_textEditors.clear();

    delete m_pluginManager;
    delete m_luaBridge;
}