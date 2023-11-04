#include "logger.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

MUDUO_STUDY_BEGIN_NAMESPACE

class InetAddress
{
public:
    InetAddress(std::string_view ip, uint16_t port) {
        ZeroMemory(addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        auto err = inet_pton(AF_INET, ip.data(), &addr_.sin_addr);
        if (err == 0) {
            MUDUO_STUDY_LOG_ERROR("Invalid netword address: {}", ip);
        }
        else if (err == -1) {
            auto e = errno;
            MUDUO_STUDY_LOG_ERROR("{}-{}", strerror(e), e);
        }
    }

    std::string ip() {
        char tmp[INET_ADDRSTRLEN];
        return inet_ntop(AF_INET, &addr_.sin_addr, tmp, sizeof(tmp));
    }

    std::string ip_port() {
        return fmt::format("{}:{}", ip(), ntohs(addr_.sin_port));
    }

    const sockaddr_in& sockaddr() const noexcept {
        return addr_;
    }

private:
    sockaddr_in addr_;
};


MUDUO_STUDY_END_NAMESPACE