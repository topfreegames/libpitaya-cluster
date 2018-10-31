#ifndef PITAYA_UTILS_SEMAPHORE_H
#define PITAYA_UTILS_SEMAPHORE_H

#include <condition_variable>
#include <mutex>

namespace pitaya {
namespace utils {

class Semaphore
{
public:
    Semaphore()
        : _count(0)
    {}

    void Notify()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        ++_count;
        _signal.notify_one();
    }

    void NotifyAll(uint64_t count = 1)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _count += count;
        _signal.notify_all();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_count == 0) {
            _signal.wait(lock);
        }
        --_count;
    }

    bool TryWait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_count) {
            --_count;
            return true;
        } else {
            return false;
        }
    }

private:
    std::condition_variable _signal;
    std::mutex _mutex;
    uint64_t _count;
};

}
}

#endif // PITAYA_UTILS_SEMAPHORE_H
