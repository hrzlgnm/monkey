#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#include <doctest/doctest.h>

auto main(int argc, char** argv) -> int
{
    doctest::Context context;

    context.applyCommandLine(argc, argv);

    return context.run();
}
