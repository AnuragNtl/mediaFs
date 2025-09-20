#include <tuple>
#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "./Client.h"
#include "../../transfer/server/MetadataServer.h"

namespace MediaFs {

    Client :: Client(int length) : openHandles(LRUCache<std::string, FileCache*>(length)) {
        endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8086);
    }

    const char* ClientBuf :: extractLength(const char *buf, int &bufLen, int &len) {
        short sepLen = strlen(LEN_SEPARATOR);
        int i = 0;
        char prevData[32];
        int ct = 0;
        if (buf[0] == '\0') {
            pendingContentsMutex.unlock();
            throw std::exception();
        }
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
                it--;
                pendingContents.erase(it + 1);
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

    bool ClientBuf :: isWaitingForLength() const {
        return this->waitingForLength;
    }

    int ClientBuf :: getExpectedLength() const {
        return this->expectingLength;
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

            if (buf.isWaitingForLength()) {
                boost::asio::read(socket, recvBuf, boost::asio::transfer_at_least(2), error);
            } else {
                boost::asio::read(socket, recvBuf, boost::asio::transfer_exactly(buf.getExpectedLength() - buf.getTotalPendingLength()), error);
            }

            if (error && error != boost::asio::error::eof) {
                socket.close();
                throw std::exception();
            }

            const char *data = boost::asio::buffer_cast<const char *>(recvBuf.data());
            char *ss = new char[recvBuf.size()];
            memcpy(ss, data, recvBuf.size());

            buf.add(ss, recvBuf.size());
        } while (!buf.isContentReady() && socket.is_open());
    }

    ClientBuf :: ~ClientBuf() {
        for (const auto &item : allContents) {
            delete[] item;
        }
    }

    std::vector<Attr> Client :: readDir(std::string path) {
        tcp::socket socket(io);
        std::vector<std::string> params = {path};
        sendRequest(socket, MediaPacketParser::generate('d', params));
        ClientBuf buf;
        readUntilReady(buf, socket);
        int readyLength = buf.getReadyLength();
        char *data = new char[readyLength];
        buf.read(data, readyLength);
        std::vector<Attr> attrs;
        for (const auto line : MediaFs::split(data, "\n")) {
            attrs.push_back(parseAttr(line));
        }
        return attrs;
    }

    Attr Client :: getAttr(std::string path) {
        tcp::socket socket(io);
        std::vector<std::string> params = {path};
        sendRequest(socket, MediaPacketParser::generate('g', params));
        ClientBuf buf;
        readUntilReady(buf, socket);
        int readyLength = buf.getReadyLength();
        char *data = new char[readyLength];
        buf.read(data, readyLength);
        std::string attrs(data, readyLength);
        auto attr = parseAttr(attrs);
        delete[] data;
        return attr;
    }

    Attr Client :: parseAttr(std::string data) const {
        Attr attr;
        std::vector<std::string> contents = MediaFs::split(data);
        attr.name = contents[0];
        attr.supportedType = (SupportedType)std::stoi(contents[1]);
        attr.size = std::stoi(contents[2]);
        return attr;
    }

    Client :: ~Client() {
        for (FileCache *fileCache : addedCaches) {
            delete fileCache;
        }
    }

    ClientFileHandle :: ClientFileHandle(Client &client, std::string path) : client(client), path(path) { }

    int ClientFileHandle :: read(char *data, int offset, int size) {
        ClientBuf buf;
        tcp::socket socket(client.io);
            std::vector<std::string> params = {path, std::to_string(size), std::to_string(offset)};
        client.sendRequest(socket, 
                MediaPacketParser::generate('r', params));
        client.readUntilReady(buf, socket);
        return buf.read(data, buf.getReadyLength());
    }

    const char* Client :: read(std::string path, int &size, int offset) {
        if (!openHandles.has(path)) {
            FileCache *fileCache = new FileCache(std::unique_ptr<FileHandle>(new ClientFileHandle(*this, path)));
            openHandles.add(path, fileCache);
            addedCaches.push_back(fileCache);
        }

        FileCache &handle = *openHandles[path];
        auto data = handle[std::make_pair(offset, offset + size)];
        size = std::get<1>(data);
        auto contents = std::get<0>(data);
        return contents;
    }

    void Client :: updateCaches() {
        for (FileCache *cache : addedCaches) {
            cache->refreshRangesAsync();
        }
    }
}
