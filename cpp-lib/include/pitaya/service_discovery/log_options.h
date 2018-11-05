#ifndef PITAYA_SERVICE_DISCOVERY_LOG_OPTIONS_H
#define PITAYA_SERVICE_DISCOVERY_LOG_OPTIONS_H

namespace pitaya {
namespace service_discovery {

struct LogOptions
{
    bool logServerDetails = false;
    bool logLeaseKeepAlive = true;
};

}
}

#endif // PITAYA_SERVICE_DISCOVERY_LOG_OPTIONS_H
