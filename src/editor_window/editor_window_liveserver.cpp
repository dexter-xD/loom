#include "editor_window.h"
#include <QCoreApplication>
#include <QRegularExpression>

void EditorWindow::toggleLiveServer()
{
    if (m_liveServerRunning) {
        stopLiveServer();
    } else {
        startLiveServer();
    }
}

void EditorWindow::startLiveServer()
{
    if (m_liveServerRunning) {
        m_statusBar->showMessage("Live server is already running", 2000);
        return;
    }

    if (m_luaBridge) {
        QString configPort = m_luaBridge->getConfigString("live_server.port", "8080");
        QString configDir = m_luaBridge->getConfigString("live_server.default_directory", "");

        if (!configPort.isEmpty()) {
            m_liveServerPort = configPort;
        }
        if (!configDir.isEmpty()) {
            m_liveServerDir = configDir;
        }
    }

    QString serverDir = m_liveServerDir;

    if (serverDir.isEmpty()) {
        Buffer* currentBuffer = getCurrentBuffer();
        if (currentBuffer && !currentBuffer->filePath().isEmpty()) {
            QFileInfo fileInfo(currentBuffer->filePath());
            serverDir = fileInfo.absolutePath();
        }
    }

    if (serverDir.isEmpty()) {
        serverDir = QDir::currentPath();
    }

    QString liveServerPath = QCoreApplication::applicationDirPath() + "/live-server-bin";

    if (m_luaBridge) {
        QString customPath = m_luaBridge->getConfigString("live_server.executable_path", "");
        if (!customPath.isEmpty()) {
            liveServerPath = customPath;
        }
    }

    if (!QFile::exists(liveServerPath)) {
        m_statusBar->showMessage(QString("Live server executable not found at: %1. Make sure both loom and live-server are built together.").arg(liveServerPath), 5000);
        return;
    }

    QStringList arguments;
    arguments << "-d" << serverDir;
    arguments << "-p" << m_liveServerPort;

    QString logLevel = "quiet"; 
    if (m_luaBridge) {
        logLevel = m_luaBridge->getConfigString("live_server.log_level", "quiet");
    }

    if (logLevel == "quiet") {
        arguments << "-q";  
    } else if (logLevel == "verbose") {
        arguments << "-v";  
    }

    m_liveServerProcess->start(liveServerPath, arguments);

    if (!m_liveServerProcess->waitForStarted(3000)) {
        m_statusBar->showMessage("Failed to start live server", 3000);
        return;
    }

    m_liveServerRunning = true;
    m_statusBar->showMessage(QString("Live server started on port %1 serving %2").arg(m_liveServerPort, serverDir), 3000);

    if (m_luaBridge) {
        bool autoOpenBrowser = m_luaBridge->getConfigBool("live_server.auto_open_browser", false);
        if (autoOpenBrowser) {

            QTimer::singleShot(1000, this, &EditorWindow::openInBrowser);
        }
    }

    refreshToolsMenu();

    DEBUG_LOG_EDITOR("Live server started: " << liveServerPath << " -d " << serverDir << " -p " << m_liveServerPort << " (log level: " << logLevel << ")");
}

void EditorWindow::stopLiveServer()
{
    if (!m_liveServerRunning || !m_liveServerProcess) {
        m_statusBar->showMessage("Live server is not running", 2000);
        return;
    }

    m_liveServerProcess->terminate();

    if (!m_liveServerProcess->waitForFinished(3000)) {
        m_liveServerProcess->kill();
        m_liveServerProcess->waitForFinished(1000);
    }

    m_liveServerRunning = false;
    m_statusBar->showMessage("Live server stopped", 2000);

    refreshToolsMenu();

    DEBUG_LOG_EDITOR("Live server stopped");
}

void EditorWindow::onLiveServerFinished(int , QProcess::ExitStatus exitStatus)
{
    m_liveServerRunning = false;

    if (exitStatus == QProcess::CrashExit) {
        m_statusBar->showMessage("Live server crashed", 3000);
        DEBUG_LOG_EDITOR("Live server crashed with exit code: " << exitCode);
    } else {
        m_statusBar->showMessage("Live server stopped", 2000);
        DEBUG_LOG_EDITOR("Live server finished with exit code: " << exitCode);
    }

    refreshToolsMenu();
}

void EditorWindow::onLiveServerError(QProcess::ProcessError error)
{
    m_liveServerRunning = false;

    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Live server failed to start";
            break;
        case QProcess::Crashed:
            errorMessage = "Live server crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "Live server timed out";
            break;
        case QProcess::WriteError:
            errorMessage = "Live server write error";
            break;
        case QProcess::ReadError:
            errorMessage = "Live server read error";
            break;
        default:
            errorMessage = "Live server unknown error";
            break;
    }

    m_statusBar->showMessage(errorMessage, 3000);
    DEBUG_LOG_EDITOR("Live server error: " << errorMessage);

    refreshToolsMenu();
}

QString EditorWindow::cleanLiveServerOutput(const QString& rawOutput)
{
    QString cleaned = rawOutput;

    static QRegularExpression ansiRegex("\\x1B\\[[0-9;]*[a-zA-Z]|\\u001B\\[[0-9;]*[a-zA-Z]");
    cleaned.remove(ansiRegex);

    cleaned.replace("\r\n", "\n");
    cleaned.replace("\r", "\n");

    QStringList lines = cleaned.split('\n', Qt::SkipEmptyParts);
    QStringList processedLines;

    for (const QString& line : lines) {
        QString processedLine = line.trimmed();

        static QRegularExpression hexPattern("^[a-f0-9]{6}");
        if (processedLine.contains("mongoose.c:") || 
            (processedLine.length() > 6 && 
             processedLine.at(6) == ' ' && 
             processedLine.contains(" 3 ") && 
             hexPattern.match(processedLine).hasMatch())) {
            continue;
        }

        if (!processedLine.isEmpty()) {
            processedLines.append(processedLine);
        }
    }

    return processedLines.join('\n');
}

void EditorWindow::onLiveServerStandardOutput()
{
    if (!m_liveServerProcess) {
        return;
    }

    QByteArray data = m_liveServerProcess->readAllStandardOutput();
    QString rawOutput = QString::fromUtf8(data);

    if (!rawOutput.isEmpty()) {

        QString logLevel = "quiet";
        if (m_luaBridge) {
            logLevel = m_luaBridge->getConfigString("live_server.log_level", "quiet");
        }

        if (logLevel != "quiet") {

            QString cleanOutput = cleanLiveServerOutput(rawOutput);

            if (!cleanOutput.isEmpty()) {

                DEBUG_LOG_EDITOR("Live server: " << cleanOutput);

                bool showInConsole = true;
                if (m_luaBridge) {
                    showInConsole = m_luaBridge->getConfigBool("live_server.show_output_in_console", true);
                }

                if (showInConsole) {

                    QStringList lines = cleanOutput.split('\n', Qt::SkipEmptyParts);
                    for (const QString& line : lines) {
                        qDebug() << "[Live Server]" << line;
                    }
                }

                if (cleanOutput.contains("Server started") || cleanOutput.contains("localhost") || 
                    cleanOutput.contains("Hot reload")) {

                    QStringList lines = cleanOutput.split('\n', Qt::SkipEmptyParts);
                    for (const QString& line : lines) {
                        if (line.contains("Server started") || line.contains("localhost")) {
                            m_statusBar->showMessage(line, 4000);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void EditorWindow::onLiveServerStandardError()
{
    if (!m_liveServerProcess) {
        return;
    }

    QByteArray data = m_liveServerProcess->readAllStandardError();
    QString rawOutput = QString::fromUtf8(data);

    if (!rawOutput.isEmpty()) {

        QString cleanOutput = cleanLiveServerOutput(rawOutput);

        if (!cleanOutput.isEmpty()) {

            DEBUG_LOG_EDITOR("Live server error: " << cleanOutput);

            bool showInConsole = true;
            if (m_luaBridge) {
                showInConsole = m_luaBridge->getConfigBool("live_server.show_output_in_console", true);
            }

            if (showInConsole) {
                QStringList lines = cleanOutput.split('\n', Qt::SkipEmptyParts);
                for (const QString& line : lines) {
                    qWarning() << "[Live Server Error]" << line;
                }
            }

            QStringList lines = cleanOutput.split('\n', Qt::SkipEmptyParts);
            if (!lines.isEmpty()) {
                m_statusBar->showMessage(QString("Live server: %1").arg(lines.first()), 5000);
            }
        }
    }
}

void EditorWindow::openInBrowser()
{
    if (!m_liveServerRunning) {
        m_statusBar->showMessage("Live server is not running", 2000);
        return;
    }

    QString url = QString("http://localhost:%1").arg(m_liveServerPort);
    QDesktopServices::openUrl(QUrl(url));
    m_statusBar->showMessage(QString("Opening %1 in browser").arg(url), 2000);
}