#include <memory>

#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/MetadataServer.h"
#include "./Utils.h"

int main(int argc, char *argv[]) {
    std::string s = " the quick brown fox jumps  over the lazy dog ";
    std::vector<std::string> data = MediaFs::split(s, "  ");
    std::cout << "__::\n";
    for (auto &item : data) {
        std::cout << item << "::\n";
    }
    std::unique_ptr<MediaFs::FSProvider> fsProvider = std::make_unique<MediaFs::FSProvider>();
    MediaFs::MetadataServer parser(8086, std::move(fsProvider));
    parser.startListen();
    return 0;
    //return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}


