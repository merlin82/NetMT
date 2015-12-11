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

typedef boost::shared_ptr<std::string> DataPtr;

class Connection: public boost::asio::ip::tcp::socket,
        public boost::enable_shared_from_this<Connection>,
        private boost::noncopyable
{
public:
    /// Construct a Connection with the given io_service.
    explicit Connection(Server* server);
    
    virtual ~Connection();

    /// Set receive buffer size, only can set in handle_connect.
    void SetBufferLen(std::size_t buffer_len);

    int AsyncSend(const char* data, uint32_t data_len);

    /// Start the first asynchronous operation for the Connection.
    void Start();

protected:
    /// Handle completion of a read operation.
    void HandleRead(const boost::system::error_code& e,
            std::size_t bytes_transferred);

    /// Handle completion of a send operation.
    void HandleSend(DataPtr data_ptr, const boost::system::error_code& e); 
protected:
    Server* m_server;

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand m_strand;    

    /// Buffer for incoming data.
    char* m_buffer;
    std::size_t m_buffer_len;

    /// current receive data
    char* m_data;
    std::size_t m_data_len;

};

typedef boost::shared_ptr<Connection> ConnectionPtr;
}

#endif
