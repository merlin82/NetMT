#ifndef _NETMT_SYNC_CLIENT_H_
#define _NETMT_SYNC_CLIENT_H_

#include <stdint.h>
#include <string>
#include <map>
#include "boost/shared_ptr.hpp"
#include "boost/asio.hpp"
#include "boost/function.hpp"
#include "boost/thread.hpp"

namespace netmt
{

class SyncConnection: public boost::asio::ip::tcp::socket
{
public:
    explicit SyncConnection(boost::asio::io_service& io_service,
            const std::string& ip, uint16_t port) :
            boost::asio::ip::tcp::socket(io_service), m_ip(ip), m_port(port)
    {

    }
    const std::string& GetIP() const
    {
        return m_ip;
    }
    uint16_t GetPort() const
    {
        return m_port;
    }
private:
    std::string m_ip;
    uint16_t m_port;
};

typedef boost::shared_ptr<SyncConnection> SyncConnectionPtr;

/// 同步客户端
class SyncClient
{
public:
    SyncClient();
    ~SyncClient();

    /// 获取线程级单例，每线程一个实例，用于长连接缓存，短连接直接申明
    static SyncClient* Instance();
public:
    typedef boost::function<int(const char* data, uint32_t data_len)> CompleteFunc;
    int SendAndRecv(const std::string& ip, uint16_t port, const char* data,
            uint32_t data_len, char*& rsp_data, uint32_t& rsp_data_len,
            int timeout_ms, CompleteFunc func);

private:
    void ReConnect();
    void SetTimer(int timeout_ms);
    void DoSend(bool reconnect);
    void DoError(int result);

    void HandleConnect(const boost::system::error_code& error);
    void HandleTimeOut(const boost::system::error_code& error);
    void HandleSend(bool reconnect, const boost::system::error_code& error);
    void HandleRecv(const boost::system::error_code& error,
            std::size_t bytes_transferred);
private:
    SyncConnectionPtr GetConnection(const std::string& ip, uint16_t port);
private:
    std::map<std::string, SyncConnectionPtr> m_conn_map;
    boost::asio::io_service m_io_service;
    boost::asio::deadline_timer m_timer;
    const char* m_send_ptr;
    char* m_recv_buffer;
    uint32_t m_send_size;
    uint32_t m_recv_size;
    SyncConnectionPtr m_handle_conn;
    int m_result;
    CompleteFunc m_complete_func;
    static boost::thread_specific_ptr<SyncClient> m_client;
};

}

#endif
