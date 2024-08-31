#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
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
            bool contentReady;
            int expectingLength;
            bool waitingForLength;
            int totalLength;
            ClientBuf();
            virtual ~ClientBuf();
            const char *extractLength(const char *buf, int &bufLen, int &len);
            void cleanBuffers();
        public:
            bool add(const char *data, int len);
            int read(char *buf, int len);
            int size();
            int getTotalLength();
            int getRemainingLength();
            bool isContentReady();
            Content& operator[](int index);
    };

    class Client  : public FSProvider {
        private:
            ios io;
            tcp::socket *socket;
            tcp::endpoint endpoint;
            // TODO FileCache should also process string keys 
            // which will be filenames, rather than file handles.
            // File names because the client would only know about them.
            LRUCache<std::string, FileCache<std::string>*> openHandles;
            ClientBuf clientBuf;
            void sendRequest(std::string);
        public:
            Client(int length = 10);
            const char* read(std::string path, int &size, int offset);
            std::vector<Attr> readDir(std::string path) const;
            std::tuple<int, const char*> getContent(const char *, int len);
            Attr getAttr(std::string path) const;
    };
};
#endif


