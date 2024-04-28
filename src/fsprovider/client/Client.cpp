#include "./Client.h"
#include "../../transfer/server/MetadataServer.h""

namespace MediaFs {

    Client :: Client() {
    }

    void Client :: sendRequest(std::vector<std::string>) {
        tcp::socket s(io);
        // TODO
    }

    const char* FSProvider :: read(std::string path, int &size, int offset) {

    }


};


