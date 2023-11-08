#pragma once
#include "core.hpp"
#include <condition_variable>

MUDUO_STUDY_BEGIN_NAMESPACE

template<class EventLoop>
class EventLoopThreadImpl
{
public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    EventLoopThreadImpl(ThreadInitCallBack&& cb = ThreadInitCallBack()) :
        init_callback_{std::move(cb)},
        loop_{nullptr},
        thread_{},
        mutex_{},
        cv_{},
        exiting_{false}
        {}
    ~EventLoopThreadImpl() {
        exiting_ = true;
        if (loop_) {
            loop_->Quit();
        }
    }

    EventLoop* StartLoop() {
        assert(!thread_.joinable());
        thread_ = std::jthread([this](){ this->StartLoop(); });
        {
            std::unique_lock lock{mutex_};
            while (!loop_) {
                cv_.wait(lock);
            }
        }
        return loop_;
    }

private:
    void ThreadFunc() {
        EventLoop loop;
        if (init_callback_) init_callback_(&loop);
        {
            std::scoped_lock(mutex_);
            loop_ = &loop;
        }
        cv_.notify_one();
        loop.Loop();
        std::scoped_lock(mutex_);
        loop_ = nullptr;
    }

    EventLoop* loop_;
    bool exiting_;
    std::jthread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    ThreadInitCallBack init_callback_;
};

class EventLoop;
using EventLoopThread = EventLoopThreadImpl<EventLoop>;

MUDUO_STUDY_END_NAMESPACE