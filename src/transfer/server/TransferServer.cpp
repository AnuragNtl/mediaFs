#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "./TransferServer.h"

using boost::asio::ip::udp;




namespace MediaFs {
    TransferServer :: TransferServer(int port) : port(port) { }
};


