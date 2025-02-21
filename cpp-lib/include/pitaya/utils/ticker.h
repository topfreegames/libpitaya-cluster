#ifndef PITAYA_UTILS_TICKER_H
#define PITAYA_UTILS_TICKER_H
#define BOOST_THREAD_PROVIDES_FUTURE
#include <chrono>
#include <functional>
#include <future>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

namespace pitaya {
namespace utils {

class Ticker
{
public:
    template<typename DurationType>
    Ticker(DurationType interval, std::function<void()> cb)
        : _running(false)
        , _tickInterval(boost::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(interval).count()))
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
    boost::chrono::milliseconds _tickInterval;
    std::function<void()> _callback;
    std::shared_ptr<boost::promise<void>> _donePromise;
    boost::future<void> _doneFuture;
    boost::future<void> _runFuture;
};

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_TICKER_H
