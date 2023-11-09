#pragma once
#include "logger.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

MUDUO_STUDY_BEGIN_NAMESPACE

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, bool loop_back_only=false) {
        ZeroMemory(addr_);
        addr_.sin_family = AF_INET;
        auto ip = loop_back_only ? INADDR_ANY : INADDR_LOOPBACK;
        addr_.sin_addr.s_addr = htonl(ip);
        addr_.sin_port = htons(port);
    }

    InetAddress(std::string_view ip, uint16_t port) {
        ZeroMemory(addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        auto err = inet_pton(AF_INET, ip.data(), &addr_.sin_addr);
        if (err == 0) {
            MUDUO_STUDY_LOG_ERROR("Invalid netword address: {}", ip);
        }
        else if (err == -1) {
            MUDUO_STUDY_LOG_SYSERR("inet_pton(...) failed!");
        }
    }

    std::string ip() {
        char tmp[INET_ADDRSTRLEN];
        return inet_ntop(AF_INET, &addr_.sin_addr, tmp, sizeof(tmp));
    }
    std::string ip_port() {
        return fmt::format("{}:{}", ip(), ntohs(addr_.sin_port));
    }
    const sockaddr_in* sockaddr() const noexcept {  return &addr_; }

    void set_sockaddr(const sockaddr_in& addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};


MUDUO_STUDY_END_NAMESPACE