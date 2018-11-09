#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "trompeloeil.hpp"

//=============================================
// Integration with the mocking library
//=============================================
namespace trompeloeil {
template<>
void
reporter<specialized>::send(severity s, const char* file, unsigned long line, const char* msg)
{
    auto f = line ? file : "[file/line unavailable]";
    if (s == severity::fatal) {
        ADD_FAIL_AT(f, line, msg);
    } else {
        ADD_FAIL_CHECK_AT(f, line, msg);
    }
}
} // namespace trompeloeil

int
main(int argc, char* argv[])
{
    doctest::Context context;

    context.setOption("no-breaks", true);
    context.applyCommandLine(argc, argv);

    int res = context.run();
    return res;
}
