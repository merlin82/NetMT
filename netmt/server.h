#ifndef _NETMT_SERVER_H_
#define _NETMT_SERVER_H_

#include <string>
#include <boost/thread.hpp>
#include "connection.h"

namespace netmt
{
class Server: private boost::noncopyable
{
public:
    /// Construct the server to listen on the specified TCP address and port
    explicit Server(const std::string& address, const std::string& port,
            std::size_t thread_pool_size);

    virtual ~Server();

    /// Run the server's io_service loop.
    void run();

    /// handle message
    virtual void handle_message(Connection_ptr conn, const char* data,
            std::size_t data_len) = 0;

    /// check message whether complete
    /// return 0:not complete, <0:error, >0:message length
    virtual int check_complete(Connection_ptr conn, const char* data,
            std::size_t data_len) = 0;

    /// handle connect event
    virtual void handle_connect(Connection_ptr conn);

    /// handle disconnect event
    virtual void handle_disconnect(Connection_ptr conn);

    /// handle completion of a write operation.
    void handle_write(Connection_ptr conn, const char* data,
            std::size_t data_len, const boost::system::error_code& e);

    /// get io_service
    boost::asio::io_service& io_service()
    {
        return m_io_service;
    }
private:
    /// Initiate an asynchronous accept operation.
    void start_accept();

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e);

    /// Handle a request to stop the server.
    void handle_stop();

    boost::thread_group m_thread_grp;

    /// The number of threads that will call io_service::run().
    std::size_t m_thread_pool_size;

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service m_io_service;

    /// The signal_set is used to register for process termination notifications.
    boost::asio::signal_set m_signals;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor m_acceptor;

    /// The next connection to be accepted.
    Connection_ptr m_new_connection;
};
}
#endif
