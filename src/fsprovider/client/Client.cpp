#include "./Client.h"
#include "../../transfer/server/MetadataServer.h"

namespace MediaFs {

    Client :: Client(int length) : openHandles(LRUCache<std::string, FileCache<std::string>*>(length)) {
        socket = new tcp::socket(io);
        endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
        socket->connect(endpoint);
    }

    void Client :: sendRequest(std::string payload) {
        // TODO
        if (socket == NULL || !socket->is_open()) {
            if (!socket->is_open()) {
                delete socket;
            }
            socket = new tcp::socket(io);
            socket->connect(endpoint);
        }
        boost::system::error_code error;
        boost::asio::write(*socket, boost::asio::buffer(payload), error);

        if (error) {
            throw std::exception();
        }
        boost::asio::streambuf recvBuf;

        boost::asio::read(*socket, recvBuf, boost::asio::transfer_all(), error);

        if (error && error != boost::asio::error::eof) {
            throw std::exception();
        }

        const char *data = boost::asio::buffer_cast<const char *>(recvBuf.data());
        
    }

    const char* FSProvider :: read(std::string path, int &size, int offset) {
        return NULL;
    }


};


