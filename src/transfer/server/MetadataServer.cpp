#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "./TransferServer.h"

using boost::asio::ip::tcp;

namespace MediaFs {

    MediaPacketParser :: MediaPacketParser(int port, std::unique_ptr<FSProvider>) : port(port), client(client)  { }

    void MediaPacketParser :: startListen() {
        boost::asio::io_service ioService;
        tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), port));
        tcp::socket socket(ioService);
        acceptor.accept(socket);

        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, "\n");
        std::string data = boost::asio::buffer_cast<const char*>(buf.data());
        boost::asio::write(socket, boost::asio::buffer(std::string("abcdef")));

        std::cout << data << "\n";

    }
};


