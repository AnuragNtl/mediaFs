#ifndef FUSE_WRAPPER_H

#define FUSE_WRAPPER_H

#define FUSE_USE_VERSION 30

#include <iostream>
#include <vector>
#include <fuse.h>

namespace MediaFs {
    struct fuse_operations* getRegistered();
};


#endif

