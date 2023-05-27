#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/TransferServer.h"

int main(int argc, char *argv[]) {
    MediaFs::MediaPacketParser parser(8086);
    parser.startListen();
    return 0;
    //return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}


