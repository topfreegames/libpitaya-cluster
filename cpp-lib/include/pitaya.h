#ifndef TFG_PITAYA_H
#define TFG_PITAYA_H

using ServerId = std::string;

namespace pitaya
{
struct Server
{
    std::string id;
    std::string type;
    std::string metadata;
    std::string hostname;
    bool frontend;

    std::string GetKey() const
    {
        // TODO: Add fmt library to improve this code.
        return std::string("servers") + std::string("/") + type + std::string("/") + id;
    }
};

} // namespace pitaya
#endif