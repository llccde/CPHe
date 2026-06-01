#include "FileView.h"
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>

// 自定义排序代理，实现：文件夹排前面，同类型按名称字母序升序
class FileSortProxyModel : public QSortFilterProxyModel {
public:
    explicit FileSortProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        // 从源模型获取文件信息
        QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel)
            return QSortFilterProxyModel::lessThan(left, right);

        QFileInfo leftInfo = fsModel->fileInfo(left);
        QFileInfo rightInfo = fsModel->fileInfo(right);

        bool leftIsDir = leftInfo.isDir();
        bool rightIsDir = rightInfo.isDir();

        // 如果一个是目录另一个不是，目录排在前面
        if (leftIsDir != rightIsDir)
            return leftIsDir;  // true表示left<right，即目录在前

        // 同类型（都是文件或都是目录），按名称字典序升序
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
    // 启用排序（会调用我们重写的lessThan）
    m_proxyModel->sort(0, Qt::AscendingOrder);

    // 将代理模型设置给树视图
    ui.treeView->setModel(m_proxyModel);

    // 只显示“名称”列，隐藏其他列（大小、类型、修改日期）
    ui.treeView->header()->setSectionHidden(1, true);
    ui.treeView->header()->setSectionHidden(2, true);
    ui.treeView->header()->setSectionHidden(3, true);

    // 可选美化
    ui.treeView->setAnimated(true);
    ui.treeView->setSortingEnabled(true);  // 允许用户点击表头排序（会走代理）

    // 连接双击信号
    connect(ui.treeView, &QTreeView::doubleClicked,
        this, &FileView::onTreeViewDoubleClicked);
}

FileView::~FileView() {}

void FileView::setRootFolder(const QString& path)
{
    // 通过源模型设置根路径，并获取对应的模型索引
    QModelIndex rootIndex = m_fileSystemModel->setRootPath(path);
    // 将源索引映射到代理模型
    QModelIndex proxyRootIndex = m_proxyModel->mapFromSource(rootIndex);
    // 让树视图以此索引为根
    ui.treeView->setRootIndex(proxyRootIndex);
}

void FileView::onTreeViewDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    // 将代理索引映射回源模型索引
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QFileInfo fileInfo = m_fileSystemModel->fileInfo(sourceIndex);

    // 仅当双击的是文件（非目录）时才发出信号
    if (fileInfo.isFile())
        emit fileDoubleClicked(fileInfo.absoluteFilePath());
}