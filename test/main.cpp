#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "netmt/server.h"
using namespace netmt;
using namespace std;

class EchoServer : public netmt::Server
{
public:
    /// Construct the server to listen on the specified TCP address and port
    explicit EchoServer(const std::string& address, const std::string& port,
            std::size_t thread_pool_size) : netmt::Server(address, port, thread_pool_size)
            {}

    /// handle connect event
    void HandleConnect(ConnectionPtr conn)
    {
        // test max receive buffer length
        conn->SetBufferLen(1024);
        cout << conn->remote_endpoint() << " connect" << endl;
    }
 
    /// handle disconnect event
    void HandleDisconnect(ConnectionPtr conn)
    {
        cout << conn->remote_endpoint() << " disconnect"  << endl;
    }
    
    /// handle message request
    void HandleMessage(ConnectionPtr conn, const char* data, std::size_t data_len)
    {
        conn->AsyncSend(data, data_len);
    }

    /// check message whether complete
    /// return 0:not complete, <0:error, >0:message length
    int CheckComplete(ConnectionPtr conn, const char* data, std::size_t data_len)
    {
        for (std::size_t i = 0; i < data_len; ++i)
        {
            if (data[i] == '\n' || data[i] == '\0')
            {
                return i + 1;
            }
        }
        return 0;
    }
};

int main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 4)
        {
            cerr << "Usage: http_server <address> <port> <threads>\n";
            cerr << "  For IPv4, try:\n";
            cerr << "    receiver 0.0.0.0 80 1 \n";
            cerr << "  For IPv6, try:\n";
            cerr << "    receiver 0::0 80 1 \n";
            return 1;
        }

        // Initialise the server.
        size_t num_threads = boost::lexical_cast < std::size_t > (argv[3]);
        EchoServer s(argv[1], argv[2], num_threads);

        // Run the server until stopped.
        s.Run();
    } catch (std::exception& e)
    {
        cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}
