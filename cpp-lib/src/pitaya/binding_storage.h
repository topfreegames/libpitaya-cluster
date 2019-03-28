#ifndef PITAYA_BINDING_STORAGE_H
#define PITAYA_BINDING_STORAGE_H

#include <chrono>
#include <string>

namespace pitaya {

class BindingStorage
{
public:
    virtual std::string GetUserFrontendId(const std::string& uid,
                                          const std::string& frontendType) = 0;
};

} // namespace pitaya

#endif // PITAYA_BINDING_STORAGE_H