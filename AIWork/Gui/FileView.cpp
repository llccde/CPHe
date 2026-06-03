#include "FileView.h"
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>

// 自定义排序代理，实现：文件夹排前面，同类型按名称字母序升序
class FileSortProxyModel : public QSortFilterProxyModel {
public:
    explicit FileSortProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel)
            return QSortFilterProxyModel::lessThan(left, right);

        QFileInfo leftInfo = fsModel->fileInfo(left);
        QFileInfo rightInfo = fsModel->fileInfo(right);

        bool leftIsDir = leftInfo.isDir();
        bool rightIsDir = rightInfo.isDir();

        if (leftIsDir != rightIsDir)
            return leftIsDir;

        return leftInfo.fileName().compare(rightInfo.fileName(), Qt::CaseInsensitive) < 0;
    }
};

FileView::FileView(QWidget* parent)
    : QWidget(parent),
    m_fileSystemModel(new QFileSystemModel(this)),
    m_proxyModel(new FileSortProxyModel(this))
{
    setObjectName("File View");
    setWindowTitle("File_View");
    ui.setupUi(this);

    // 配置文件系统模型
    m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    // 设置代理模型的源模型
    m_proxyModel->setSourceModel(m_fileSystemModel);
    m_proxyModel->sort(0, Qt::AscendingOrder);

    // 将代理模型设置给树视图
    ui.treeView->setModel(m_proxyModel);

    // 只显示“名称”列，隐藏其他列
    ui.treeView->header()->setSectionHidden(1, true);
    ui.treeView->header()->setSectionHidden(2, true);
    ui.treeView->header()->setSectionHidden(3, true);

    ui.treeView->setAnimated(true);
    ui.treeView->setSortingEnabled(true);

    // 启用自定义右键菜单
    ui.treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接双击信号
    connect(ui.treeView, &QTreeView::doubleClicked,
        this, &FileView::onTreeViewDoubleClicked);
    // 连接右键菜单信号
    connect(ui.treeView, &QTreeView::customContextMenuRequested,
        this, &FileView::onCustomContextMenu);
}

FileView::~FileView() {}

void FileView::setRootFolder(const QString& path)
{
    QModelIndex rootIndex = m_fileSystemModel->setRootPath(path);
    QModelIndex proxyRootIndex = m_proxyModel->mapFromSource(rootIndex);
    ui.treeView->setRootIndex(proxyRootIndex);
}

void FileView::onTreeViewDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QFileInfo fileInfo = m_fileSystemModel->fileInfo(sourceIndex);

    if (fileInfo.isFile())
        emit fileDoubleClicked(fileInfo.absoluteFilePath());
}

// 辅助函数：从代理索引获取文件信息
QFileInfo FileView::fileInfoFromIndex(const QModelIndex& proxyIndex) const
{
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    return m_fileSystemModel->fileInfo(sourceIndex);
}

void FileView::showError(const QString& message) const
{
    QMessageBox::warning(nullptr, tr("Error"), message);
}

// 右键菜单弹出
void FileView::onCustomContextMenu(const QPoint& pos)
{
    // 获取点击位置的索引
    QModelIndex index = ui.treeView->indexAt(pos);
    // 保存当前右键目标的路径（用于后续操作）
    QString targetPath;
    bool isDir = false;

    if (index.isValid()) {
        QFileInfo info = fileInfoFromIndex(index);
        targetPath = info.absoluteFilePath();
        isDir = info.isDir();
    }
    else {
        // 在空白区域右键：使用根目录
        QModelIndex rootProxy = ui.treeView->rootIndex();
        if (rootProxy.isValid()) {
            QFileInfo info = fileInfoFromIndex(rootProxy);
            targetPath = info.absoluteFilePath();
            isDir = true; // 根目录必定是目录
        }
        else {
            return; // 无有效根目录
        }
    }

    // 构建右键菜单
    QMenu menu(this);

    QAction* newFileAction = menu.addAction(tr("New File"));
    QAction* renameAction = nullptr;
    QAction* deleteAction = nullptr;

    // 只有在有效项上右键时才显示重命名和删除（且需要存在实际文件）
    if (index.isValid()) {
        menu.addSeparator();
        renameAction = menu.addAction(tr("Rename"));
        deleteAction = menu.addAction(tr("Delete"));
    }

    // 显示菜单并等待选择
    QAction* selected = menu.exec(ui.treeView->viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    // 处理动作，使用捕获的 targetPath 和 isDir
    if (selected == newFileAction) {
        // 新文件所在目录：如果右键目标为目录，则为该目录；否则为其父目录
        QString parentDir = isDir ? targetPath : QFileInfo(targetPath).dir().absolutePath();

        bool ok;
        QString newName = QInputDialog::getText(this, tr("New File"),
            tr("File name:"), QLineEdit::Normal,
            QString(), &ok);
        if (ok && !newName.isEmpty()) {
            QFile newFile(parentDir + QDir::separator() + newName);
            if (newFile.exists()) {
                showError(tr("A file with that name already exists."));
            }
            else {
                if (newFile.open(QIODevice::WriteOnly)) {
                    newFile.close();
                }
                else {
                    showError(tr("Could not create file."));
                }
            }
        }
    }
    else if (selected == renameAction && renameAction) {
        // 重命名操作
        QString oldName = QFileInfo(targetPath).fileName();
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename"),
            tr("New name:"), QLineEdit::Normal,
            oldName, &ok);
        if (ok && !newName.isEmpty() && newName != oldName) {
            QDir dir = QFileInfo(targetPath).dir();
            QString newPath = dir.absoluteFilePath(newName);
            if (QFile::exists(newPath)) {
                showError(tr("A file or directory with that name already exists."));
            }
            else {
                if (!QFile::rename(targetPath, newPath)) {
                    showError(tr("Rename failed."));
                }
            }
        }
    }
    else if (selected == deleteAction && deleteAction) {
        // 删除操作：确认对话框
        QString message = isDir ? tr("Are you sure you want to delete the directory?\n\"%1\"")
            .arg(QFileInfo(targetPath).fileName())
            : tr("Are you sure you want to delete the file?\n\"%1\"")
            .arg(QFileInfo(targetPath).fileName());
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Confirm Delete"), message,
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            bool success = false;
            if (isDir) {
                // 删除目录（包括非空目录）
                QDir targetDir(targetPath);
                success = targetDir.removeRecursively();
            }
            else {
                QFile targetFile(targetPath);
                success = targetFile.remove();
            }
            if (!success) {
                showError(tr("Delete failed."));
            }
        }
    }
}