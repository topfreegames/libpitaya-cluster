#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

int
main(int argc, char* argv[])
{
    doctest::Context context;

    context.setOption("no-breaks", true);
    context.applyCommandLine(argc, argv);

    int res = context.run();
    return res;
}
