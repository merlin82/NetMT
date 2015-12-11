#ifndef _NETMT_ASYNC_CLIENT_H_
#define _NETMT_ASYNC_CLIENT_H_

#include "connection.h"
#include <boost/thread/mutex.hpp>
#include <vector>

namespace netmt
{

class AsyncConnection: public Connection
{
public:
    explicit AsyncConnection(Server* server, const std::string& ip, uint16_t port);
    
    virtual ~AsyncConnection();

    void Send(const char* data, uint32_t data_len);

private:
    void HandleConnect(DataPtr data_ptr, const boost::system::error_code& e); 

    void HandleSend(DataPtr data_ptr, const boost::system::error_code& e, bool resend);

private:
    std::string m_ip;
    uint16_t m_port;    
};
typedef boost::shared_ptr<AsyncConnection> AsyncConnectionPtr;

class ASyncClient: private boost::noncopyable
{
public:
    ASyncClient();
    ~ASyncClient();

    static ASyncClient* Instance();

    int Send(Server* server, const std::string& ip, uint16_t port, const char* data,
                uint32_t data_len);
private:
    AsyncConnectionPtr GetConnection(Server* server, const std::string& ip, uint16_t port);
    
private:
    static ASyncClient s_client;
    std::map<boost::asio::ip::tcp::endpoint, AsyncConnectionPtr> m_conn_map;
    boost::mutex m_mutex;
};
}
#endif
