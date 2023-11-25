#pragma once
#include "core.hpp"
#include "default_poller.hpp"
#include "logger.hpp"
#include <sys/eventfd.h>
#include <atomic>
#include <mutex>

MUDUO_STUDY_BEGIN_NAMESPACE

int CreateEventfd() {
    auto fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1) {
        MUDUO_STUDY_LOG_SYSERR("eventfd failed!");
    }
    return fd;
}

class EventLoop
{
public:
    MUDUO_STUDY_NONCOPYABLE(EventLoop)
    using Functor = std::move_only_function<void()>;
    thread_local static inline EventLoop* Instance = nullptr;
    static constexpr auto kPoolTimeoutMs = 10000ms;

    EventLoop() :
        looping_{false},
        quit_{false},
        event_handling_{false},
        calling_pending_functors_{false},
        iteration_{0},
        thread_id_{std::this_thread::get_id()},
        poller_{Poller::NewDefaultPoller(this)},
        cur_active_channel_{nullptr},
        wakeup_channel_{new Channel(this, CreateEventfd())}
    {
        MUDUO_STUDY_LOG_DEBUG("EventLoop created");
        if (Instance) {
            MUDUO_STUDY_LOG_FATAL("Another EventLoop {:016x} has existed in this thread({})!", (intptr_t)this, thread_id_);
        }
        else {
            Instance = this;
        }
        wakeup_channel_->set_read_callback([this](auto){ this->HandleRead(); });
        wakeup_channel_->EnableReading();
    }
    ~EventLoop() {
        wakeup_channel_->DisableAll();
        wakeup_channel_->Remove();
        ::close(wakeup_channel_->fd());
    }

    auto poll_return_time() const noexcept {
        return poll_return_time_;
    }
    auto queue_size() const noexcept {
        std::scoped_lock lock{mutex_};
        return pending_functors_.size();
    }

    void Loop() {
        assert(!looping_);
        AssertInLoopThread();
        looping_ = true;
        quit_ = false;
        MUDUO_STUDY_LOG_DEBUG("EventLoop({:016x}) Starting!", (intptr_t)this);
        while (!quit_) {
            active_channels_.clear();
            poll_return_time_ = poller_->Poll(kPoolTimeoutMs, &active_channels_);
            ++iteration_;
            event_handling_ = true;
            for (auto channel : active_channels_) {
                cur_active_channel_ = channel;
                cur_active_channel_->HandleEvent(poll_return_time_);
            }
            cur_active_channel_ = nullptr;
            event_handling_ = false;
            DoPendingFunctors();
        }
        MUDUO_STUDY_LOG_DEBUG("EventLoop({:016x}) Stop!", (intptr_t)this);
        looping_ = false;
    }
    void Quit() {
        quit_ = true;
        if (!IsInLoopThread()) {
            Wakeup();
        }
    }
    void RunInLoop(Functor cb) {
        if (IsInLoopThread()) {
            cb();
        }
        else {
            QueueInLoop(std::move(cb));
        }
    }
    void QueueInLoop(Functor cb) {
        {
            std::scoped_lock lock{mutex_};
            pending_functors_.push_back(std::move(cb));
        }
        if (!IsInLoopThread() || calling_pending_functors_) {
            Wakeup();
        }
    }
    void Wakeup() {
        uint64_t one = 1;
        auto n = ::write(wakeup_channel_->fd(), &one, sizeof(one));
        if (n != sizeof(one)) {
            MUDUO_STUDY_LOG_ERROR("write() {} bytes instead of 8!", n);
        }
    }
    bool IsInLoopThread() const { return thread_id_ == std::this_thread::get_id(); }

    void UpdateChannel(Channel* channel) {
        assert(channel->owner_loop() == this);
        AssertInLoopThread();
        poller_->UpdateChannel(channel);
    }
    void RemoveChannel(Channel* channel) {
        assert(channel->owner_loop() == this);
        AssertInLoopThread();
        if (event_handling_.load()) {
            assert(cur_active_channel_ == channel ||
                std::ranges::find(active_channels_, channel) == active_channels_.end());
        }
        poller_->RemoveChannel(channel);
    }
    bool HasChannel(Channel* channel) {
        assert(channel->owner_loop() == this);
        AssertInLoopThread();
        return poller_->HasChannel(channel);
    }

    void AssertInLoopThread() {
        if (!IsInLoopThread()) {
            MUDUO_STUDY_LOG_FATAL("EventLoop({:016x}) was created in thread-id:{}, but current thread-id is {}",
                (intptr_t)this, thread_id_, std::this_thread::get_id());
        }
    }

private:
    void DoPendingFunctors() {
        std::vector<Functor> functors;
        calling_pending_functors_ = true;
        {
            std::scoped_lock lock{mutex_};
            functors.swap(functors);
        }
        for (decltype(auto) functor : functors) {
            functor();
        }
        calling_pending_functors_ = false;
    }

    void HandleRead() {
        uint64_t one = 1;
        auto n = ::read(wakeup_channel_->fd(), &one, sizeof(one));
        if (n != sizeof(one)) {
            MUDUO_STUDY_LOG_ERROR("read: {} bytes instead of 8!", n);
        }
    }

    std::atomic_bool looping_;
    std::atomic_bool quit_;
    std::atomic_bool event_handling_;
    std::atomic_bool calling_pending_functors_;

    int64_t iteration_;
    std::jthread::id thread_id_;
    time_point poll_return_time_;

    std::unique_ptr<Poller> poller_;
    ChannelList active_channels_;
    Channel* cur_active_channel_;
    std::unique_ptr<Channel> wakeup_channel_;

    mutable std::mutex mutex_;
    std::vector<Functor> pending_functors_;
};


MUDUO_STUDY_END_NAMESPACE