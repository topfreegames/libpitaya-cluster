#ifndef SYNC_MAP_H
#define SYNC_MAP_H

#include <unordered_map>
#include <mutex>

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

private:
    std::unordered_map<K, V> _map;
    std::mutex _mutex;
};

#endif // SYNC_MAP_H
