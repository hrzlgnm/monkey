#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#include <doctest/doctest.h>

auto main(int argc, char** argv) -> int
{
    doctest::Context context;

    context.applyCommandLine(argc, argv);

    // overrides
    int res = context.run();  // run

    if (context.shouldExit()) {  // important - query flags (and --exit) rely on the user doing this
        return res;  // propagate the result of the tests
    }

    int client_stuff_return_code = 0;
    // your program - if the testing framework is integrated in your production code

    return res + client_stuff_return_code;  // the result from doctest is propagated here as well
}
