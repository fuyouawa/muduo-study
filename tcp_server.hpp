#pragma once
#include "core.hpp"
#include "inet_address.hpp"
#include "event_loop_thread_pool.hpp"
#include "callbacks.hpp"
#include "acceptor.hpp"
#include "tcp_connection.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

namespace details {
void DefaultConnectionCallback(const TcpConnectionPtr conn) {
    MUDUO_STUDY_LOG_DEBUG("{} -> {} is {}",
                            conn->local_addr().ip_port(),
                            conn->peer_addr().ip_port(),
                            conn->connected() ? "UP" : "DOWN");
}
void DefaultMessageCallback(const TcpConnectionPtr conn, Buffer* buf, time_point receive_time) {
    buf->RetrieveAll();
}
}

class TcpServer
{
public:
    MUDUO_STUDY_NONCOPYABLE(TcpServer)

    enum Option {
        kNoReusePort,
        kReusePort
    };

    explicit TcpServer(EventLoop* loop, const InetAddress& listen_addr, std::string_view name, Option opt = kNoReusePort) :
        loop_{loop},
        ip_port_{listen_addr.ip_port()},
        name_{name},
        acceptor_{new Acceptor(loop_, listen_addr, opt == kReusePort)},
        thread_pool_{new EventLoopThreadPool(loop_, name)},
        connection_callback_{details::DefaultConnectionCallback},
        message_callback_{details::DefaultMessageCallback},
        next_connid_{1},
        started_{false}
    {
        acceptor_->set_new_connection_callback([this](auto sockfd, auto peer_addr){
            NewConnection(sockfd, peer_addr);
        });
    }
    ~TcpServer() {
        loop_->AssertInLoopThread();
        MUDUO_STUDY_LOG_DEBUG("tcp server [{}] destructing", name_);
        for (decltype(auto) item : connections_) {
            TcpConnectionPtr conn{item.second};
            item.second.reset();
            conn->loop()->RunInLoop([conn](){ conn->ConnectDestroyed(); });
        }
    }

    auto ip_port() const { return ip_port_; }
    auto name() const { return name_; }
    auto loop() { return loop_; }
    auto thread_pool() { return thread_pool_; }

    void set_thread_num(size_t num) { thread_pool_->set_thread_num(num); }
    void set_thread_init_callback(ThreadInitCallBack cb) { thread_init_callback_ = std::move(cb); }
    void set_connection_callback(ConnectionCallback cb) { connection_callback_ = std::move(cb); }
    void set_message_callback(MessageCallback cb) { message_callback_ = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { write_complete_callback_ = std::move(cb); }

    void Start() {
        if (!started_) {
            started_ = true;
            thread_pool_->Start(thread_init_callback_);
            assert(!acceptor_->listening());
            loop_->RunInLoop([this](){ acceptor_->Listen(); });
        }
    }

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    void NewConnection(int sockfd, const InetAddress& peer_addr) {
        loop_->AssertInLoopThread();
        auto ioloop = thread_pool_->next_loop();
        auto conn_name = std::format("{}-{}#{}", name_, ip_port_, next_connid_);
        ++next_connid_;
        MUDUO_STUDY_LOG_INFO("new connection [{}] from {}", conn_name, peer_addr.ip_port());
        if (auto opt = Socket(sockfd).local_addr(); opt.has_value()) {
            InetAddress local_addr{ opt.value() };
            auto conn = std::make_shared<TcpConnection>(loop_, conn_name, sockfd, local_addr, peer_addr);
            connections_[conn_name] = conn;
            conn->set_connection_callback(connection_callback_);
            conn->set_message_callback(message_callback_);
            conn->set_write_complete_callback(write_complete_callback_);
            conn->set_close_callback([this](auto ptr){ RemoveConnection(ptr); });
            ioloop->RunInLoop([conn](){ conn->ConnectEstablished(); });
        }
    }
    void RemoveConnection(const TcpConnectionPtr& conn) {
        loop_->RunInLoop([this, conn](){
            RemoveConnectionInLoop(conn);
        });
    }
    void RemoveConnectionInLoop(const TcpConnectionPtr& conn) {
        loop_->AssertInLoopThread();
        MUDUO_STUDY_LOG_INFO("remove connection {}", conn->name());
        auto n = connections_.erase(conn->name());
        assert(n == 1);
        auto ioloop = conn->loop();
        ioloop->QueueInLoop([conn](){ conn->ConnectDestroyed(); });
    }

    EventLoop* loop_;
    const std::string ip_port_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> thread_pool_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    ThreadInitCallBack thread_init_callback_;
    int next_connid_;
    ConnectionMap connections_;
    bool started_;
};

MUDUO_STUDY_END_NAMESPACE