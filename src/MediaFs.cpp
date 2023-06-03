#include <memory>

#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/MetadataServer.h"

int main(int argc, char *argv[]) {
    std::unique_ptr<MediaFs::FSProvider> fsProvider = std::make_unique<MediaFs::FSProvider>();
    MediaFs::MediaPacketParser parser(8086, std::move(fsProvider));
    parser.startListen();
    return 0;
    //return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}


