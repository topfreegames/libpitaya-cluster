#ifndef TICKER_H
#define TICKER_H

#include <chrono>
#include <future>
#include <functional>

class Ticker {
public:
    template<typename DurationType>
    Ticker(DurationType interval, std::function<void()> cb)
    : _tickInterval(std::chrono::duration_cast<std::chrono::milliseconds>(interval))
    , _callback(cb)
    {}

    void Start();
    void Stop();

    Ticker &operator=(const Ticker &) = delete;
    Ticker(const Ticker&) = delete;

private:
    void TickWrapper();

private:
    std::chrono::milliseconds _tickInterval;
    std::function<void()> _callback;
    std::promise<void> _donePromise;
    std::future<void> _doneFuture;
};

#endif // TICKER_H
