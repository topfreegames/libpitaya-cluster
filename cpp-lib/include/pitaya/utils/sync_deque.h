#ifndef PITAYA_UTILS_SYNC_DEQUE_H
#define PITAYA_UTILS_SYNC_DEQUE_H

#include <deque>
#include <mutex>

namespace pitaya {
namespace utils {

template<typename T>
class SyncDeque
{
public:
    // BasicLockable interface for usage with std::lock_guard
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

    void PushBack(T val) { _deque.push_back(val); }

    void Clear() { _deque.clear(); }

    T PopFront()
    {
        T val = _deque.front();
        _deque.pop_front();
        return val;
    }

    bool Empty() const { return _deque.empty(); }

    size_t Size() const { return _deque.size(); }

private:
    std::deque<T> _deque;
    std::mutex _mutex;
};

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_SYNC_DEQUE_H
