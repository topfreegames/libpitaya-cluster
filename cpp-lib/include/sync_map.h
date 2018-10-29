#ifndef SYNC_MAP_H
#define SYNC_MAP_H

#include <unordered_map>
#include <mutex>

//
// NOTE(lhahn): This type does not actually map to sync.Map in golang.
//
template<typename K, typename V>
class SyncMap {
public:
    // BasicLockable interface for usage with std::lock_guard
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

    V &operator[](const K &key)
    {
        return _map[key];
    }

    V &operator[](const K &&key)
    {
        return _map[std::move(key)];
    }

    typename std::unordered_map<K, V>::iterator Find(const K &key)
    {
        auto it = _map.find(key);
        return it;
    }

    typename std::unordered_map<K, V>::iterator FindWithLock(const K &key)
    {
        std::lock_guard<decltype(*this)> lock(*this);
        return Find(key);
    }

    void Erase(const K &key)
    {
        _map.erase(key);
    }

    // Make type iterable with range-based for loop.
    typename std::unordered_map<K, V>::iterator begin() { return _map.begin(); }
    typename std::unordered_map<K, V>::iterator end() { return _map.end(); }
    typename std::unordered_map<K, V>::const_iterator cbegin() const { return _map.cbegin(); }
    typename std::unordered_map<K, V>::const_iterator cend() const { return _map.cend(); }

private:
    std::unordered_map<K, V> _map;
    std::mutex _mutex;
};

#endif // SYNC_MAP_H
