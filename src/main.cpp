#include "log.hpp"
#include "error.h"
#include "configuration.h"
#include "layer_info.h"
#include "block.h"
#include "main_redirect.h"
#include  "rstring.h"

int main(int argc, char *argv[])
{
    try
    {
        std::string redirect_name(argv[0]);
        regex_replace_all(redirect_name, R"(.*/)", [](std::string)->std::string{ return ""; });
        regex_replace_all(redirect_name, R"(\..*)", [](std::string)->std::string{ return ""; });
        if (redirect_name == "fsck")
        {
            return fsck_main(argc, argv);
        }

        if (redirect_name == "mkfs")
        {
            return mkfs_main(argc, argv);
        }

        if (redirect_name == "mount")
        {
            return mount_main(argc, argv);
        }

        throw std::invalid_argument("Unknown command");
    }
    catch (std::exception & e)
    {
        error_log("Exception occurred: " + std::string(e.what()) + "\n");
        return EXIT_FAILURE;
    }
    catch (...)
    {
        error_log("Unknown exception occurred\n");
        return EXIT_FAILURE;
    }
}
