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

        const char *it = data;
        int ct = 0;
        char lenData[32];
        for (const char *it = data; *it != ',' && ct < recvBuf.size(); it++, ct++) {
            if (ct >= recvBuf.size() - 1) {
                throw std::exception();
            }
            lenData[ct] = *it;
        }
        
    }

    std::tuple<int, const char *> Client :: getContent(const char *data, int len) {
        const char *it = data;
        int ct = 0;
        char lenData[32];
        for (; *it != ',' && ct < len; it++, ct++) {
            if (ct >= len - 1) {
                throw std::exception();
            }
            lenData[ct] = *it;
        }
        lenData[ct] = '\0';
        return {std::stoi(std::string(lenData)), it + 1};
    }

    const char* Client :: read(std::string path, int &size, int offset) {
        return NULL;
    }

    Attr Client :: getAttr(std::string path) const {

    }

    std::vector<Attr> Client :: readDir(std::string path) const {
        return {};
    }


};


