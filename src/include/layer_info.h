#ifndef CPPCOWOVERLAY_LAYER_INFO_H
#define CPPCOWOVERLAY_LAYER_INFO_H

#include <cstdint>
#include <string>

struct LayerInfoType
{
    std::string path_to_data_blocks;
    std::string root_inode_name;
    std::string log_dir;
    uint64_t block_size;
};

#endif //CPPCOWOVERLAY_LAYER_INFO_H
