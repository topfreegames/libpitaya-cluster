#ifndef PITAYA_UTILS_TICKER_H
#define PITAYA_UTILS_TICKER_H

#include <chrono>
#include <functional>
#include <future>

namespace pitaya {
namespace utils {

class Ticker
{
public:
    template<typename DurationType>
    Ticker(DurationType interval, std::function<void()> cb)
        : _running(false)
        , _tickInterval(std::chrono::duration_cast<std::chrono::milliseconds>(interval))
        , _callback(cb)
    {}

    ~Ticker();

    void Start();
    void Stop();

    Ticker& operator=(const Ticker&) = delete;
    Ticker(const Ticker&) = delete;

private:
    void TickWrapper();

private:
    bool _running;
    std::chrono::milliseconds _tickInterval;
    std::function<void()> _callback;
    std::shared_ptr<std::promise<void>> _donePromise;
    std::future<void> _doneFuture;
    std::future<void> _runFuture;
};

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_TICKER_H
