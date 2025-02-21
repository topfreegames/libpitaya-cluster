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
    _donePromise = std::make_shared<boost::promise<void>>();
    _doneFuture = _donePromise->get_future();
    _runFuture = boost::async(boost::launch::async, boost::bind(&Ticker::TickWrapper,this));
}

void
Ticker::Stop()
{
    if (_running) {
        _running = false;
        _donePromise->set_value();
        _runFuture.wait();
    }
}

void
Ticker::TickWrapper()
{
    auto status = boost::future_status::timeout;
    while (status != boost::future_status::ready) {
        status = _doneFuture.wait_for(_tickInterval);
        if (status != boost::future_status::ready) {
            _callback();
        }
    }
}

} // namespace utils
} // namespace pitaya
