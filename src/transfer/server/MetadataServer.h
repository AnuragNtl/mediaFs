#ifndef TRANSFER_SERVER_H

#define TRANSFER_SERVER_H

#include "../fsprovider/FSProvider.h"

#include <memory>

namespace MediaFs {
    class MediaPacketParser {
        private:
            int port;
            std::unique_ptr<FSProvider> client;
        public:
            MediaPacketParser(int port, std::unique_ptr<FSProvider>);
            void startListen();
    };
};

#endif

