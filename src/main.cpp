// entry point
// initializes qt application and creates main window
// handles command line arguments and application lifecycle

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QIcon>
#include "editor_window.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    QFile styleFile(":/themes/gruvbox.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    app.setApplicationName("Loom");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Loom");

    app.setWindowIcon(QIcon(":/assets/icon.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("A lightweight, cross-platform text editor with Lua scripting support");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("path", "File or project directory to open on startup", "[file|directory]");

    try {
        parser.process(app);
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Command Line Error", 
                            QString("Failed to parse command line arguments: %1").arg(e.what()));
        return 1;
    }

    EditorWindow window;

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        const QString path = args.first();
        QFileInfo pathInfo(path);
        
        if (pathInfo.exists()) {
            if (pathInfo.isFile()) {
                window.openFile(path);
            } else if (pathInfo.isDir()) {
                window.openProject(path);
            }
        } else {
            QMessageBox::warning(&window, "Path Not Found", 
                               QString("Could not find file or directory: %1").arg(path));
            window.ensureAtLeastOneTab();
        }
    } else {
        window.ensureAtLeastOneTab();
    }

    window.show();

    return app.exec();
}
