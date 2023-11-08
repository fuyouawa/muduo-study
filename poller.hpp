#pragma once
#include "core.hpp"
#include "channel.hpp"
#include <vector>
#include <unordered_map>

MUDUO_STUDY_BEGIN_NAMESPACE

using ChannelList = std::vector<Channel*>;
using ChannelMap = std::unordered_map<int, Channel*>;

class Poller
{
public:
    static Poller* NewDefaultPoller(EventLoop* loop);

    Poller(EventLoop* loop) :
        loop_{loop} {}
    virtual ~Poller() {}

    virtual time_point Poll(std::chrono::milliseconds timeout, ChannelList* active_channels) = 0;
    virtual void UpdateChannel(Channel* channel) = 0;
    virtual void RemoveChannel(Channel* channel) = 0;
    virtual bool HasChannel(Channel* channel) {
        auto it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }

protected:
    ChannelMap channels_;

private:
    EventLoop* loop_;
};

MUDUO_STUDY_END_NAMESPACE