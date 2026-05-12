#include"SpiderVisitor.h"
#include "RuntimeErrcollector.h"

// ==================== SpiderController ====================
SpiderController::SpiderController(TagTreeNode* node, int dataSize,
    RunTimeErrorCollector* err)
    : dataSize(dataSize), m_node(node), m_err(err)
{
    if (dataSize > 0 && m_node->costomTag.isEmpty()) {
        m_node->costomTag.resize(dataSize);
    }
}

CXCursorKind SpiderController::getCursorKind() const {
    return clang_getCursorKind(m_node->cursor);
}

QString SpiderController::getCursorSpelling() const {
    CXString s = clang_getCursorSpelling(m_node->cursor);
    QString ret = QString::fromUtf8(clang_getCString(s));
    clang_disposeString(s);
    return ret;
}

QString SpiderController::getCursorUSR() const {
    CXString s = clang_getCursorUSR(m_node->cursor);
    QString ret = QString::fromUtf8(clang_getCString(s));
    clang_disposeString(s);
    return ret;
}

bool SpiderController::precheck() {
    if (m_badControl || m_nextNode || m_shouldStop) {
        m_badControl = true;
        return true;
    }
    return false;
}

void SpiderController::reportError(const QString& msg) {
    if (m_err) m_err->putError(msg);
}

void SpiderController::goParent() {
    if (precheck()) return;
    if (m_node->parent) {
        m_nextNode = m_node->parent;
    }
    else {
        reportError("spider control: try get parent on root node");
        m_shouldStop = true;
    }
}

void SpiderController::nextChild() {
    if (precheck()) return;
    if (m_node->childVisitIndex < m_node->children.size()) {
        m_nextNode = m_node->children[m_node->childVisitIndex].get();
        ++m_node->childVisitIndex;
    }
    else {
        reportError("spider control: child index out of range");
        m_shouldStop = true;
    }
}

void SpiderController::nextBrother() {
    if (precheck()) return;
    if (!m_node->parent) {
        reportError("spider control: try get brother on root node");
        m_shouldStop = true;
        return;
    }
    const auto& siblings = m_node->parent->children;
    for (int i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == m_node) {
            if (i + 1 < siblings.size()) {
                m_nextNode = siblings[i + 1].get();
                return;
            }
            else {
                reportError("spider control: no next brother");
                m_shouldStop = true;
                return;
            }
        }
    }
    reportError("spider control: current node not found in parent's children");
    m_shouldStop = true;
}

void SpiderController::notMove() {
    if (precheck()) return;   
    m_nextNode = m_node;   
}
void SpiderController::stop() {
    if (precheck()) return;
    m_shouldStop = true;
}

bool SpiderController::shouldStop() const {
    return m_shouldStop;
}

bool SpiderController::badControl() const {
    return m_badControl;
}

TagTreeNode* SpiderController::consumeNextNode() {
    TagTreeNode* n = m_nextNode;
    m_nextNode = nullptr;
    return n;
}

// ==================== SpiderBox ====================
SpiderBox::SpiderBox(int dataSize, TagTreeNode* current,
    SpiderCppFileVisitor* outer, bool stop)
    : dataSize(dataSize),
    current(current),
    controller(current, dataSize, &outer->m_err), // 注入错误收集器
    stoped(stop),
    outer(outer)
{
}

SpiderBox SpiderBox::actFork() {
    // 基于当前状态创建移动分支

    SpiderBox afterAct = *this;
    // 移动分支执行一步

    *this = SpiderBox(dataSize, current, outer, stoped);
    this->controller.notMove();
    this->act();

    return afterAct;
}

void SpiderBox::act() {
    if (stoped) {
        outer->m_err.putError("spider run: is stoped");
        return;
    }
    if (controller.badControl()) {
        outer->m_err.putError("spider control: multiple move actions set in one step");
        stoped = true;
        return;
    }
    if (controller.shouldStop()) {
        stoped = true;
        return;
    }

    TagTreeNode* next = controller.consumeNextNode();
    if (!next) {
        outer->m_err.putError("spider control: no move action set");
        stoped = true;
        return;
    }

    // 完成移动，并用新节点重建控制器（状态复位）
    current = next;
    controller = SpiderController(current, dataSize, &outer->m_err);
}

SpiderBox SpiderBox::act_new()
{
    if (stoped) {
        outer->m_err.putError("spider run: is stoped");
        return SpiderBox(dataSize, current, outer, true);
    }
    if (controller.badControl()) {
        outer->m_err.putError("spider control: multiple move actions set in one step");
        return SpiderBox(dataSize, current, outer, true);
    }
    if (controller.shouldStop()) {
        SpiderBox newOne(dataSize, current, outer, true);
        newOne.stoped = true;
        return newOne;
    }

    TagTreeNode* next = controller.consumeNextNode();
    controller = SpiderController(current, dataSize, &outer->m_err);

    if (!next) {
        outer->m_err.putError("spider control: no move action set");
        return SpiderBox(dataSize, current, outer, true);
    }
    return SpiderBox(dataSize, next, outer, false);
}
// ==================== SpiderCppFileVisitor ====================
SpiderCppFileVisitor::SpiderCppFileVisitor(const QString& cppFile,
    const QStringList& includeDirs,
    RunTimeErrorCollector& ec)
    : m_err(ec)
{
    m_index = clang_createIndex(0, 0);

    QVector<const char*> args;
    args.push_back("-x");
    args.push_back("c++");
    QByteArray tmp;
    for (const QString& dir : includeDirs) {
        tmp = ("-I" + dir).toUtf8();
        args.push_back(tmp.constData());
    }

    m_tu = clang_parseTranslationUnit(
        m_index,
        cppFile.toUtf8().constData(),
        args.data(), static_cast<int>(args.size()),
        nullptr, 0,
        CXTranslationUnit_None);

    if (m_tu) {
        m_root = buildTreeFromTU(m_tu);
    }
    else {
        qWarning("Failed to parse translation unit: %s", qPrintable(cppFile));
    }
}

SpiderCppFileVisitor::~SpiderCppFileVisitor() {
    if (m_tu)   clang_disposeTranslationUnit(m_tu);
    if (m_index) clang_disposeIndex(m_index);
}

SpiderBox SpiderCppFileVisitor::getBox(int dataSize) {
    return SpiderBox(dataSize, m_root.get(), this);
}

void SpiderCppFileVisitor::spiderRun(Spider spiderFn, int customDataSize) {
    if (!m_root) return;

    SpiderBox box = getBox(customDataSize);
    while (!box.stoped && box.current) {
        spiderFn(box.controller);   // 用户函数，内部调用移动指令
        box.act();                  // 消费指令，切换节点
    }
}

// 静态树构建辅助
std::unique_ptr<TagTreeNode> SpiderCppFileVisitor::buildTreeFromTU(CXTranslationUnit tu) {
    auto root = std::make_unique<TagTreeNode>();
    root->cursor = clang_getTranslationUnitCursor(tu);
    root->isRoot = true;
    buildChildren(root.get());
    return root;
}

void SpiderCppFileVisitor::buildChildren(TagTreeNode* parent) {
    QVector<CXCursor> childCursors;
    clang_visitChildren(
        parent->cursor,
        [](CXCursor child, CXCursor /*parent*/, CXClientData data) {
            auto* vec = static_cast<QVector<CXCursor>*>(data);
            vec->push_back(child);
            return CXChildVisit_Continue;
        },
        &childCursors);

    for (const auto& childCursor : childCursors) {
        auto node = std::make_unique<TagTreeNode>();
        node->cursor = childCursor;
        node->isRoot = false;
        node->parent = parent;
        buildChildren(node.get());
        parent->children.push_back(std::move(node));
    }
}