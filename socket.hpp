#pragma once
#include "core.hpp"
#include "inet_address.hpp"
#include <netinet/tcp.h>

MUDUO_STUDY_BEGIN_NAMESPACE

class Socket
{
public:
    explicit Socket(int sockfd) :
        sockfd_{sockfd}
    {}
    ~Socket() {
        ::close(sockfd_);
    }

    int fd() const { return sockfd_; }
    auto tcp_info() const -> std::expected<::tcp_info, int> {
        ::tcp_info tcpi;
        ZeroMemory(tcpi);
        socklen_t len = sizeof(tcpi);
        auto ret = ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, &tcpi, &len);
        if (ret == 0) return tcpi;
        return std::unexpected(errno);
    }
    int socket_error() {
        int optval;
        socklen_t optlen = sizeof(optval);
        if (::getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0) {
            return optval;
        }
        else {
            return errno;
        }
    }
    auto local_addr() -> std::optional<sockaddr_in> {
        sockaddr_in addr;
        ZeroMemory(addr);
        socklen_t addrlen = sizeof(addr);
        if (::getsockname(sockfd_, (sockaddr*)&addr, &addrlen) == -1) {
            MUDUO_STUDY_LOG_SYSERR("::getsockname failed!");
            return std::nullopt;
        }
        return addr;
    }

    void set_tcp_no_delay(bool b) {
        int optval = b ? 1 : 0;
        ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
    }
    void set_reuse_addr(bool b) {
        int optval = b ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }
    void set_reuse_port(bool b) {
        int optval = b ? 1 : 0;
        int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if (ret < 0 && b) {
            MUDUO_STUDY_LOG_SYSERR("setsockopt SO_REUSEPORT failed!");
        }
    }
    void set_keep_alive(bool b) {
        int optval = b ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

    void BindAddress(const InetAddress& local_addr) {
        auto ret = ::bind(sockfd_, (sockaddr*)local_addr.sockaddr(), sizeof(sockaddr_in));
        if (ret == -1) {
            MUDUO_STUDY_LOG_SYSFATAL("bind() failed!");
        }
    }
    void Listen() {
        if (::listen(sockfd_, SOMAXCONN) == -1) {
            MUDUO_STUDY_LOG_SYSFATAL("listen() failed!");
        }
    }
    int Accept(InetAddress* peeraddr) {
        socklen_t addrlen = sizeof(sockaddr_in);
        sockaddr_in addr;
        auto connfd = ::accept4(sockfd_, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (connfd == -1) {
            MUDUO_STUDY_LOG_SYSFATAL("accept4() failed!");
        }
        else {
            peeraddr->set_sockaddr(addr);
        }
        return connfd;
    }
    void ShutDownWrite() {
        if (::shutdown(sockfd_, SHUT_WR) == -1) {
            MUDUO_STUDY_LOG_SYSERR("shutdown() failed!");
        }
    }
private:
    const int sockfd_;
};

MUDUO_STUDY_END_NAMESPACE