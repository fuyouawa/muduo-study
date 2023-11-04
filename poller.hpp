#include "core.hpp"
#include <vector>
#include <unordered_map>

MUDUO_STUDY_BEGIN_NAMESPACE

class EventLoop;
class Channel;

namespace details {
class Poller
{
public:
    using ChannelList = std::vector<std::reference_wrapper<Channel>>;

    static std::shared_ptr<Poller> NewDefaultPoller(std::reference_wrapper<EventLoop> loop);

    Poller(std::reference_wrapper<EventLoop> loop) :
        loop_{loop} {}
    virtual ~Poller() {}

    virtual time_point Poll(std::chrono::milliseconds timeout, ChannelList& active_channels) = 0;
    virtual void UpdateChannel(Channel& channel) = 0;
    virtual void RemoveChannel(Channel& channel) = 0;
    virtual void HasChannel(const Channel& channel) = 0;

protected:
    using ChannelMap = std::unordered_map<int, std::reference_wrapper<Channel>>;
    ChannelMap channels_;

private:
    std::reference_wrapper<EventLoop> loop_;
};
}

MUDUO_STUDY_END_NAMESPACE