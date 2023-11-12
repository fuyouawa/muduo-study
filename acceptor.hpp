#pragma once
#include "core.hpp"
#include "inet_address.hpp"
#include "event_loop.hpp"
#include "socket.hpp"


MUDUO_STUDY_BEGIN_NAMESPACE

class Acceptor
{
public:
    MUDUO_STUDY_NONCOPYABLE(Acceptor)
    using NewConnectionCallback = std::move_only_function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port) :
        loop_{loop},
        accept_socket_{::socket(listen_addr.family(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)},
        accept_channel_{loop, accept_socket_.fd()},
        listening_{false}
        {
            if (accept_socket_.fd() == -1) {
                MUDUO_STUDY_LOG_SYSFATAL("socket() failed!");
            }
            accept_socket_.set_reuse_addr(true);
            accept_socket_.set_reuse_port(reuse_port);
            accept_socket_.BindAddress(listen_addr);
            accept_channel_.set_read_callback([this](auto){ this->HandleRead(); });
        }
    ~Acceptor() {
        accept_channel_.DisableAll();
        accept_channel_.Remove();
    }

    auto listening() const noexcept { return listening_; }
    void set_new_connection_callback(NewConnectionCallback cb) { new_connection_callback_ = std::move(cb); }

    void Listen() {
        loop_->AssertInLoopThread();
        listening_ = true;
        accept_socket_.Listen();
        accept_channel_.EnableReading();
    }
private:
    void HandleRead() {
        loop_->AssertInLoopThread();
        InetAddress peer_addr;
        auto connfd = accept_socket_.Accept(&peer_addr);
        if (new_connection_callback_)
            new_connection_callback_(connfd, peer_addr);
        else
            ::close(connfd);
    }

    EventLoop* loop_;
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listening_;
};


MUDUO_STUDY_END_NAMESPACE