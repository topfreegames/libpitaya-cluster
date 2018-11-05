#ifndef PITAYA_CLUSTER_LOG_OPTIONS_H
#define PITAYA_CLUSTER_LOG_OPTIONS_H

#include "pitaya/service_discovery/log_options.h"

namespace pitaya {
namespace cluster {

struct LogOptions
{
    service_discovery::LogOptions serviceDiscovery;
};

}
}

#endif // PITAYA_CLUSTER_LOG_OPTIONS_H
