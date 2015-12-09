#ifndef _NETMT_CONNECTION_
#define _NETMT_CONNECTION_

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace netmt
{

const int DEFAULT_BUFFER_LEN = 8192;

class Server;

class Connection: public boost::asio::ip::tcp::socket,
        public boost::enable_shared_from_this<Connection>,
        private boost::noncopyable
{
    friend class Server;
public:
    ~Connection();

    /// Set receive buffer size, only can set in handle_connect.
    void set_buffer_len(std::size_t buffer_len);

    int SendAndRecv(const char* data, uint32_t data_len, char*& rsp_data,
            uint32_t& rsp_data_len, int timeout_ms = 10000);

    int AsyncSend(const char* data, uint32_t data_len);
private:
    /// Construct a Connection with the given io_service.
    explicit Connection(Server& server);

    /// Start the first asynchronous operation for the Connection.
    void start();

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
            std::size_t bytes_transferred);
private:
    Server& m_server;

    /// Buffer for incoming data.
    char* m_buffer;
    std::size_t m_buffer_len;

    /// current receive data
    char* m_data;
    std::size_t m_data_len;
};

typedef boost::shared_ptr<Connection> Connection_ptr;
}

#endif
