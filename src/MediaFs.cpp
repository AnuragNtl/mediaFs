#include <memory>
#include <signal.h>
#include <unistd.h>

#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/MetadataServer.h"
#include "./Utils.h"
#include "fsprovider/client/Client.h"
#include "fsprovider/server/Server.h"

void sigIntHandler(int);

MediaFs::MetadataServer *parser;

int main(int argc, char *argv[]) {
    struct sigaction sigHandler;
    sigHandler.sa_handler = sigIntHandler;
    sigemptyset(&sigHandler.sa_mask);
    sigHandler.sa_flags = 0;

    sigaction(SIGINT, &sigHandler, NULL);
    if (argc > 2) {
        return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
    } else {
        if (argc > 1) {
            if (chdir(argv[1]) < 0) {
                std::cerr << "cannot change to dir " << argv[1] << "\n";
                return 1;
            }
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
    exit(0);
}

