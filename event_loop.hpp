#pragma once
#include "core.hpp"
#include "poller.hpp"
#include "channel.hpp"
#include "logger.hpp"
#include <sys/eventfd.h>

MUDUO_STUDY_BEGIN_NAMESPACE

int CreateEventfd() {
    auto fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1) {
        MUDUO_STUDY_LOG_SYSERR("eventfd failed!");
    }
    return fd;
}

class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop() :
        poller_{details::Poller::NewDefaultPoller(this)},
        cur_active_channel_{nullptr},
        wakeup_channel_{new Channel(this, CreateEventfd())}
    {
        MUDUO_STUDY_LOG_DEBUG("EventLoop created");
        wakeup_channel_->set_read_callback([this](auto receive_time){
            uint64_t one = 1;
            auto n = read(wakeup_channel_->fd(), &one, sizeof(one));
            if (n != sizeof(one)) {
                MUDUO_STUDY_LOG_ERROR("handle read: {} bytes instead of 8!", n);
            }
        });
        wakeup_channel_->EnableReading();
    }
    ~EventLoop() {
        wakeup_channel_->DisableAll();
        wakeup_channel_->Remove();
        close(wakeup_channel_->fd());
    }

    void Loop();
    void Quit();
    time_point poll_return_time();
    void run_in_loop(Functor& cb);
    void queue_in_loop(Functor& cb);
    size_t queue_size() const;

    void UpdateChannel(Channel* channel) {
        assert(channel->owner_loop() == this);
        poller_->UpdateChannel(channel);
    }
    void RemoveChannel(Channel* channel) {
        assert(channel->owner_loop() == this);
        if (event_handling_) {
            assert(cur_active_channel_ == channel ||
                std::ranges::find_if(active_channels_, [=](auto ch){ return &ch.get() == channel;}) == active_channels_.end());
        }
        poller_->RemoveChannel(channel);
    }
    bool HasChannel(const Channel& channel) {
        assert(channel.owner_loop() == this);
        return poller_->HasChannel(channel);
    }

private:
    time_point poll_return_time_;
    std::unique_ptr<details::Poller> poller_;
    ChannelList active_channels_;
    Channel* cur_active_channel_;
    std::unique_ptr<Channel> wakeup_channel_;
    bool event_handling_;
};


MUDUO_STUDY_END_NAMESPACE