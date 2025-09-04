// file tree widget for displaying project directory structure
// provides navigation and file opening functionality
// supports toggling visibility and keyboard shortcuts

#include "file_tree_widget.h"
#include "debug_log.h"
#include <QApplication>
#include <QStandardPaths>
#include <QSpacerItem>

FileTreeWidget::FileTreeWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_rootLabel(nullptr)
    , m_collapseButton(nullptr)
    , m_refreshButton(nullptr)
    , m_treeView(nullptr)
    , m_fileSystemModel(nullptr)
    , m_contextMenu(nullptr)
    , m_openAction(nullptr)
    , m_openInExplorerAction(nullptr)
    , m_newFileAction(nullptr)
    , m_newFolderAction(nullptr)
    , m_renameAction(nullptr)
    , m_deleteAction(nullptr)
    , m_refreshAction(nullptr)
    , m_collapseAllAction(nullptr)
    , m_expandAllAction(nullptr)
    , m_isVisible(true)
    , m_backgroundColor(QColor(40, 37, 34))
    , m_textColor(QColor(146, 131, 116))
    , m_highlightColor(QColor(251, 241, 199))
{
    setupUI();
    setupContextMenu();

    setRootPath(QDir::currentPath());
}

FileTreeWidget::~FileTreeWidget()
{

}

void FileTreeWidget::setupUI()
{

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setMargin(0);

    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setContentsMargins(0, 4, 8, 4);  
    m_headerLayout->setSpacing(4);

    m_headerLayout->addSpacing(16);

    m_rootLabel = new QLabel("Project");
    m_rootLabel->setStyleSheet(
        "font-weight: 600; "
        "font-size: 11px; "  
        "color: #d4d4d4; "
        "padding: 3px 0px 3px 0px; "  
        "background: transparent; "
        "border: none; "
        "letter-spacing: 0.3px;"
    );
    m_rootLabel->setWordWrap(true);
    m_headerLayout->addWidget(m_rootLabel);

    m_headerLayout->addStretch();

    m_collapseButton = new QPushButton("⊟");
    m_collapseButton->setFixedSize(24, 24);
    m_collapseButton->setToolTip("Collapse All");
    connect(m_collapseButton, &QPushButton::clicked, this, &FileTreeWidget::onCollapseAll);
    m_headerLayout->addWidget(m_collapseButton);

    m_refreshButton = new QPushButton("⟳");
    m_refreshButton->setFixedSize(24, 24);
    m_refreshButton->setToolTip("Refresh");
    connect(m_refreshButton, &QPushButton::clicked, this, &FileTreeWidget::onRefresh);
    m_headerLayout->addWidget(m_refreshButton);

    QWidget *headerWidget = new QWidget();
    headerWidget->setLayout(m_headerLayout);
    headerWidget->setFixedHeight(30);  

    m_mainLayout->addWidget(headerWidget);

    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setRootPath("");

    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_fileSystemModel);
    m_treeView->setRootIndex(m_fileSystemModel->index(QDir::currentPath()));

    m_treeView->hideColumn(1); 
    m_treeView->hideColumn(2); 
    m_treeView->hideColumn(3); 

    m_treeView->setHeaderHidden(true);
    m_treeView->setIndentation(8);  
    m_treeView->setAnimated(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_treeView->setContentsMargins(0, 0, 0, 0);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FileTreeWidget::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::clicked, this, &FileTreeWidget::onItemClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileTreeWidget::onCustomContextMenuRequested);

    m_mainLayout->addWidget(m_treeView);

    setMinimumWidth(200);
    setMaximumWidth(400);
    resize(250, 400);

    setContentsMargins(0, 0, 0, 0);
}

void FileTreeWidget::setupContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_openAction = new QAction("Open", this);
    connect(m_openAction, &QAction::triggered, this, &FileTreeWidget::onOpenFile);
    m_contextMenu->addAction(m_openAction);

    m_openInExplorerAction = new QAction("Open in File Manager", this);
    connect(m_openInExplorerAction, &QAction::triggered, this, &FileTreeWidget::onOpenInSystemExplorer);
    m_contextMenu->addAction(m_openInExplorerAction);

    m_contextMenu->addSeparator();

    m_newFileAction = new QAction("New File", this);
    connect(m_newFileAction, &QAction::triggered, this, &FileTreeWidget::onNewFile);
    m_contextMenu->addAction(m_newFileAction);

    m_newFolderAction = new QAction("New Folder", this);
    connect(m_newFolderAction, &QAction::triggered, this, &FileTreeWidget::onNewFolder);
    m_contextMenu->addAction(m_newFolderAction);

    m_contextMenu->addSeparator();

    m_renameAction = new QAction("Rename", this);
    connect(m_renameAction, &QAction::triggered, this, &FileTreeWidget::onRename);
    m_contextMenu->addAction(m_renameAction);

    m_deleteAction = new QAction("Delete", this);
    connect(m_deleteAction, &QAction::triggered, this, &FileTreeWidget::onDelete);
    m_contextMenu->addAction(m_deleteAction);

    m_contextMenu->addSeparator();

    m_refreshAction = new QAction("Refresh", this);
    connect(m_refreshAction, &QAction::triggered, this, &FileTreeWidget::onRefresh);
    m_contextMenu->addAction(m_refreshAction);

    m_collapseAllAction = new QAction("Collapse All", this);
    connect(m_collapseAllAction, &QAction::triggered, this, &FileTreeWidget::onCollapseAll);
    m_contextMenu->addAction(m_collapseAllAction);

    m_expandAllAction = new QAction("Expand All", this);
    connect(m_expandAllAction, &QAction::triggered, this, &FileTreeWidget::onExpandAll);
    m_contextMenu->addAction(m_expandAllAction);
}

void FileTreeWidget::setRootPath(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists()) {
        DEBUG_LOG_EDITOR("Invalid root path:" << path);
        return;
    }

    m_rootPath = QDir(path).absolutePath();

    if (m_fileSystemModel && m_treeView) {
        QModelIndex rootIndex = m_fileSystemModel->setRootPath(m_rootPath);
        m_treeView->setRootIndex(rootIndex);

        m_treeView->expand(rootIndex);
    }

    updateRootLabel();
    DEBUG_LOG_EDITOR("File tree root path set to:" << m_rootPath);
}

QString FileTreeWidget::rootPath() const
{
    return m_rootPath;
}

bool FileTreeWidget::isVisible() const
{
    return m_isVisible && QWidget::isVisible();
}

void FileTreeWidget::setVisible(bool visible)
{
    m_isVisible = visible;
    QWidget::setVisible(visible);
    emit visibilityChanged(visible);
}

void FileTreeWidget::toggleVisibility()
{
    setVisible(!isVisible());
}

void FileTreeWidget::updateRootLabel()
{
    if (!m_rootLabel) return;

    QDir dir(m_rootPath);
    QString displayName = dir.dirName();
    if (displayName.isEmpty()) {
        displayName = m_rootPath;
    }

    m_rootLabel->setText(displayName);
    m_rootLabel->setToolTip(m_rootPath);
}

bool FileTreeWidget::isValidProjectPath(const QString &path) const
{
    QDir dir(path);
    return dir.exists() && dir.isReadable();
}

QString FileTreeWidget::getSelectedFilePath() const
{
    QModelIndex index = getSelectedIndex();
    if (index.isValid()) {
        return m_fileSystemModel->filePath(index);
    }
    return QString();
}

QModelIndex FileTreeWidget::getSelectedIndex() const
{
    if (!m_treeView) return QModelIndex();

    QModelIndexList selected = m_treeView->selectionModel()->selectedIndexes();
    if (!selected.isEmpty()) {
        return selected.first();
    }
    return QModelIndex();
}

void FileTreeWidget::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QString filePath = m_fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);

    if (fileInfo.isFile()) {
        DEBUG_LOG_EDITOR("File tree: Opening file:" << filePath);
        emit fileOpenRequested(filePath);
    } else if (fileInfo.isDir()) {

        if (m_treeView->isExpanded(index)) {
            m_treeView->collapse(index);
        } else {
            m_treeView->expand(index);
        }
    }
}

void FileTreeWidget::onItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QString filePath = m_fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);

    if (fileInfo.isFile()) {

        DEBUG_LOG_EDITOR("File tree: Opening file (single click):" << filePath);
        emit fileOpenRequested(filePath);
    } else if (fileInfo.isDir()) {

        if (m_treeView->isExpanded(index)) {
            m_treeView->collapse(index);
        } else {
            m_treeView->expand(index);
        }
    }
}

void FileTreeWidget::onCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = m_treeView->indexAt(pos);

    if (index.isValid()) {
        QString filePath = m_fileSystemModel->filePath(index);
        QFileInfo fileInfo(filePath);

        m_openAction->setEnabled(fileInfo.isFile());
        m_renameAction->setEnabled(true);
        m_deleteAction->setEnabled(true);
    } else {

        m_openAction->setEnabled(false);
        m_renameAction->setEnabled(false);
        m_deleteAction->setEnabled(false);
    }

    m_contextMenu->exec(m_treeView->mapToGlobal(pos));
}

void FileTreeWidget::onOpenFile()
{
    QString filePath = getSelectedFilePath();
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile()) {
            emit fileOpenRequested(filePath);
        }
    }
}

void FileTreeWidget::onOpenInSystemExplorer()
{
    QString filePath = getSelectedFilePath();
    if (!filePath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
    }
}

void FileTreeWidget::onNewFile()
{
    QString selectedPath = getSelectedFilePath();
    QString parentDir;

    if (selectedPath.isEmpty()) {
        parentDir = m_rootPath;
    } else {
        QFileInfo fileInfo(selectedPath);
        parentDir = fileInfo.isDir() ? selectedPath : fileInfo.absolutePath();
    }

    bool ok;
    QString fileName = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);

    if (ok && !fileName.isEmpty()) {
        QString fullPath = QDir(parentDir).absoluteFilePath(fileName);

        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            DEBUG_LOG_EDITOR("Created new file:" << fullPath);

            m_treeView->update();

            emit fileOpenRequested(fullPath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to create file: " + fileName);
        }
    }
}

void FileTreeWidget::onNewFolder()
{
    QString selectedPath = getSelectedFilePath();
    QString parentDir;

    if (selectedPath.isEmpty()) {
        parentDir = m_rootPath;
    } else {
        QFileInfo fileInfo(selectedPath);
        parentDir = fileInfo.isDir() ? selectedPath : fileInfo.absolutePath();
    }

    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);

    if (ok && !folderName.isEmpty()) {
        QString fullPath = QDir(parentDir).absoluteFilePath(folderName);

        if (QDir().mkpath(fullPath)) {
            DEBUG_LOG_EDITOR("Created new folder:" << fullPath);
            m_treeView->update();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create folder: " + folderName);
        }
    }
}

void FileTreeWidget::onRename()
{
    QString filePath = getSelectedFilePath();
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QString currentName = fileInfo.fileName();

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        QString newPath = QDir(fileInfo.absolutePath()).absoluteFilePath(newName);

        if (QFile::rename(filePath, newPath)) {
            DEBUG_LOG_EDITOR("Renamed" << filePath << "to" << newPath);
            m_treeView->update();
        } else {
            QMessageBox::warning(this, "Error", "Failed to rename: " + currentName);
        }
    }
}

void FileTreeWidget::onDelete()
{
    QString filePath = getSelectedFilePath();
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QString itemName = fileInfo.fileName();
    QString itemType = fileInfo.isDir() ? "folder" : "file";

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete " + itemType,
        QString("Are you sure you want to delete the %1 '%2'?").arg(itemType, itemName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool success = false;

        if (fileInfo.isDir()) {
            success = QDir(filePath).removeRecursively();
        } else {
            success = QFile::remove(filePath);
        }

        if (success) {
            DEBUG_LOG_EDITOR("Deleted" << itemType << ":" << filePath);
            m_treeView->update();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete " + itemType + ": " + itemName);
        }
    }
}

void FileTreeWidget::onRefresh()
{
    if (m_treeView) {
        m_treeView->update();
        DEBUG_LOG_EDITOR("File tree refreshed");
    }
}

void FileTreeWidget::onCollapseAll()
{
    if (m_treeView) {
        m_treeView->collapseAll();

        m_treeView->expand(m_treeView->rootIndex());
    }
}

void FileTreeWidget::onExpandAll()
{
    if (m_treeView) {
        m_treeView->expandAll();
    }
}

void FileTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{

    if (m_contextMenu) {

        m_openAction->setEnabled(false);
        m_renameAction->setEnabled(false);
        m_deleteAction->setEnabled(false);

        m_contextMenu->exec(event->globalPos());
    }
}

void FileTreeWidget::setThemeColors(const QColor &background, const QColor &text, const QColor &highlight)
{
    m_backgroundColor = background;
    m_textColor = text;
    m_highlightColor = highlight;

    QString widgetStyle = QString(
        "FileTreeWidget {"
        "    background-color: %1;"
        "    border: none;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
    ).arg(m_backgroundColor.name());

    setStyleSheet(widgetStyle);

    if (m_rootLabel) {
        m_rootLabel->setStyleSheet(QString(
            "QLabel {"
            "    color: %1;"
            "    font-weight: 600;"
            "    font-size: 13px;"  
            "    padding: 2px 0px;"  
            "    background-color: %2;"
            "    border: none;"
            "    letter-spacing: 0.3px;"
            "}"
        ).arg(m_textColor.name(), m_backgroundColor.name()));
    }

    if (m_headerLayout) {
        QWidget *headerWidget = m_headerLayout->parentWidget();
        if (headerWidget) {
            headerWidget->setStyleSheet(QString(
                "QWidget {"
                "    background-color: %1;"
                "    border: none;"
                "    margin: 0px;"
                "    padding: 0px;"
                "}"
            ).arg(m_backgroundColor.name()));
        }
    }

    if (m_treeView) {
        QString style = QString(
            "QTreeView {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: none;"
            "    padding: 0px;"
            "    font-size: 13px;"
            "    font-family: 'JetBrains Mono', 'Consolas', 'Monaco', monospace;"
            "    selection-background-color: transparent;"
            "    alternate-background-color: transparent;"
            "    outline: none;"
            "    margin: 0px;"
            "}"
            "QTreeView::item {"
            "    padding: 3px 8px;"  
            "    margin: 1px 0px;"
            "    border-radius: 2px;"
            "    border: none;"
            "    color: %2;"
            "}"
            "QTreeView::item:selected {"
            "    background-color: %4;"
            "    color: %5;"
            "    border-radius: 2px;"
            "}"
            "QTreeView::item:hover {"
            "    background-color: %6;"
            "    border-radius: 2px;"
            "}"
            "QTreeView::item:selected:hover {"
            "    background-color: %4;"
            "}"
            "QTreeView::branch {"
            "    background: transparent;"
            "    margin: 1px;"  
            "}"
            "QTreeView::branch:has-children:!has-siblings:closed,"
            "QTreeView::branch:closed:has-children:has-siblings {"
            "    border-image: none;"
            "    image: none;"
            "}"
            "QTreeView::branch:open:has-children:!has-siblings,"
            "QTreeView::branch:open:has-children:has-siblings {"
            "    border-image: none;"
            "    image: none;"
            "}"
            "QScrollBar:vertical {"
            "    border: none;"
            "    background-color: transparent;"
            "    width: 6px;"  
            "    border-radius: 0px;"
            "    margin: 0px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    background-color: %8;"  
            "    border-radius: 0px;"
            "    min-height: 20px;"
            "    margin: 1px;"
            "}"
            "QScrollBar::handle:vertical:pressed {"
            "    background-color: %9;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "    border: none;"
            "    background: none;"
            "    height: 0px;"
            "}"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
            "    background: none;"
            "}"
            "QScrollBar:horizontal {"
            "    border: none;"
            "    background-color: transparent;"
            "    height: 8px;"  
            "    border-radius: 0px;"  
            "    margin: 0px;"
            "}"
            "QScrollBar::handle:horizontal {"
            "    background-color: %7;"  
            "    border-radius: 0px;"  
            "    min-width: 15px;"
            "    margin: 0px;"
            "}"
            "QScrollBar::handle:horizontal:hover {"
            "    background-color: %8;"  
            "}"
            "QScrollBar::handle:horizontal:pressed {"
            "    background-color: %9;"
            "}"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
            "    border: none;"
            "    background: none;"
            "    width: 0px;"
            "}"
            "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
            "    background: none;"
            "}"
        ).arg(
            m_backgroundColor.name(),                    
            m_textColor.name(),                         
            m_backgroundColor.darker(120).name(),       
            m_highlightColor.name(),                    
            m_backgroundColor.name(),                   
            m_highlightColor.lighter(160).name(),       
            m_textColor.lighter(130).name() + "80",     
            m_textColor.lighter(130).name(),            
            m_highlightColor.name(),                    
            m_backgroundColor.name(),                   
            m_textColor.name(),                         
            m_highlightColor.name()                     
        );

        m_treeView->setStyleSheet(style);
    }

    if (m_collapseButton && m_refreshButton) {
        QString buttonStyle = QString(
            "QPushButton {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid transparent;"
            "    border-radius: 2px;"  
            "    padding: 2px;"       
            "    font-size: 14px;"    
            "    font-weight: normal;"
            "    min-width: 16px;"    
            "    min-height: 16px;"   
            "    max-width: 24px;"    
            "    max-height: 24px;"   
            "}"
            "QPushButton:hover {"
            "    background-color: %3;"
            "    border: 1px solid %4;"
            "}"
            "QPushButton:pressed {"
            "    background-color: %4;"
            "    border: 1px solid %5;"
            "}"
        ).arg(
            m_backgroundColor.name(),                    
            m_textColor.name(),                         
            m_highlightColor.lighter(170).name(),       
            m_highlightColor.lighter(140).name(),       
            m_highlightColor.name()                     
        );

        m_collapseButton->setStyleSheet(buttonStyle);
        m_refreshButton->setStyleSheet(buttonStyle);
    }

    update();
}

void FileTreeWidget::updateThemeColors()
{

    QPalette palette = this->palette();
    QColor baseColor = palette.color(QPalette::Base);
    QColor textColor = palette.color(QPalette::Text);
    QColor highlightColor = palette.color(QPalette::Highlight);

    setThemeColors(baseColor, textColor, highlightColor);
}