#include <memory>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#include "./fuseWrapper/FuseWrapper.h"
#include "./transfer/server/MetadataServer.h"
#include "./Utils.h"
#include "fsprovider/client/Client.h"
#include "fsprovider/server/Server.h"

void sigIntHandler(int);

MediaFs::MetadataServer *parser;

std::map<std::string, std::string> getOptions(int argc, char *argv[]);

void printHelp();

int main(int argc, char *argv[]) {
    char fOpt[] = "-f",
         dOpt[] = "-d";
    char *fuseOpts[] = {fOpt, NULL, dOpt};
    struct sigaction sigHandler;
    sigHandler.sa_handler = sigIntHandler;
    sigemptyset(&sigHandler.sa_mask);
    sigHandler.sa_flags = 0;

    sigaction(SIGINT, &sigHandler, NULL);
    std::map<std::string, std::string> options = getOptions(argc, argv);
    if (options.empty()) {
        printHelp();
        return 1;
    }
    if (options["type"] == "client") {
        fuseOpts[1] = const_cast<char *>(options["directory"].c_str());
        return fuse_main(3, fuseOpts, MediaFs::getRegistered(), 0);
    } else if (options["type"] == "server") {
        if (chdir(options["directory"].c_str()) < 0) {
            std::cerr << "cannot change to dir " << argv[1] << "\n";
            return 1;
        }
        std::unique_ptr<MediaFs::FSProvider> fsProvider = std::make_unique<MediaFs::Server>();
        MediaFs::MetadataServer *parser = new MediaFs::MetadataServer(8086, std::move(fsProvider));
        parser->startListen();
        delete parser;
    } else {
        printHelp();
    }
    return 0;
    //return fuse_main(argc, argv, MediaFs::getRegistered(), 0);
}

void sigIntHandler(int signal) {
    std::cout << "sigint " << signal << "\n";
    exit(0);
}

struct option allOpts[] = {
    {"help", no_argument, 0, 'h'},
    {"type", required_argument, 0, 't'},
    {"directory", required_argument, 0, 'd'},
};

std::map<std::string, std::string> getOptions(int argc, char *argv[]) {
    std::map<std::string, std::string> options;
    int option;
    int index = 0;
    while ((option = getopt_long(argc, argv, "ht:d:", allOpts, &index)) != -1) {
        switch (option) {
            case 'h': {
                          options["help"] = "";
                      }
                      break;
            case 't': {
                          options["type"] = optarg;
                          if (options["type"] != "server" && options["type"] != "client") {

                              std::cerr << "Unknown type "  << options["type"] << "\n";
                              options.clear();
                              return options;
                          }
                      }
                      break;
            case 'd': {
                          options["directory"] = optarg;
                      }
                      break;
            default: {
                         std::string optionval;
                         optionval.push_back('-');
                         optionval.push_back(option);
                         options[optionval] = optarg == NULL ? "" : optarg;
                     }
                     break;

        }
    }
    return options;
}

void printHelp() {
    std::cout << "usage: sshfs --t=client|server --d=PATH\n";
    std::cout << "\t-t type=client|server\t\t act as mediafs client or server\n";
    std::cout << "\t-d directory=PATH the mount point for client OR the source directory for server\n";
}


