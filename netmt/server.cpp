#include "server.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

namespace netmt {

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
    m_signals.async_wait(boost::bind(&Server::handle_stop, this));

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(m_io_service);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();

    start_accept();
}

Server::~Server()
{

}

void Server::run()
{
    for (std::size_t i = 0; i < m_thread_pool_size; ++i)
    {
        m_thread_grp.create_thread(
                boost::bind(&boost::asio::io_service::run, &m_io_service));
    }

    m_thread_grp.join_all();
}

void Server::start_accept()
{
    m_new_connection.reset(new Connection(*this));
    m_acceptor.async_accept(*m_new_connection,
            boost::bind(&Server::handle_accept, this,
                    boost::asio::placeholders::error));
}

void Server::handle_accept(const boost::system::error_code& e)
{
    if (!e)
    {
        handle_connect(m_new_connection);
        m_new_connection->start();
    }

    start_accept();
}

void Server::handle_stop()
{
    m_io_service.stop();
}

void Server::handle_connect(Connection_ptr conn)
{
    
}

void Server::handle_disconnect(Connection_ptr conn)
{

}

}
