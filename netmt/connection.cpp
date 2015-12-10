#include <boost/bind.hpp>
#include "server.h"
#include "connection.h"

namespace netmt
{

Connection::Connection(Server& server) :
        boost::asio::ip::tcp::socket(server.io_service()), m_server(server)
{
    m_buffer = NULL;
    m_buffer_len = DEFAULT_BUFFER_LEN;

    m_data = NULL;
    m_data_len = 0;
}

Connection::~Connection()
{
    if (m_buffer)
    {
        delete[] m_buffer;
        m_buffer = NULL;
    }
}

void Connection::SetBufferLen(std::size_t buffer_len)
{
    if (buffer_len > 0 && NULL == m_buffer)
    {
        m_buffer_len = buffer_len;
    }
}

void Connection::Start()
{
    m_buffer = new char[m_buffer_len];
    m_data = m_buffer;

    async_read_some(
            boost::asio::buffer(m_data + m_data_len,
                    m_buffer + m_buffer_len - m_data - m_data_len),
            boost::bind(&Connection::HandleRead, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void Connection::HandleRead(const boost::system::error_code& e,
        std::size_t bytes_transferred)
{
    if (!e)
    {
        m_data_len += bytes_transferred;
        int ret = m_server.CheckComplete(shared_from_this(), m_data,
                m_data_len);
        if (ret > 0)
        {
            m_server.HandleMessage(shared_from_this(), m_data, ret);
            m_data += ret;
            m_data_len -= ret;
        }
        else if (ret < 0)
        {
            return;
        }

        if (m_buffer + m_buffer_len - m_data - m_data_len == 0)
        {
            if (m_buffer == m_data)
            {
                return;
            }

            memmove(m_buffer, m_data, m_data_len);
            m_data = m_buffer;
        }

        async_read_some(
                boost::asio::buffer(m_data + m_data_len,
                        m_buffer + m_buffer_len - m_data - m_data_len),
                boost::bind(&Connection::HandleRead, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        close();
        m_server.HandleDisconnect(shared_from_this());
    }
}

void Connection::HandleConnect(const char* data,
        std::size_t data_len, const boost::system::error_code& e)
{
    if (!e)
    {
        async_send(boost::asio::buffer(data, data_len),
                boost::bind(&Server::HandleWrite, &m_server, shared_from_this(),
                        data, data_len, boost::asio::placeholders::error));
    }
}

int Connection::AsyncSend(const char* data, uint32_t data_len)
{
    if (is_open())
    {
        async_send(boost::asio::buffer(data, data_len),
                boost::bind(&Server::HandleWrite, &m_server, shared_from_this(),
                        data, data_len, boost::asio::placeholders::error));        
    }
    else
    {
        close();

        boost::asio::ip::tcp::resolver resolver(m_io_service);
        boost::asio::ip::tcp::resolver::query query(address, port);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    
        async_connect(endpoint, boost::bind(&Connection::HandleConnect, shared_from_this(),
                        data, data_len, boost::asio::placeholders::error));
    }
    return 0;
}

}
