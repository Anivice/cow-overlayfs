#include "log.hpp"
#include "error.h"
#include "configuration.h"
#include "layer_info.h"

int main(int argc, char *argv[])
{
    try
    {
        info_log(*argv, ": build ID ", BUILD_ID, ", built on ", BUILD_TIME, ", version ", VERSION, "\n");
        if (argc != 3)
        {
            error_log(*argv, " [Configuration] [Mount Point]\n");
            return EXIT_FAILURE;
        }

        const std::string config_path = argv[1];
        const std::string mount_point = argv[2];
        LayerInfoType layer_global_readonly_info {};
        info_log("Using configuration ", config_path, ", mounting at ", mount_point, "\n");
        for (const configuration config(config_path);
            const auto & [section, keys] : config)
        {
            debug_log("Checking section ", section, "...\n");
            if (section != "general")
            {
                warning_log("Section \"", section, "\" unknown, skipped\n");
                continue;
            }

            for (const auto & [key, val] : keys)
            {
                cow_assert_wm(val.size() == 1, InvalidConfiguration, "Faulty definition of key \"" + key + "\"")
                debug_log("Entry: Section \"", section, "\": \"", key, "\": \"", val, "\"\n");
                if (key == "attributes")
                {
                    layer_global_readonly_info.path_to_block_attribute_dir = val.front();
                }
                else if (key == "data")
                {
                    layer_global_readonly_info.path_to_data_blocks = val.front();
                }
                else if (key == "block_size")
                {
                    layer_global_readonly_info.block_size = std::strtoull(val.front().c_str(), nullptr, 10);
                }
                else if (key == "root")
                {
                    layer_global_readonly_info.root_inode_name = val.front();
                }
                else
                {
                    warning_log("Unknown key \"" + key + "\", skipped\n");
                }
            }
        }

        cow_assert_wm(
            !(layer_global_readonly_info.block_size == 0
                || layer_global_readonly_info.path_to_block_attribute_dir.empty()
                || layer_global_readonly_info.root_inode_name.empty()
                || layer_global_readonly_info.path_to_data_blocks.empty()),
            InvalidConfiguration, "Faulty configuration!");
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
