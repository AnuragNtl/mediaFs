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
            // TODO FileCache should also process string keys 
            // which will be filenames, rather than file handles.
            // File names because the client would only know about them.
            LRUCache<std::string, FileCache<std::string>*> openHandles;
            void sendRequest(std::vector<std::string>);
        public:
            Client();
            const char* read(std::string path, int &size, int offset);
            std::vector<Attr> read(std::string path) const;
            Attr getAttr(std::string path) const;
    };
};

#endif

