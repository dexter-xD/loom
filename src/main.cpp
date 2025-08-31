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
    // create qt application instance
    QApplication app(argc, argv);
    
    // load gruvbox theme stylesheet from resources
    QFile styleFile(":/themes/gruvbox.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }
    
    // set application metadata for cross-platform compatibility
    app.setApplicationName("Loom");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Loom");
    
    // set application icon
    app.setWindowIcon(QIcon(":/assets/icon.png"));
    
    // setup command line argument parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("A lightweight, cross-platform text editor with Lua scripting support");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // add positional argument for file to open
    parser.addPositionalArgument("file", "File to open on startup", "[file]");
    
    // process command line arguments with error handling
    try {
        parser.process(app);
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Command Line Error", 
                            QString("Failed to parse command line arguments: %1").arg(e.what()));
        return 1;
    }
    
    // create main editor window
    EditorWindow window;
    
    // handle file opening from command line arguments
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        const QString filePath = args.first();
        if (QFile::exists(filePath)) {
            window.openFile(filePath);
        } else {
            QMessageBox::warning(&window, "File Not Found", 
                               QString("Could not find file: %1").arg(filePath));
            // if file doesn't exist, create an untitled tab
            window.ensureAtLeastOneTab();
        }
    } else {
        // no command line arguments, create an untitled tab
        window.ensureAtLeastOneTab();
    }
    
    // show main window
    window.show();
    
    // start qt event loop
    return app.exec();
}