// manages text content and file operations
// handles file loading, saving, and modification tracking
// provides interface between qt text widget and core logic

#include "buffer.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "debug_log.h"

Buffer::Buffer(const QString &filePath)
    : m_filePath(filePath)
    , m_modified(false)
{
    if (!filePath.isEmpty()) {
        load(filePath);
    }
}

Buffer::~Buffer()
{
    // no cleanup needed for basic types
}

bool Buffer::load(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }
    
    if (readFromFile(filePath)) {
        m_filePath = filePath;
        m_modified = false;
        updateLastModified();
        return true;
    }
    
    return false;
}

bool Buffer::save(const QString &filePath)
{
    QString targetPath = filePath.isEmpty() ? m_filePath : filePath;
    
    if (targetPath.isEmpty()) {
        return false;
    }
    
    if (writeToFile(targetPath)) {
        m_filePath = targetPath;
        m_modified = false;
        updateLastModified();
        return true;
    }
    
    return false;
}

QString Buffer::content() const
{
    return m_content;
}

void Buffer::setContent(const QString &content)
{
    if (m_content != content) {
        m_content = content;
        m_modified = true;
    }
}

QString Buffer::filePath() const
{
    return m_filePath;
}

void Buffer::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

bool Buffer::isModified() const
{
    return m_modified;
}

void Buffer::setModified(bool modified)
{
    m_modified = modified;
}

QString Buffer::fileName() const
{
    if (m_filePath.isEmpty()) {
        return "Untitled";
    }
    
    QFileInfo fileInfo(m_filePath);
    return fileInfo.fileName();
}

QDateTime Buffer::lastModified() const
{
    return m_lastModified;
}

bool Buffer::exists() const
{
    if (m_filePath.isEmpty()) {
        return false;
    }
    
    return QFile::exists(m_filePath);
}

void Buffer::clear()
{
    m_content.clear();
    m_filePath.clear();
    m_modified = false;
    m_lastModified = QDateTime();
}

bool Buffer::isEmpty() const
{
    return m_content.isEmpty();
}

int Buffer::lineCount() const
{
    if (m_content.isEmpty()) {
        return 0;
    }
    
    return m_content.count('\n') + 1;
}

void Buffer::updateLastModified()
{
    if (!m_filePath.isEmpty() && QFile::exists(m_filePath)) {
        QFileInfo fileInfo(m_filePath);
        m_lastModified = fileInfo.lastModified();
    } else {
        m_lastModified = QDateTime::currentDateTime();
    }
}

bool Buffer::writeToFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file for writing:" << filePath << file.errorString());
        return false;
    }
    
    QTextStream out(&file);
    out << m_content;
    
    return true;
}

bool Buffer::readFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file for reading:" << filePath << file.errorString());
        return false;
    }
    
    QTextStream in(&file);
    m_content = in.readAll();
    
    return true;
}