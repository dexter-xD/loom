#ifndef FILE_TREE_WIDGET_H
#define FILE_TREE_WIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSplitter>
#include <QDir>
#include <QFileInfo>
#include <QModelIndex>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>

class FileTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileTreeWidget(QWidget *parent = nullptr);
    ~FileTreeWidget();

    void setRootPath(const QString &path);
    QString rootPath() const;
    bool isVisible() const;
    void setVisible(bool visible);
    void toggleVisibility();

    void setThemeColors(const QColor &background, const QColor &text, const QColor &highlight);
    void updateThemeColors();

signals:
    void fileOpenRequested(const QString &filePath);
    void visibilityChanged(bool visible);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onItemClicked(const QModelIndex &index);
    void onCustomContextMenuRequested(const QPoint &pos);
    void onOpenFile();
    void onOpenInSystemExplorer();
    void onNewFile();
    void onNewFolder();
    void onRename();
    void onDelete();
    void onRefresh();
    void onCollapseAll();
    void onExpandAll();

private:
    void setupUI();
    void setupContextMenu();
    void updateRootLabel();
    bool isValidProjectPath(const QString &path) const;
    QString getSelectedFilePath() const;
    QModelIndex getSelectedIndex() const;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QLabel *m_rootLabel;
    QPushButton *m_collapseButton;
    QPushButton *m_refreshButton;
    QTreeView *m_treeView;
    QFileSystemModel *m_fileSystemModel;

    QMenu *m_contextMenu;
    QAction *m_openAction;
    QAction *m_openInExplorerAction;
    QAction *m_newFileAction;
    QAction *m_newFolderAction;
    QAction *m_renameAction;
    QAction *m_deleteAction;
    QAction *m_refreshAction;
    QAction *m_collapseAllAction;
    QAction *m_expandAllAction;

    QString m_rootPath;
    bool m_isVisible;

    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_highlightColor;
};

#endif 