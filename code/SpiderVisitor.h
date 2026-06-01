#pragma once

#include <QVector>
#include <QString>
#include <QVariant>
#include <memory>
#include <functional>
#include <clang-c/Index.h>

// 前向声明
class RunTimeErrorCollector;

// ---------- 语法树节点 ----------
class TagTreeNode {
public:
    QVector<std::unique_ptr<TagTreeNode>> children;
    TagTreeNode* parent = nullptr;
    bool isRoot = true;
    CXCursor cursor;
    QVector<QVariant> costomTag;
    int childVisitIndex = 0;
    ~TagTreeNode() = default;
};

// 前向声明 SpiderCppFileVisitor（供 SpiderBox 使用）
class SpiderCppFileVisitor;

// ---------- 蛛控制器 ----------
class SpiderController {
public:
    explicit SpiderController(TagTreeNode* node, int dataSize = 0,
        RunTimeErrorCollector* err = nullptr);

    SpiderController() = delete;

    // 基础查询
    const TagTreeNode* rawNode() const { return m_node; }
    CXCursorKind getCursorKind() const;
    QString getCursorSpelling() const;
    QString getCursorUSR() const;

    // 标记读写
    template<typename Enum>
    QVariant readTag(Enum name) const {
        int idx = static_cast<int>(name);
        Q_ASSERT(idx < m_node->costomTag.size());
        return m_node->costomTag[idx];
    }

    template<typename Enum>
    void setTag(Enum name, const QVariant& value) {
        int idx = static_cast<int>(name);
        Q_ASSERT(idx < m_node->costomTag.size());
        m_node->costomTag[idx] = value;
    }

    // 移动指令（调用时立刻计算目标节点）
    void goParent();
    void nextChild();
    void nextBrother();
    void notMove();
    void stop();

    // 状态查询
    bool shouldStop() const;
    bool badControl() const;
    int dataSize;

    // 供 SpiderBox 消费
    TagTreeNode* consumeNextNode();

private:
    void reportError(const QString& msg);
    bool precheck();   // 防止单步内多次设置移动

    TagTreeNode* m_node;
    RunTimeErrorCollector* m_err = nullptr;
    TagTreeNode* m_nextNode = nullptr;
    bool m_shouldStop = false;
    bool m_badControl = false;
};

// ---------- 蛛盒：一次运行上下文 ----------
class SpiderBox {
public:
    SpiderBox(int dataSize, TagTreeNode* current,
        SpiderCppFileVisitor* outer, bool stop = false);
    SpiderBox actFork();
    // 执行一步移动（消费控制器计算好的目标节点，并复位状态）
    void act();
    SpiderBox act_new();
    int dataSize;
    TagTreeNode* current = nullptr;
    SpiderController controller;
    bool stoped = false;
    SpiderCppFileVisitor* outer;
};

// 定义一次“蛛”运行函数
using Spider = std::function<void(SpiderController&)>;

// ---------- 文件访问器 ----------
class SpiderCppFileVisitor {
public:
    SpiderCppFileVisitor(const QString& cppFile,
        const QStringList& includeDirs,
        RunTimeErrorCollector& ec);
    ~SpiderCppFileVisitor();

    // 从根节点创建蛛盒
    SpiderBox getBox(int dataSize);

    // 在整棵树上运行自定义蛛逻辑
    void spiderRun(Spider spiderFn, int customDataSize);

    // 允许 SpiderBox 访问错误收集器
    RunTimeErrorCollector& m_err;

private:
    static std::unique_ptr<TagTreeNode> buildTreeFromTU(CXTranslationUnit tu);
    static void buildChildren(TagTreeNode* parent);

    CXIndex m_index = nullptr;
    CXTranslationUnit m_tu = nullptr;
    std::unique_ptr<TagTreeNode> m_root;
};