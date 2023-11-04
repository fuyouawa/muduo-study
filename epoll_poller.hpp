#pragma once
#include "logger.hpp"
#include "poller.hpp"
#include "channel.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class EPollPoller : public details::Poller
{
public:
    EPollPoller(std::reference_wrapper<EventLoop> loop) :
        details::Poller{loop},
        epollfd_{epoll_create1(EPOLL_CLOEXEC)},
        events_{kInitEventListSize} {}

    ~EPollPoller() override {
        close(epollfd_);
    }

    time_point Poll(std::chrono::milliseconds timeout, ChannelList& active_channels) override {

    }
    void UpdateChannel(Channel& channel) override {

    }
    void RemoveChannel(Channel& channel) override {

    }
    void HasChannel(const Channel& channel) override {

    }

private:
    static constexpr int kInitEventListSize = 16;

    static std::string_view OpToStr(int op) {
        switch (op)
        {
        case EPOLL_CTL_ADD:
            return "EPOLL_CTL_ADD";
        case EPOLL_CTL_DEL:
            return "EPOLL_CTL_DEL";
        case EPOLL_CTL_MOD:
            return "EPOLL_CTL_MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
        }
    }

    void FillActiveChannels(int num_events, ChannelList& active_channels) const {
        assert(num_events < events_.size());
        for (size_t i = 0; i < num_events; i++){
            decltype(auto) channel = *static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
            auto it = channels_.find(channel.fd());
            assert(it != channels_.end() && &it->second.get() == &channel);
#endif
            channel.set_revents(events_[i].events);
            active_channels.push_back(channel);
        }
    }

    void Update(int op, Channel& channel) {
        epoll_event event;
        ZeroMemory(event);
        event.events = channel.events();
        event.data.ptr = &channel;
        auto fd = channel.fd();
        if (epoll_ctl(epollfd_, op, fd, &event) == -1) {
            auto e = errno;
            if (op == EPOLL_CTL_DEL) {
                MUDUO_STUDY_LOG_ERROR("epoll_ctl failed, op is {}, info:{}-{}", OpToStr(op), strerror(e), e);
            }
            else {
                MUDUO_STUDY_LOG_FATAL("epoll_ctl failed, op is {}, info:{}-{}", OpToStr(op), strerror(e), e);
            }
        }
        
    }

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};


MUDUO_STUDY_END_NAMESPACE