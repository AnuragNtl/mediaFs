#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <boost/asio.hpp>
#include "../FSProvider.h"
#include "../server/Server.h"

using ios = boost::asio::io_service;
using tcp = boost::asio::ip::tcp;

namespace MediaFs {
    class Client  : public FSProvider {
        private:
            ios io;
            tcp::socket *socket;
            tcp::endpoint endpoint;
            // TODO FileCache should also process string keys 
            // which will be filenames, rather than file handles.
            // File names because the client would only know about them.
            LRUCache<std::string, FileCache<std::string>*> openHandles;
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

