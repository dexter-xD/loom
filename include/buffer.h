// manages text content and file operations
// handles file loading, saving, and modification tracking
// provides interface between qt text widget and core logic

#ifndef BUFFER_H
#define BUFFER_H

#include <QString>
#include <QDateTime>
#include <QFileInfo>

class Buffer
{
public:
    Buffer(const QString &filePath = QString());
    ~Buffer();

    bool load(const QString &filePath);
    bool save(const QString &filePath = QString());

    QString content() const;
    void setContent(const QString &content);

    QString filePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified);

    QString fileName() const;
    QDateTime lastModified() const;
    bool exists() const;

    void clear();
    bool isEmpty() const;
    int lineCount() const;

private:
    QString m_filePath;
    QString m_content;
    bool m_modified;
    QDateTime m_lastModified;

    void updateLastModified();
    bool writeToFile(const QString &filePath);
    bool readFromFile(const QString &filePath);
};

#endif 