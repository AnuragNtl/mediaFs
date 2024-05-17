#include "./Client.h"
#include "../../transfer/server/MetadataServer.h"

namespace MediaFs {

    Client :: Client(int length) : openHandles(LRUCache<std::string, FileCache<std::string>*>(length)) {
        socket = tcp::socket(io);
        endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
        socket.connect(endpoint);
    }

    void Client :: sendRequest(std::string payload) {
        // TODO
        if (!socket.is_open()) {
            socket = tcp::socket(io);
            socket.connect(endpoint);
        }
        boost::system::system_error error;
        boost::asio::write(socket, boost::asio::buffer(payload), error);

        if (error) {
            throw std::exception();
        }
    }

    const char* FSProvider :: read(std::string path, int &size, int offset) {

    }


};


