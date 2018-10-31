#ifndef SYNC_DEQUE_H
#define SYNC_DEQUE_H

#include <deque>
#include <mutex>

template<typename T>
class SyncDeque {
public:
    // BasicLockable interface for usage with std::lock_guard
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

    void PushBack(T val)
    {
        _deque.push_back(val);
    }

    void Clear()
    {
        _deque.clear();
    }

    T PopFront()
    {
        T val = _deque.front();
        _deque.pop_front();
        return val;
    }

    bool Empty() const
    {
        return _deque.empty();
    }

private:
    std::deque<T> _deque;
    std::mutex _mutex;
};

#endif // SYNC_DEQUE_H
