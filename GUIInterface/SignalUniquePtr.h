// SignalUniquePtr.h
#pragma once

#include <memory>
#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include<functional>
class SignalHelper : public QObject {
    Q_OBJECT
public:
    explicit SignalHelper(QObject* parent = nullptr) : QObject(parent) {}

signals:
    void dataChanged();  
};

template <class T>
class SignalUniquePtr {
public:
    using element_type = T;
    using pointer = T*;

    explicit SignalUniquePtr(pointer ptr = nullptr) noexcept
        : res(ptr), helper(std::make_unique<SignalHelper>())
    {
    }

    explicit SignalUniquePtr(std::unique_ptr<T>&& ptr) noexcept
        : res(std::move(ptr)), helper(std::make_unique<SignalHelper>())
    {
    }

    SignalUniquePtr(const SignalUniquePtr&) = delete;
    SignalUniquePtr& operator=(const SignalUniquePtr&) = delete;

    SignalUniquePtr(SignalUniquePtr&& other) noexcept
        : res(std::move(other.res))
        , helper(std::move(other.helper))
    {
    }


    SignalUniquePtr& operator=(SignalUniquePtr&& other) noexcept
    {
        if (this != &other) {
            res = std::move(other.res);
            helper = std::move(other.helper);
        }
        return *this;
    }

    void resetAsync(pointer ptr = nullptr)
    {
        resetAsync(std::unique_ptr<T>(ptr));
    }

    void resetAsync(std::unique_ptr<T>&& newPtr)
    {
        QMetaObject::invokeMethod(helper.get(),
            [this, ptr = std::move(newPtr)]() mutable {
                res = std::move(ptr);
                emit helper->dataChanged();
            },
            Qt::QueuedConnection);
    }

    void resetSync(pointer ptr = nullptr)
    {
        resetSync(std::unique_ptr<T>(ptr));
    }

    void resetSync(std::unique_ptr<T>&& newPtr)
    {
        res = std::move(newPtr);
        emit helper->dataChanged();
    }

    pointer release() noexcept
    {
        return res.release();
    }

    pointer get() const noexcept
    {
        return res.get();
    }

    T& operator*() const noexcept
    {
        return *res;
    }

    pointer operator->() const noexcept
    {
        return res.operator->();
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(res);
    }

    bool operator==(pointer p) const noexcept
    {
        return res.get() == p;
    }

    bool operator!=(pointer p) const noexcept
    {
        return res.get() != p;
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return res == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept
    {
        return res != nullptr;
    }

    bool operator==(const SignalUniquePtr& other) const noexcept
    {
        return res == other.res;
    }

    bool operator!=(const SignalUniquePtr& other) const noexcept
    {
        return res != other.res;
    }

    QObject* signalSender() const noexcept
    {
        return helper.get();
    }
    using CallBack = std::function<void(void)>;
    void connectDataChanged(const QObject* receiver, CallBack&& slot, Qt::ConnectionType type = Qt::AutoConnection)
    {
        QObject::connect(helper.get(), &SignalHelper::dataChanged, receiver, slot, type);
    }

private:
    std::unique_ptr<T> res;
    std::unique_ptr<SignalHelper> helper; 
};