#ifndef TRANSFER_SERVER_H

#define TRANSFER_SERVER_H

namespace MediaFs {
    class TransferServer {
        private:
            int port;
        public:
            TransferServer(int port);
            void listen();
    };
};

#endif

