#include "server.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

namespace netmt
{

Server::Server(const std::string& address, const std::string& port,
        std::size_t thread_pool_size) :
        m_thread_pool_size(thread_pool_size), m_signals(m_io_service), m_acceptor(
                m_io_service), m_new_connection()
{
    // Register to handle the signals that indicate when the Server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);
#if defined(SIGQUIT)
    m_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
    m_signals.async_wait(boost::bind(&Server::HandleStop, this));

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(m_io_service);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::linger(true, 0));  
    m_acceptor.set_option(boost::asio::socket_base::keep_alive(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();

    StartAccept();
}

Server::~Server()
{

}

void Server::Run()
{
    for (std::size_t i = 0; i < m_thread_pool_size; ++i)
    {
        m_thread_grp.create_thread(
                boost::bind(&boost::asio::io_service::run, &m_io_service));
    }

    m_thread_grp.join_all();
}

void Server::StartAccept()
{
    m_new_connection.reset(new Connection(this));
    m_acceptor.async_accept(*m_new_connection,
            boost::bind(&Server::HandleAccept, this,
                    boost::asio::placeholders::error));
}

void Server::HandleAccept(const boost::system::error_code& e)
{
    if (!e)
    {
        HandleConnect(m_new_connection);
        m_new_connection->Start();
    }

    StartAccept();
}

void Server::HandleStop()
{
    m_io_service.stop();
}

void Server::HandleConnect(ConnectionPtr conn)
{

}

void Server::HandleDisconnect(ConnectionPtr conn)
{

}

void Server::HandleSendError(ConnectionPtr conn, const char* data, std::size_t data_len,
        const boost::system::error_code& e)
{

}

}
