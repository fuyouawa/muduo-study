#include "epoll_poller.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

Poller* Poller::NewDefaultPoller(EventLoop * loop) {
    return new EPollPoller(loop);
}

MUDUO_STUDY_END_NAMESPACE