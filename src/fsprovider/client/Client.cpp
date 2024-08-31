#include <tuple>
#include <stdio.h>
#include <cstring>

#include "./Client.h"
#include "../../transfer/server/MetadataServer.h"

namespace MediaFs {

    Client :: Client(int length) : openHandles(LRUCache<std::string, FileCache<std::string>*>(length)) {
        socket = new tcp::socket(io);
        endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
        socket->connect(endpoint);
    }

    const char* ClientBuf :: extractLength(const char *buf, int &bufLen, int &len) {
        short sepLen = strlen(LEN_SEPARATOR);
        int i = 0;
        char prevData[32];
        int ct = 0;
        for (const auto &content : pendingContents) {
            memcpy(prevData + ct, std::get<1>(content), std::get<0>(content));
            ct += std::get<0>(content);
        }
        for (i = 0; i < bufLen; i++, ct++) {
            if (i >= 32) {
                throw std::exception();
            }
            if ((buf[i] == LEN_SEPARATOR[0]) && (memcmp(buf, LEN_SEPARATOR, strlen(LEN_SEPARATOR))) == 0) {
                buf = buf + i + sepLen;
                bufLen = bufLen - (i + sepLen);
                break;
            } 
            prevData[ct] = buf[i];
        }
        if (i == bufLen) {
            return NULL;
        }
        prevData[ct] = '\0';
        len = std::atoi(prevData);
        return buf;
    }

    ClientBuf :: ClientBuf() : waitingForLength(true) { }

    void ClientBuf :: cleanBuffers() {
        for (const auto &content : pendingContents) {
            delete[] std::get<1>(content);
        }
        pendingContents.clear();
    }

    bool ClientBuf :: isContentReady() {
        return contentReady;
    }

    bool ClientBuf :: add(const char *data, int bufLen) {
        if (isContentReady()) {
            throw std::exception();
        }
        if (pendingContents.size() == 0) {
            int len = 0;
            const char *buf = extractLength(data, bufLen, len);
            if (buf == NULL) {
                pendingContents.push_back(MAKE_CONTENT(bufLen, data));
                return false;
            }
            data = buf;
            expectingLength = len;
            waitingForLength = false;
            cleanBuffers();
        }
        pendingContents.push_back(MAKE_CONTENT(bufLen, data));
        if (totalLength + bufLen >= expectingLength) {
            contentReady = true;
            waitingForLength = true;
        } else {
            contentReady = false;
        }
        return contentReady;
    }

    int ClientBuf :: read(char *buf, int len) {
        int ct = 0;
        std::for_each(pendingContents.begin(), pendingContents.end(), [&ct, &buf] (Content content) {
                int bufSize = std::get<0>(content);
                memcpy(buf, std::get<1>(content), bufSize);
                buf = buf + bufSize;
        });
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

        std::tuple<int, const char *> contentAndLen = getContent(data, recvBuf.size());
    }

    std::pair<Content, Content> Client :: getContent(const char *data, int len) {
        /*const char *it = data;
        int ct = 0;
        char lenData[32];
        bool lenRead = false;
        auto &pendingContents = clientBuf.pendingContents;
        pendingContents.push_back(std::make_tuple(len, data));
        clientBuf.length += len;
        for(const Content &pendingContent : pendingContents) {
            int len = std::get<0>(pendingContent);
            const char *pendingData = std::get<1>(pendingContent);
            if (pendingContent
        }
        for (it = std::get<0>(pendingContent); it != 0; 

        if (std::get<0>(pendingContent) > 0) {
            int previousLength = std::get<0>(pendingContent);
            int i1 = 0;
            const char *prevData = 
            it = std::get<1>(pendingContent);
            bool readingLen = false;
            for (it = std::get<1>(pendingContent); i1 < previousLength && *it != ','; i1++, it++) {
                if (readingLen) {
                    lenData[i1] = pendingContent<1>
                }
                if (*it == ',') {
                    readingLen = true;
                    continue;
                }
            }
        }

        for (; *it != ',' && ct < len; it++, ct++) {
            if (ct >= len - 1) {
                delete[] std::get<1>(pendingContent);
                pendingContent = std::make_tuple(0, static_cast<const char*>(NULL));
            }
            lenData[ct] = *it;
        }
        if (++ct < len) {
            char *pendingData = new char[len - ct];
            memcpy(pendingData, &data[ct], len - ct);
            pendingContent = std::make_tuple(len - ct, pendingData);

        }
        lenData[ct] = '\0';
        return {std::stoi(std::string(lenData)), it + 1};*/
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


