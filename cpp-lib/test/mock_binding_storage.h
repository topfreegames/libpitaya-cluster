#ifndef PITAYA_MOCK_BINDING_STORAGE_H
#define PITAYA_MOCK_BINDING_STORAGE_H

#include "pitaya/binding_storage.h"

#include <gmock/gmock.h>

class MockBindingStorage : public pitaya::BindingStorage
{
public:
    MOCK_METHOD2(GetUserFrontendId,
                 std::string(const std::string& uid, const std::string& frontendType));
};

#endif // PITAYA_MOCK_BINDING_STORAGE_H
