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
    // BasicLockable interface for usage with std::lock_guard
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

    T& operator[](size_t index) { return _vector[index]; }
    const T& operator[](size_t index) const { return _vector[index]; }

    void PushBack(const T& el) { _vector.push_back(el); }

    // Make type iterable with range-based for loop.
    typename std::vector<T>::iterator begin() { return _vector.begin(); }
    typename std::vector<T>::iterator end() { return _vector.end(); }
    typename std::vector<T>::const_iterator cbegin() const { return _vector.cbegin(); }
    typename std::vector<T>::const_iterator cend() const { return _vector.cend(); }

private:
    std::vector<T> _vector;
    std::mutex _mutex;
};

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_SYNC_VECTOR_H