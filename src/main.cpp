#include "log.hpp"
#include "error.h"

int main()
{
    debug_log("Hello World!");
    debug_log("Welcome to Hello World!\n");
    info_log("Hello World!\n");
    const cppCowOverlayBaseErrorType error(require_back_trace, "Error test");
    debug_log("\n", error.what(), "\n");
    debug_log("Hello World!\n");
}
