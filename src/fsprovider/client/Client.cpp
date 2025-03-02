#include <tuple>
#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "./Client.h"
#include "../../transfer/server/MetadataServer.h"

namespace MediaFs {

    Client :: Client(int length) : openHandles(LRUCache<std::string, FileCache<std::string>*>(length)) {
        endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
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
            if ((buf[i] == LEN_SEPARATOR[0]) && (memcmp(buf + i, LEN_SEPARATOR, strlen(LEN_SEPARATOR))) == 0) {
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

    ClientBuf :: ClientBuf() : waitingForLength(true), contentReady(false), totalLength(0), expectingLength(0), readyLength(0) { }

    void ClientBuf :: cleanBuffers() {
        pendingContents.clear();
    }

    bool ClientBuf :: isContentReady() {
        return contentReady;
    }

    int ClientBuf :: getTotalPendingLength() {
        pendingContentsMutex.lock();
        int len = this->calculateTotalLength();
        pendingContentsMutex.unlock();
        return len;
    }

    int ClientBuf :: calculateTotalLength() {
        std::vector<int> lengths(pendingContents.size());
        std::transform(pendingContents.begin(), pendingContents.end(), lengths.begin(), [] (Content content) {
                return std::get<0>(content);
                });
        return std::accumulate(lengths.begin(), lengths.end(), 0, [] (int len1, int len2) {  
                return len1 + len2;
                });
    }

    int ClientBuf :: size() {
        return readyContents.size();
    }

    bool ClientBuf :: add(const char *data, int bufLen) {
        pendingContentsMutex.lock();
        allContents.push_back(data);
        if (waitingForLength) {
            int len = 0;
            const char *buf = extractLength(data, bufLen, len);
            ClientBuf &&ss = std::move(*this);

            if (buf == NULL) {
                pendingContents.push_back(MAKE_CONTENT(bufLen, data));
                pendingContentsMutex.unlock();
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
            addReadyContent();
            totalLength = 0;
            readyLength += expectingLength;
            expectingLength = 0;
        } else {
            contentReady = false;
            totalLength += bufLen;
        }
        pendingContentsMutex.unlock();
        return contentReady;
    }

    void ClientBuf :: addReadyContent() {
        int len = this->calculateTotalLength();
        char *buf = new char[len];
        Content readyContent = MAKE_CONTENT(len, buf);
        char *bp = buf;
        int eLen = expectingLength;
        for (auto it = pendingContents.begin(); it != pendingContents.end() && eLen > 0; it++) {
            Content content = *it;
            int curLen = std::min(std::get<0>(content), eLen);
            const char *curBuf = std::get<1>(content);
            memcpy(bp, curBuf, curLen);
            bp += curLen;
            if (eLen < std::get<0>(content)) {
                curBuf += eLen;
                Content updated = MAKE_CONTENT(std::get<0>(content) - eLen, curBuf);
                content.swap(updated);
            } else {
                pendingContents.erase(it);
            }
            eLen -= curLen;
        }
        readyContentsMutex.lock();
        readyContents.push(readyContent);
        readyContentsMutex.unlock();
    }

    Content& ClientBuf :: operator[](int index) {
        return pendingContents[index];
    }

    int ClientBuf :: read(char *buf, int len) {
        readyContentsMutex.lock();
        int ct = 0;
        while (ct < len && !readyContents.empty()) {
            auto item = readyContents.front();
            readyContents.pop();
            int bufSize = std::min(std::get<0>(item), len - ct);
            const char *content = std::get<1>(item);
            if (bufSize < std::get<0>(item)) {
                Content updated = MAKE_CONTENT(bufSize, content + (len - ct));
                item.swap(updated);
            }
            memcpy(buf, content, bufSize);
            buf += bufSize;
            ct += bufSize;
        }
        if (readyContents.empty()) {
            this->contentReady = false;
        }
        readyContentsMutex.unlock();
        readyLength -= ct;
        return ct;
    }

    int ClientBuf :: getReadyLength() const {
        return this->readyLength;
    }

    void Client :: sendRequest(tcp::socket &socket, std::string payload) {
        // TODO
        socket.connect(endpoint);
        boost::system::error_code error;
        boost::asio::write(socket, boost::asio::buffer(payload), error);

        if (error) {
            throw std::exception();
        }
    }

    void Client :: readUntilReady(ClientBuf &buf, tcp::socket &socket) {
        do {
            boost::asio::streambuf recvBuf;
            boost::system::error_code error;

            boost::asio::read(socket, recvBuf, boost::asio::transfer_all(), error);

            if (error && error != boost::asio::error::eof) {
                throw std::exception();
            }

            const char *data = boost::asio::buffer_cast<const char *>(recvBuf.data());

            buf.add(data, recvBuf.size());
        } while (!buf.isContentReady());
    }

    ClientBuf :: ~ClientBuf() {
        for (const auto &item : allContents) {
            delete[] item;
        }
    }

    std::vector<Attr> Client :: readDir(std::string path) const {
        return {};
    }

    Attr Client :: getAttr(std::string path) const {
        return {};
    }

    Content Client :: getContent(const char *data, int len) {
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
        ClientBuf buf;
        tcp::socket socket(io);
            std::vector<std::string> params = {path, std::to_string(size), std::to_string(offset)};
        sendRequest(socket, 
                MediaPacketParser::generate('r', params));
        readUntilReady(buf, socket);
        char *data = new char[size];
        size = buf.read(data, size);
        return data;
    }
}
