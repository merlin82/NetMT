#include "async_client.h"
#include "server.h"

using namespace std;
namespace netmt
{

AsyncConnection::AsyncConnection(Server* server, const std::string& ip, uint16_t port) : Connection(server), m_ip(ip), m_port(port)
{
    
}

AsyncConnection::~AsyncConnection()
{
    
}

void AsyncConnection::Send(const char* data, uint32_t data_len)
{
    DataPtr data_ptr(new string(data, data_len));
    if (is_open())
    {
        async_send(boost::asio::buffer(*data_ptr),
                m_strand.wrap(boost::bind(&AsyncConnection::HandleSend, this,
                        data_ptr, boost::asio::placeholders::error, true)));        
    }
    else
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_ip), m_port);
        async_connect(endpoint, m_strand.wrap(boost::bind(&AsyncConnection::HandleConnect, this,
                data_ptr, boost::asio::placeholders::error)));
    }
}

void AsyncConnection::HandleConnect(DataPtr data_ptr, const boost::system::error_code& e)
{
    if (!e)
    {
        m_server->HandleConnect(Connection::shared_from_this());
        Start();
        async_send(boost::asio::buffer(*data_ptr),
                m_strand.wrap(boost::bind(&AsyncConnection::HandleSend, this,
                        data_ptr, boost::asio::placeholders::error, false))); 
    }
}

void AsyncConnection::HandleSend(DataPtr data_ptr, const boost::system::error_code& e, bool resend)
{
    if (e)
    {
        if (resend)
        {
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_ip), m_port);
            async_connect(endpoint, m_strand.wrap(boost::bind(&AsyncConnection::HandleConnect, this,
                data_ptr, boost::asio::placeholders::error)));             
        }
        else
        {
            m_server->HandleSendError(Connection::shared_from_this(), data_ptr->c_str(), data_ptr->size(), e);
        }
    }
}

ASyncClient ASyncClient::s_client;
ASyncClient* ASyncClient::Instance()
{
    return &s_client;
}
ASyncClient::ASyncClient()
{
    
}
ASyncClient::~ASyncClient()
{
    
}

int ASyncClient::Send(Server* server, const std::string& ip, uint16_t port, const char* data,
            uint32_t data_len)
{
    AsyncConnectionPtr conn = GetConnection(server, ip, port);
    conn->Send(data, data_len);
    return 0;    
}

AsyncConnectionPtr ASyncClient::GetConnection(Server* server, const std::string& ip, uint16_t port)
{
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
    AsyncConnectionPtr conn;
    boost::mutex::scoped_lock lock(m_mutex); 
    map<boost::asio::ip::tcp::endpoint, AsyncConnectionPtr>::iterator it = m_conn_map.find(endpoint);
    if (it == m_conn_map.end())
    {
        conn.reset(new AsyncConnection(server, ip, port));
        m_conn_map[endpoint] = conn;       
    }
    else
    {
        conn = it->second;
    }
    
    return conn;
}

}
