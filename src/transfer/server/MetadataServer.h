#ifndef MEDIA_PACKET_PARSER_H

#define MEDIA_PACKET_PARSER_H

#include "../../fsprovider/FSProvider.h"

#include <memory>
#include <map>
#include <boost/asio.hpp>
#include <iostream>

#include "../../Utils.h"

using boost::asio::ip::tcp;

namespace MediaFs {
    class MediaPacketParser;
    class MetadataServer {
        private:
            int port;
            std::atomic<bool> running;
            std::unique_ptr<MediaPacketParser> parser;
            boost::asio::io_service ioService;
            static void handleClient(tcp::socket *socket, MetadataServer *);
        public:
            MetadataServer(const MetadataServer &) = delete;
            MetadataServer(int port, std::unique_ptr<FSProvider> &&);
            void startListen();
            void stopServer();
    };

    class MediaPacketParser {
        private:
            static std::map<const char, std::function<const char *(std::vector<std::string>, int &)> > functionMap;
            std::string formatAttr(const Attr &) const;
            std::unique_ptr<FSProvider> fsProvider;
            MediaPacketParser(std::unique_ptr<FSProvider> &&);
            const char* parse(const char *, int length, int &outputLength);
            friend class MetadataServer;
        public:
            static std::string generate(const char id, std::vector<std::string>);
    };
};

#endif

