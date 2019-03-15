#include "pitaya/utils/ticker.h"

namespace pitaya {
namespace utils {

Ticker::~Ticker()
{
    Stop();
}

void
Ticker::Start()
{
    _running = true;
    _donePromise = std::promise<void>();
    _doneFuture = _donePromise.get_future();
    _runFuture = std::async(std::launch::async, &Ticker::TickWrapper, this);
}

void
Ticker::Stop()
{
    if (_running) {
        _running = false;
        _donePromise.set_value();
        _runFuture.wait();
    }
}

void
Ticker::TickWrapper()
{
    auto status = std::future_status::timeout;
    while (status != std::future_status::ready) {
        status = _doneFuture.wait_for(_tickInterval);
        if (status != std::future_status::ready) {
            _callback();
        }
    }
}

} // namespace utils
} // namespace pitaya
