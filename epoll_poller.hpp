#pragma once
#include "logger.hpp"
#include "poller.hpp"
#include "channel.hpp"


MUDUO_STUDY_BEGIN_NAMESPACE

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop) :
        Poller{loop},
        epollfd_{epoll_create1(EPOLL_CLOEXEC)},
        events_{kInitEventListSize}
    {
        if (epollfd_ < 0) {
            MUDUO_STUDY_LOG_FATAL("epoll_create1 failed!");
        }
    }

    ~EPollPoller() override {
        ::close(epollfd_);
    }

    time_point Poll(std::chrono::milliseconds timeout, ChannelList* active_channels) override {
        size_t num_events = ::epoll_wait(epollfd_, events_.data(), events_.size(), timeout.count());
        auto e = errno;
        if (num_events > 0) {
            MUDUO_STUDY_LOG_DEBUG("{} events happened!", num_events);
            FillActiveChannels(num_events, active_channels);
            if (num_events == events_.size()) {
                events_.resize(events_.size() * 2);
            }
        }
        else if (num_events == 0) {
            MUDUO_STUDY_LOG_DEBUG("epoll_wait nothing hanppend");
        }
        else {
            if (e != EINTR) {
                errno = e;
                MUDUO_STUDY_LOG_SYSERR("epoll_wait faild!");
            }
        }
        return std::chrono::system_clock::now();
    }

    void UpdateChannel(Channel* channel) override {
        auto st = channel->status();
        auto fd = channel->fd();
        if (st == Channel::kNew || st == Channel::kDeleted) {
            if (st == Channel::kNew) {
                assert(channels_.find(fd) == channels_.end());
                channels_[fd] = channel;
            }
            else {
                assert(channels_.find(fd) != channels_.end());
                assert(channels_[fd] == channel);
            }
            channel->set_status(Channel::kAdded);
            Update(EPOLL_CTL_ADD, channel);
        }
        else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
            assert(st == Channel::kAdded);
            if (channel->IsNoneEvent()) {
                Update(EPOLL_CTL_DEL, channel);
                channel->set_status(Channel::kDeleted);
            }
            else {
                Update(EPOLL_CTL_MOD, channel);
            }
        }
    }
    void RemoveChannel(Channel* channel) override {
        auto fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->IsNoneEvent());
        auto st = channel->status();
        assert(st != Channel::kNew);
        auto n = channels_.erase(fd);
        assert(n == 1);
        if (st == Channel::kAdded) {
            Update(EPOLL_CTL_DEL, channel);
        }
        channel->set_status(Channel::kNew);
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

    void FillActiveChannels(size_t num_events, ChannelList* active_channels) const {
        assert(num_events < events_.size());
        for (size_t i = 0; i < num_events; i++){
            auto channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
            auto it = channels_.find(channel->fd());
            assert(it != channels_.end() && it->second == channel);
#endif
            channel->set_revents(events_[i].events);
            active_channels->push_back(channel);
        }
    }

    void Update(int op, Channel* channel) {
        epoll_event event;
        ZeroMemory(event);
        event.events = channel->events();
        event.data.ptr = channel;
        auto fd = channel->fd();
        MUDUO_STUDY_LOG_DEBUG("epoll_ctl({}, {}, {}, {})", epollfd_, OpToStr(op), fd, channel->events_str());
        if (::epoll_ctl(epollfd_, op, fd, &event) == -1) {
            if (op == EPOLL_CTL_DEL) {
                MUDUO_STUDY_LOG_SYSERR("epoll_ctl failed! op is {}", OpToStr(op));
            }
            else {
                MUDUO_STUDY_LOG_SYSFATAL("epoll_ctl failed! op is {}", OpToStr(op));
            }
        }
        
    }

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};


MUDUO_STUDY_END_NAMESPACE