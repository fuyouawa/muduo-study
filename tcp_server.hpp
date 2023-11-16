#pragma once
#include "core.hpp"
#include "inet_address.hpp"
#include "event_loop_thread_pool.hpp"
#include "callbacks.hpp"
#include "acceptor.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class TcpServer
{
public:
    MUDUO_STUDY_NONCOPYABLE(TcpServer)

    enum Option {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop, const InetAddress& listen_addr, std::string_view name, Option opt = kNoReusePort);
    ~TcpServer();

    auto ip_port() const { return ip_port_; }
    auto name() const { return name_; }
    auto loop() { return loop_; }
    auto thread_pool() { return thread_pool_; }

    void set_thread_num(int num);
    void set_thread_init_callback(ThreadInitCallBack cb) { thread_init_callback_ = std::move(cb); }
    void set_connection_callback(ConnectionCallback cb) { connection_callback_ = std::move(cb); }
    void set_message_callback(MessageCallback cb) { message_callback_ = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { write_complete_callback_ = std::move(cb); }

    void Start();

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    void NewConnection(int sockfd, const InetAddress& peer_addr);
    void RemoveConnection(const TcpConnectionPtr& conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    const std::string ip_port_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> thread_pool_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    ThreadInitCallBack thread_init_callback_;
    std::atomic_int32_t started_;
    int next_connid_;
    ConnectionMap connections_;
};

MUDUO_STUDY_END_NAMESPACE