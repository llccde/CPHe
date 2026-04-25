#pragma once
#include <memory>
#include <cassert>
#include <type_traits>
#include <QStack>

class BaseContext {
public:
    virtual ~BaseContext() = default;
};

class BaseContextManager {
public:
    virtual BaseContext* getTopOne() = 0;
    virtual void newScope() = 0;
    virtual void endScope() = 0;
    virtual ~BaseContextManager() = default;
};

template<class T>
class ContextManagerTemplate : public BaseContextManager { 
    static_assert(std::is_base_of<BaseContext, T>::value,
        "T must derive from BaseContext");

protected:
    QStack<std::unique_ptr<T>> contexts;

    BaseContext* getTopOne() override {
        assert(!contexts.empty());
        return contexts.top().get(); 
    }

    virtual T* getNewContext() = 0;

    void newScope() override {
        contexts.push_back(std::unique_ptr<T>(getNewContext())); 
    }

    void endScope() override {
        contexts.pop();
    }
};