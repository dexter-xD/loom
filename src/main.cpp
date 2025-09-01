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

    parser.addPositionalArgument("file", "File to open on startup", "[file]");

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
        const QString filePath = args.first();
        if (QFile::exists(filePath)) {
            window.openFile(filePath);
        } else {
            QMessageBox::warning(&window, "File Not Found", 
                               QString("Could not find file: %1").arg(filePath));

            window.ensureAtLeastOneTab();
        }
    } else {

        window.ensureAtLeastOneTab();
    }

    window.show();

    return app.exec();
}