#include <string>
#include <iostream>
#include <thread>
#include <numeric>
#include <fstream>

#include "./MetadataServer.h"
#include "../../Utils.h"


namespace MediaFs {

    MetadataServer :: MetadataServer(int port, std::unique_ptr<FSProvider> &&client) : parser(std::unique_ptr<MediaPacketParser>(new MediaPacketParser(std::move(client)))), port(port) { }

    void MetadataServer :: startListen() {
        tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), port));
        try {
            while (true) {
                if (!acceptor.is_open()) {
                    return;
                }
                tcp::socket *socket = new tcp::socket(ioService);
                acceptor.accept(*socket);
                if (!socket->is_open()) {
                    return;
                }
                std::thread handle(handleClient, socket, this);
                handle.detach();
            }
        } catch (boost::wrapexcept<boost::system::system_error> e) { }
    }

    void MetadataServer :: handleClient(tcp::socket *socket, MetadataServer *metadataServer) {
        char *errOutput = new char[1];
        errOutput[0] = '\0';
        do {
            std::cout << "__\n";
            boost::asio::streambuf buf;
            boost::asio::read_until(*socket, buf, "\n");
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            int length;
            const char *output = NULL;
            try {
                output = metadataServer->parser->parse(data.c_str(), data.size(), length);
            } catch(std::exception& e) {
                output = NULL;
            } catch (boost::exception &e) {
                output = NULL;
            }
            //boost::asio::write(*socket, boost::asio::buffer(std::string("abcdef")));
            if (output == NULL) {
                length = 1;
                output = errOutput;
                boost::asio::write(*socket, boost::asio::buffer(output, 1));
                break;
            } else {
                std::string len = std::to_string(length);
                boost::asio::write(*socket, boost::asio::buffer(len, len.size()));
                boost::asio::write(*socket, boost::asio::buffer(LEN_SEPARATOR, 1));
                boost::asio::write(*socket, boost::asio::buffer(output, length));
            }
            std::cout << data << "\n";
            delete[] output;
        } while (socket->is_open());
        if (socket->is_open()) {
            socket->close();
        }

        delete socket;
    }

    MediaPacketParser :: MediaPacketParser(std::unique_ptr<FSProvider> &&fsProvider) : fsProvider(std::move(fsProvider)) {
        functionMap['r'] = [this] (std::vector<std::string> data, int &len) {
            int size = stoi(data[1]);
            len = size;
            int offset = stoi(data[2]);
            const char *output = this->fsProvider->read(data[0], len, offset);
            std::cout << "output " << output << "\n";
            return output;
        };

        functionMap['d'] = [this] (std::vector<std::string> data, int &len) {
            std::vector<Attr> attrs = this->fsProvider->readDir(data[0]);
            std::string output = "";
            for (const auto &attr : attrs) {
                output += formatAttr(attr) + "\n";
            }
            len = output.size() + 1;
            char *buf = new char[len];
            strcpy(buf, output.c_str());
            return buf;
        };

        functionMap['g'] = [this] (std::vector<std::string> data, int &len) {
            std::string file = data[0];
            std::string attr = formatAttr(this->fsProvider->getAttr(file));
            len = attr.size() + 1;
            char *buf = new char[len];
            strcpy(buf, attr.c_str());
            return buf;
        };
    }

    const char* MediaPacketParser :: parse(const char *data, int length, int &outputLength) {
        if (length == 0) {
            return NULL;
        }

        std::string content(data + 1, length - 1);
        std::cout << data[0] << " content " << content << "\n";
        std::vector<std::string> params = MediaFs::split(content, " ");
        auto handler = functionMap.find(data[0]);
        if (handler != functionMap.end()) {
            return (handler->second)(params, outputLength);
        }
        return NULL;
    }

    std::map<const char, std::function<const char *(std::vector<std::string>, int &)> > MediaPacketParser::functionMap = std::map<const char, std::function<const char *(std::vector<std::string>, int &)> >();


    std::string MediaPacketParser :: formatAttr(const Attr &attr) const {
        return attr.name + " " + std::to_string((int)attr.supportedType) + " " + std::to_string(attr.size);
    }

    std::string MediaPacketParser :: generate(const char id, std::vector<std::string> options) {
        const char *idContents = &id;
        return std::string(idContents, 1) + std::accumulate(options.begin(), options.end(), std::string(), [] (const std::string s1, const std::string s2) {
                if (s1.empty() || s2.empty()) return s1 + s2;
                return s1 + " " + s2;
                }) + "\n";
    }

};
