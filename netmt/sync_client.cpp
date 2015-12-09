#include "sync_client.h"
#include "boost/bind.hpp"

using namespace std;

namespace netmt
{

///////////////////////////////////////////////////////////////////////////////

const uint32_t MAX_DATA_SIZE = 2 * 1024 * 1024;

SyncClient::SyncClient() :
        m_timer(m_io_service)
{
    m_send_ptr = NULL;
    m_recv_buffer = new char[MAX_DATA_SIZE];
    m_send_size = 0;
    m_recv_size = 0;
    m_result = boost::system::errc::success;
}

SyncClient::~SyncClient()
{
    m_send_ptr = NULL;
    delete[] m_recv_buffer;
    m_recv_buffer = NULL;
    m_handle_conn.reset();
    m_conn_map.clear();
}

boost::thread_specific_ptr<SyncClient> SyncClient::m_client;
SyncClient* SyncClient::Instance()
{
    if (m_client.get() == NULL)
    {
        m_client.reset(new SyncClient());
    }
    return m_client.get();
}

int SyncClient::SendAndRecv(const std::string& ip, uint16_t port,
        const char* data, uint32_t data_len, char*& rsp_data,
        uint32_t& rsp_data_len, int timeout_ms, CompleteFunc func)
{
    //LOG(INFO, "SyncClient::SendAndRecv, ip : %s, port : %d, timeout_ms : %d.",
    //        ip.c_str(), port, timeout_ms);

    try
    {
        m_io_service.reset();
        m_handle_conn = GetConnection(ip, port);
        m_send_ptr = data;
        m_send_size = data_len;
        m_recv_size = 0;
        m_complete_func = func;
        SetTimer(timeout_ms);
        //第一次发送失败尝试重连
        DoSend(true);
        m_io_service.run();
        if (m_result != 0)
        {
            //LOG(ERROR, "SyncClient::SendAndRecv failed, errno : %d.", m_result);
            return -1;
        }
        rsp_data = m_recv_buffer;
        rsp_data_len = m_recv_size;
    } catch (...)
    {
        //LOG(ERROR, "SyncClient::SendAndRecv failed, catch exception.");
        return -1;
    }
    return 0;
}

SyncConnectionPtr SyncClient::GetConnection(const std::string& ip,
        uint16_t port)
{
    string key = ip;
    key += ":";
    key += port;

    SyncConnectionPtr conn;
    map<string, SyncConnectionPtr>::iterator it = m_conn_map.find(key);
    if (it == m_conn_map.end())
    {
        conn.reset(new SyncConnection(m_io_service, ip, port));
        m_conn_map[key] = conn;
    }
    else
    {
        conn = it->second;
    }
    return conn;
}

void SyncClient::DoSend(bool reconnect)
{
    if (m_handle_conn->is_open())
    {
        m_handle_conn->async_send(boost::asio::buffer(m_send_ptr, m_send_size),
                boost::bind(&SyncClient::HandleSend, this, reconnect,
                        boost::asio::placeholders::error));
    }
    else
    {
        if (reconnect)
        {
            ReConnect();
        }
        else
        {
            DoError(-1);
        }
    }
}

void SyncClient::DoError(int result)
{
    m_result = result;
    m_timer.cancel();
    m_handle_conn->close();
}
void SyncClient::ReConnect()
{
    m_handle_conn->close();
    boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::address::from_string(m_handle_conn->GetIP()),
            m_handle_conn->GetPort());
    m_handle_conn->async_connect(endpoint,
            boost::bind(&SyncClient::HandleConnect, this,
                    boost::asio::placeholders::error));
}

void SyncClient::SetTimer(int timeout_ms)
{
    m_result = boost::system::errc::success;
    m_timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms));
    m_timer.async_wait(
            boost::bind(&SyncClient::HandleTimeOut, this,
                    boost::asio::placeholders::error));
}

void SyncClient::HandleConnect(const boost::system::error_code& error)
{
    if (m_result)
    {
        //LOG(ERROR, "SyncClient::HandleConnect failed, connect timeout.");
    }
    else
    {
        if (!error)
        {
            //这次发送不再重连
            DoSend(false);
        }
        else
        {
            //LOG(ERROR, "SyncClient::HandleConnect failed, %s.",
            //        error.message().c_str());
            DoError(error.value());
        }
    }
}

void SyncClient::HandleSend(bool reconnect,
        const boost::system::error_code& error)
{
    if (!error)
    {
        m_handle_conn->async_read_some(
                boost::asio::buffer(m_recv_buffer, MAX_DATA_SIZE),
                boost::bind(&SyncClient::HandleRecv, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        if (reconnect)
        {
            //LOG(WARN, "SyncClient::HandleSend failed, %s, reconnect it.",
            //        error.message().c_str());
            ReConnect();
        }
        else
        {
            //LOG(ERROR, "SyncClient::HandleSend failed, %s.",
            //        error.message().c_str());
            DoError(error.value());
        }
    }

}

void SyncClient::HandleRecv(const boost::system::error_code& error,
        std::size_t bytes_transferred)
{
    if (!error)
    {
        m_recv_size += bytes_transferred;
        int ret = m_complete_func(m_recv_buffer, m_recv_size);
        if (ret < 0)
        {
            DoError(-1);
        }
        if (ret > 0)
        {
            if (m_recv_size != (uint32_t)ret)
            {
                //LOG(ERROR,
                //        "SyncClient::HandleRecv failed, recv package length(%d) is not equal real length(%d).",
                //        m_recv_size, ret);
                DoError(-1);
            }
            else
            {
                m_timer.cancel();
            }
        }
        else
        {
            if (m_recv_size == MAX_DATA_SIZE)
            {
                //LOG(ERROR,
                //        "SyncClient::HandleRecv failed, package length is invalid.");
                DoError(-1);
                return;
            }

            m_handle_conn->async_read_some(
                    boost::asio::buffer(m_recv_buffer + m_recv_size,
                            MAX_DATA_SIZE - m_recv_size),
                    boost::bind(&SyncClient::HandleRecv, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        }
    }
    else
    {
        //LOG(ERROR, "SyncClient::HandleRecv failed, %s.",
        //        error.message().c_str());
        DoError(error.value());
    }
}

void SyncClient::HandleTimeOut(const boost::system::error_code& error)
{
    if (!error)
    {
        //LOG(ERROR, "SyncClient::HandleTimeOut, execute timeout.");
        m_result = boost::system::errc::timed_out;
        m_handle_conn->close();
    }
}

}

