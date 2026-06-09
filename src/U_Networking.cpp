#include "U_Networking.h"

unsigned short Networking::HostPortsBegin = 10230;
unsigned short Networking::HostPortsEnd = 10234;

unsigned short Networking::ClientPortsBegin = 10235;
unsigned short Networking::ClientPortsEnd = 10240;

unsigned short Networking::HostPort = Networking::HostPortsBegin;
unsigned short Networking::ClientPort = Networking::ClientPortsBegin;

int Networking::ParsePort(const std::string& port_str)
{
    int port = std::stoi(port_str);
    if (port < 1 || port > 65535)
    {
        return Networking::HostPort; //means invalid (default)
    }
    return port;
}

bool Networking::ParseEndpoint(boost::asio::ip::udp::endpoint& endpoint, boost::asio::io_context& io_context, const std::string& input, int default_port)
{
    std::string ip_part;
    std::string port_part;
    std::string::size_type delim_pos;

    if (input.front() == '[')
    {
        delim_pos = input.find(']');
        if (delim_pos == std::string::npos)
        {
            return false;
        }
        ip_part = input.substr(1, delim_pos - 1);

        if (input.length() > delim_pos + 2 && input[delim_pos + 1] == ':')
        {
            port_part = input.substr(delim_pos + 2);
        }
    }
    else
    {
        delim_pos = input.find_first_of(": ");
        if (delim_pos != std::string::npos)
        {
            ip_part = input.substr(0, delim_pos);
            port_part = input.substr(delim_pos + 1);
        }
        else
        {
            ip_part = input;
        }
    }

    int port = default_port;
    if (!port_part.empty())
    {
        port = Networking::ParsePort(port_part);
    }

    boost::system::error_code ec;
    boost::asio::ip::address ip_address = boost::asio::ip::make_address(ip_part, ec);

    if (!ec)
    {
        endpoint = boost::asio::ip::udp::endpoint(ip_address, port);
    }
    else
    {
        boost::asio::ip::udp::resolver resolver(io_context);

        boost::system::error_code err;
        auto endpoints = resolver.resolve(ip_part, std::to_string(port), err);

        if (err.value() != 0)
        {
            return false;
        }

        endpoint = (*endpoints.begin()).endpoint();
    }
    return true;
}
