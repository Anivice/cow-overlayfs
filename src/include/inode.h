#ifndef CPPCOWOVERLAY_INODE_H
#define CPPCOWOVERLAY_INODE_H

#include <sys/stat.h>
#include "block.h"

class inode
{
public:
    explicit inode(const std::string & block_name);
};


#endif //CPPCOWOVERLAY_INODE_H