#include "./fuseWrapper/FuseWrapper.h"

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}


