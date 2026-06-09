#pragma once
#include "boost/asio.hpp"

using CEndPoint = boost::asio::ip::udp::endpoint;

namespace Networking
{
    extern unsigned short HostPortsBegin;
    extern unsigned short HostPortsEnd;

    extern unsigned short ClientPortsBegin;
    extern unsigned short ClientPortsEnd;

    extern unsigned short HostPort;
    extern unsigned short ClientPort;

    int ParsePort(const std::string& port_str);
    bool ParseEndpoint(boost::asio::ip::udp::endpoint& endpoint, boost::asio::io_context& io_context, const std::string& input, int default_port);
}
