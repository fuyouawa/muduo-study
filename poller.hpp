#pragma once
#include "core.hpp"
#include <vector>
#include <unordered_map>

MUDUO_STUDY_BEGIN_NAMESPACE

class EventLoop;
class Channel;
using ChannelList = std::vector<std::reference_wrapper<Channel>>;
using ChannelMap = std::unordered_map<int, std::reference_wrapper<Channel>>;

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
    virtual bool HasChannel(const Channel& channel) {
        auto it = channels_.find(channel.fd());
        return it != channels_.end() && &it->second.get() == &channel;
    }

protected:
    ChannelMap channels_;

private:
    EventLoop* loop_;
};

MUDUO_STUDY_END_NAMESPACE