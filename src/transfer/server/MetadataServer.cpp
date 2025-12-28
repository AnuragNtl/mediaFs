#include <boost/exception/exception.hpp>
#include <string>
#include <iostream>
#include <thread>
#include <numeric>
#include <fstream>
#include <cstring>

#include "./MetadataServer.h"
#include "../../Utils.h"


namespace MediaFs {

    MetadataServer :: MetadataServer(int port, std::unique_ptr<FSProvider> &&client) : parser(std::unique_ptr<MediaPacketParser>(new MediaPacketParser(std::move(client)))), port(port), running(false) { }

    void MetadataServer :: startListen() {
        running.store(true);
        tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), port));
        try {
            while (running.load()) {
                if (!acceptor.is_open()) {
                    return;
                }
                tcp::socket *socket = new tcp::socket(ioService);
                acceptor.accept(*socket);
                if (!socket->is_open()) {
                    delete socket;
                    return;
                }
                std::thread handle(handleClient, socket, this);
                handle.detach();
            }
        } catch (boost::wrapexcept<boost::system::system_error> e) { }
    }

    void MetadataServer :: stopServer() {
        running.store(false);
    }

    void MetadataServer :: handleClient(tcp::socket *socket, MetadataServer *metadataServer) {
        char *errOutput = new char[1];
        errOutput[0] = '\0';
        do {
            int length;
            const char *output = NULL;
            std::string data;
            try {
                boost::asio::streambuf buf;
                boost::asio::read_until(*socket, buf, "\n");
                data = boost::asio::buffer_cast<const char*>(buf.data());
                output = metadataServer->parser->parse(data.c_str(), data.size(), length);
            } catch(std::exception& e) {
                output = NULL;
            } catch (boost::exception &e) {
                output = NULL;
            }
            //boost::asio::write(*socket, boost::asio::buffer(std::string("abcdef")));
            try {
                if (output == NULL) {
                    boost::asio::write(*socket, boost::asio::buffer(errOutput, 1));
                    break;
                } else {
                    std::string len = std::to_string(length);
                    boost::asio::write(*socket, boost::asio::buffer(len, len.size()));
                    boost::asio::write(*socket, boost::asio::buffer(LEN_SEPARATOR, 1));
                    boost::asio::write(*socket, boost::asio::buffer(output, length));
                    CacheableFSProvider *cacheableProvider = dynamic_cast<CacheableFSProvider*>(&(*metadataServer->parser->fsProvider));
                    if (cacheableProvider != NULL) {
                        cacheableProvider->updateCaches();
                    }

                }
            } catch (boost::exception &e) {
                delete socket;
                if (output != NULL) {
                    //delete[] output;
                }
                break;
            }
            if (output != NULL) {
                //delete[] output;
            }
        } while (socket->is_open());
        if (socket->is_open()) {
            socket->close();
        }

        delete[] errOutput;
        delete socket;
    }

    MediaPacketParser :: MediaPacketParser(std::unique_ptr<FSProvider> &&fsProvider) : fsProvider(std::move(fsProvider)) {
        functionMap['r'] = [this] (std::vector<std::string> data, int &len) {
            int size = stoi(data[1]);
            len = size;
            int offset = stoi(data[2]);
            std::cout << "r " << data[0] << "\n";
            const char *output = this->fsProvider->read(data[0], len, offset);
            return output;
        };

        functionMap['d'] = [this] (std::vector<std::string> data, int &len) {
            std::vector<Attr> attrs = this->fsProvider->readDir(data[0]);
            std::string output = "";
            std::cout << "d " << data[0] << "\n";
            int sz = attrs.size() - 1;
            for (const auto &attr : attrs) {
                output += formatAttr(attr) + (sz-- > 0 ?  "\n" : "");
            }
            len = output.size();
            char *buf = new char[len + 1];
            strcpy(buf, output.c_str());
            return buf;
        };

        functionMap['g'] = [this] (std::vector<std::string> data, int &len) {
            std::string file = data[0];
            std::string attr = formatAttr(this->fsProvider->getAttr(file));
            std::cout << "g " << data[0] << "\n";
            len = attr.size();
            char *buf = new char[len + 1];
            strcpy(buf, attr.c_str());
            return buf;
        };
    }

    const char* MediaPacketParser :: parse(const char *data, int length, int &outputLength) {
        if (length == 0) {
            return NULL;
        }

        std::string content(data + 1, length - 1);
        std::vector<std::string> params = MediaFs::split(content, " ");
        std::string &param = params.back();
        if (param.back() == '\n') {
            param = param.substr(0, param.size() - 1);
        }
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
