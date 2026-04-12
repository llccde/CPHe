
inline QString CXStringToQString(CXString str);
struct QStringPool;
// 字符串池
using poolID = int;
struct QStringPool {
    std::vector<QString> strings;
public:
    poolID add(const QString& s);
    QString get(poolID i)const;
};

// 前向声明
struct CodeNode;

// 代码范围
class CodeScope {
    friend CodeNode;
public:
    using CodeLocation = unsigned int;
    QStringPool* FileNamePool = nullptr;
    poolID fileBeginName = 0;
    poolID fileEndName = 0;
    CodeLocation rowBegin = 0;
    CodeLocation columnBegin = 0;
    CodeLocation rowEnd = 0;
    CodeLocation columnEnd = 0;

    explicit CodeScope(const CXCursor& cursor, QStringPool* pool);
    bool operator==(const CodeScope& other) const;
    bool contains(const CodeScope& other) const;
    bool isCrossFileScope() const;
    QString toString() const;
    QString getFileName()const;

    ~CodeScope() = default;
private:
    CodeScope() = default;
};

// 标识符
class Identifier {
    friend CodeNode;
public:
    CXCursorKind kind;
    QString name;
    QString USR;

    explicit Identifier(CXCursor cursor);
    QString toString();
    bool isDecl();
    bool hasName();
private:
    Identifier() {
        name = "root";
        USR = "theRootOfAll";
        //表示无效值,符合预期
        kind = (CXCursorKind)-1;
    };
};

// AST节点
struct CodeNode {
private:
    bool isRoot = false;
    CodeNode();  // 仅用于构造根节点
public:
    CodeScope cs;
    Identifier id;
    std::vector<std::unique_ptr<CodeNode>> beContaineds;

    static CodeNode getRoot();
    explicit CodeNode(CXCursor cursor, QStringPool* pool);
    QString toString();
    bool isNameNode();
    void sinkCallOnRoot(CodeNode*&& node, CodeNode** theParent);
    bool getIsRoot() const;

private:
    void sink_impl(CodeNode*&& node, CodeNode** parent);
};

// 名称映射节点（用于输出）
class CppNameMapNode : public NameMapNode {

    CodePosition mypos;
    poolID fileID;
    QStringPool* pool;

public:
    explicit CppNameMapNode(QString name, const CodeNode* node, QStringPool*);
    QString getMyFileName();
    bool GetIsRoot();
    void setFile(QString myFileName);
    CodePosition getPosition() override;
    ~CppNameMapNode();
    QStringPool* getPool() {
        return pool;
    }
};
class QStringPtrKeyWrapper {
    QString* str = nullptr;

public:
    QStringPtrKeyWrapper() {
    };
    explicit QStringPtrKeyWrapper(QString* str) : str(str) {
        assert(str != nullptr);
    }
    ~QStringPtrKeyWrapper() = default;
    QString* get() const { return str; }

    // unordered_map 必须
    bool operator==(const QStringPtrKeyWrapper& other) const {
        return *str == *other.str;
    }
    bool operator<(const QStringPtrKeyWrapper& other) const {
        return *str < *other.str;
    }

};
class CppNameMapResPack :public NameMapResPack {
public:
    std::unique_ptr<QStringPool> fileNamePool;
    CppNameMapResPack() {
        fileNamePool.reset(new QStringPool);
    }

};
// ==================== CodeAnalyzer::BaseVisitor 类拆分 ====================
class CPPCodeVisitor : public BaseVisitor {
    using child = QStringPtrKeyWrapper;
    using parent = QStringPtrKeyWrapper;
    //std::hash<CPPCodeVisitor::child> a;
    //std::map<child, parent> reversedAST_OrderByUSR;
public:

    CPPCodeVisitor();
    ~CPPCodeVisitor() override = default;

    CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent) override;
    std::unique_ptr<NameMapResPack> getNameMap() override;

private:
    CodeNode rootNode = CodeNode::getRoot();
    QStringPool pool;

    void preOrderDFS_NameMapBuild(NameMapNode* parent, CodeNode* _this, QStringPool* pool, std::map<QStringPtrKeyWrapper, NameMapNode*>&);
};