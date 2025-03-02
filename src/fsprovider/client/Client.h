#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <queue>
#include <boost/asio.hpp>
#include "../FSProvider.h"
#include "../server/Server.h"

#define WAITING_LENGTH -1

using ios = boost::asio::io_service;
using tcp = boost::asio::ip::tcp;

typedef std::tuple<int, const char *> Content;

#define MAKE_CONTENT(len, s) std::make_tuple(len, s)

namespace MediaFs {
    class ClientBuf {
        private:
            std::vector<Content> pendingContents;
            std::queue<Content> readyContents;
            std::vector<const char *> allContents;
            std::mutex pendingContentsMutex, readyContentsMutex;
            bool contentReady;
            int expectingLength;
            bool waitingForLength;
            int totalLength;
            int readyLength;
            const char *extractLength(const char *buf, int &bufLen, int &len);
            void cleanBuffers();
            void addReadyContent();
            int calculateTotalLength();
        public:
            bool add(const char *data, int len);
            int read(char *buf, int len);
            int size();
            int getTotalPendingLength();
            ClientBuf();
            virtual ~ClientBuf();
            bool isContentReady();
            Content& operator[](int index);
            int getReadyLength() const;
    };

    class Client  : public FSProvider {
        private:
            ios io;
            tcp::endpoint endpoint;
            LRUCache<std::string, FileCache<std::string>*> openHandles;
            void sendRequest(tcp::socket &, std::string);
            const char *readResponse(int &);
            void readUntilReady(ClientBuf &, tcp::socket &);
        public:
            Client(int length = 10);
            const char* read(std::string path, int &size, int offset);
            std::vector<Attr> readDir(std::string path) const;
            std::tuple<int, const char*> getContent(const char *, int len);
            Attr getAttr(std::string path) const;
    };
};
#endif

