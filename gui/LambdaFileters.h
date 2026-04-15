#pragma once
#include <QObject>
#include <QEvent>
#include <functional>

class LambdaEventFilter : public QObject {
    Q_OBJECT
public:
    // 定义两个函数签名
    // FilterFunc：返回 true 表示拦截事件
    // ActionFunc：拦截后执行的动作（无返回值）
    using FilterFunc = std::function<bool(QObject*, QEvent*)>;
    using ActionFunc = std::function<void(QObject*, QEvent*)>;

    explicit LambdaEventFilter(FilterFunc filterFunc, ActionFunc actionFunc,
        QObject* parent = nullptr)
        : QObject(parent)
        , m_filterFunc(std::move(filterFunc))
        , m_actionFunc(std::move(actionFunc))
    {
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        // 如果过滤函数存在且返回 true，则拦截该事件
        if (m_filterFunc && m_filterFunc(obj, event)) {
            if (m_actionFunc)
                m_actionFunc(obj, event);
            return true;  // 事件已被拦截，不再继续传递
        }
        // 否则交给父类默认处理
        return QObject::eventFilter(obj, event);
    }

private:
    FilterFunc m_filterFunc;
    ActionFunc m_actionFunc;
};