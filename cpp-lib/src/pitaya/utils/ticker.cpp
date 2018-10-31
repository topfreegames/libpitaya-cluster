#include "pitaya/utils/ticker.h"

namespace pitaya {
namespace utils {

void
Ticker::Start()
{
    _donePromise = std::promise<void>();
    _doneFuture = _donePromise.get_future();
    std::async(std::launch::async, &Ticker::TickWrapper, this);
}

void
Ticker::Stop()
{
    _donePromise.set_value();
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

}
}