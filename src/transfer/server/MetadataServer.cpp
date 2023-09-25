#include <iostream>
#include <thread>

#include "./MetadataServer.h"
#include "../../Utils.h"


namespace MediaFs {

    MetadataServer :: MetadataServer(int port, std::unique_ptr<FSProvider> &&client) : parser(std::move(std::unique_ptr<MediaPacketParser>(new MediaPacketParser(std::move(client))))), port(port)  { }

    void MetadataServer :: startListen() {
        boost::asio::io_service ioService;
        tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), port));
        while (true) {
            tcp::socket *socket = new tcp::socket(ioService);
            acceptor.accept(*socket);
            std::thread handle(handleClient, socket, this);
            handle.detach();
        }

    }
    
    void MetadataServer :: handleClient(tcp::socket *socket, MetadataServer *metadataServer) {

        std::cout << "__\n";
        boost::asio::streambuf buf;
        boost::asio::read_until(*socket, buf, "\n");
        std::string data = boost::asio::buffer_cast<const char*>(buf.data());
        int length;
        const char *output = metadataServer->parser->parse(data.c_str(), data.size(), length);
        //boost::asio::write(*socket, boost::asio::buffer(std::string("abcdef")));
        boost::asio::write(*socket, boost::asio::buffer(output, length));
        delete[] output;
        socket->close();

        std::cout << data << "\n";
        delete socket;
    }

    MediaPacketParser :: MediaPacketParser(std::unique_ptr<FSProvider> &&fsProvider) : fsProvider(std::move(fsProvider)) {
        functionMap['r'] = [this] (std::vector<std::string> data, int &len) {
            int size = stoi(data[1]);
            int offset = stoi(data[2]);
            char *buf = new char[size];
            int outputLength = this->fsProvider->read(data[0], buf, size, offset);
            std::cout << "output " << buf << "\n";
            len = outputLength;
            return buf;
        };

        functionMap['d'] = [this] (std::vector<std::string> data, int &len) {
            std::vector<Attr> attrs = this->fsProvider->readDir(data[0]);
            std::string output = "";
            for (const auto &attr : attrs) {
                output += formatAttr(attr) + "\n";
            }
            len = output.size();
            return output.c_str();
        };

        functionMap['g'] = [this] (std::vector<std::string> data, int &len) {
            std::string file = data[0];
            std::string attr = formatAttr(this->fsProvider->getAttr(file));
            len = attr.size();
            return attr.c_str();
        };
    }

    const char* MediaPacketParser :: parse(const char *data, int length, int &outputLength) {
        if (length == 0) {
            return NULL;
        }

        std::string content(data + 1, length - 1);
        std::vector<std::string> params = MediaFs::split(content.substr(1), " ");
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


};

