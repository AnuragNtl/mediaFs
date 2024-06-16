#include <memory>
#include <signal.h>

#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/MetadataServer.h"
#include "./Utils.h"
#include "fsprovider/client/Client.h"
#include "fsprovider/server/Server.h"

void sigIntHandler(int);

MediaFs::MetadataServer *parser;

int main(int argc, char *argv[]) {
    if (argc > 0 && strcmp(argv[0], "client")) {
        MediaFs::Client client(10);
    } else {
        struct sigaction sigHandler;
        sigHandler.sa_handler = sigIntHandler;
        sigemptyset(&sigHandler.sa_mask);
        sigHandler.sa_flags = 0;

        sigaction(SIGINT, &sigHandler, NULL);

        std::string s = " the quick brown fox jumps  over the lazy dog ";
        std::vector<std::string> data = MediaFs::split(s, "  ");
        std::cout << "__::\n";
        for (auto &item : data) {
            std::cout << item << "::\n";
        }
        std::unique_ptr<MediaFs::FSProvider> fsProvider = std::make_unique<MediaFs::Server>();
        MediaFs::MetadataServer *parser = new MediaFs::MetadataServer(8086, std::move(fsProvider));
        parser->startListen();
        delete parser;
    }
    return 0;
    //return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}

void sigIntHandler(int signal) {
    std::cout << "sigint " << signal << "\n";
}


