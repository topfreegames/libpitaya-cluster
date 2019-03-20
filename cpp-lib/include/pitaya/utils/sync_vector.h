#ifndef PITAYA_UTILS_SYNC_VECTOR_H
#define PITAYA_UTILS_SYNC_VECTOR_H

#include <mutex>
#include <vector>

namespace pitaya {
namespace utils {

template<typename T>
class SyncVector
{
public:
    using Iterator = typename std::vector<T>::iterator;
    using ConstIterator = typename std::vector<T>::iterator;

    // BasicLockable interface for usage with std::lock_guard
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

    T& operator[](size_t index) { return _vector[index]; }
    const T& operator[](size_t index) const { return _vector[index]; }

    void PushBack(const T& el) { _vector.push_back(el); }

    Iterator Erase(Iterator start, Iterator end) { return _vector.erase(start, end); }

    // Make type iterable with range-based for loop.
    Iterator begin() { return _vector.begin(); }
    Iterator end() { return _vector.end(); }
    ConstIterator cbegin() const { return _vector.cbegin(); }
    ConstIterator cend() const { return _vector.cend(); }

private:
    std::vector<T> _vector;
    std::mutex _mutex;
};

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_SYNC_VECTOR_H
