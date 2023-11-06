#pragma once
#include "core.hpp"
#include <chrono>
#include <functional>

MUDUO_STUDY_BEGIN_NAMESPACE

template<typename EventLoop/*=EventLoop*/>
class ChannelImpl
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(std::chrono::system_clock::time_point)>;

    enum Event {
        kNoneEvent = 0,
        kReadEvent = EPOLLIN | EPOLLPRI,
        kWriteEvent = EPOLLOUT
    };
    enum Status {
        kNew,
        kDeleted,
        kAdded
    };

    ChannelImpl(EventLoop* loop, int fd) : 
        loop_{loop},
        fd_{fd},
        events_{kNoneEvent},
        revents_{kNoneEvent},
        status_{kNew},
        event_handling_{false},
        added_to_loop_{false} {}

    ~ChannelImpl() {

    }

    void Tie(const std::shared_ptr<void>& obj) {
        tie_ = obj;
    }

    void set_read_callback(ReadEventCallback&& cb) noexcept { read_callback_ = std::move(cb); }
    void set_write_callback(EventCallback&& cb) noexcept { write_callback_ = std::move(cb); }
    void set_close_callback(EventCallback&& cb) noexcept { close_callback_ = std::move(cb); }
    void set_error_callback(EventCallback&& cb) noexcept { error_callback_ = std::move(cb); }

    int fd() const noexcept { return fd_; }
    int events() const noexcept { return events_; }
    int revents() const noexcept { return revents_; }
    void set_revents(int revents) noexcept { revents_ = revents; }
    Status status() const noexcept { return status_; }
    void set_status(Status status) noexcept { status_ = status; }
    EventLoop* owner_loop() const noexcept { return loop_; }

    bool IsNoneEvent() const {return events_ == kNoneEvent; }
    bool IsWriting() const {return events_ & kWriteEvent; }
    bool IsReading() const {return events_ & kReadEvent; }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    void Remove() {
        added_to_loop_ = false;
        loop_->UpdateChannel(this);
    }

    void HandleEvent(time_point receive_time) {
        if (tie_.expired()) {
            if (tie_.lock()) {
                HandleEventWithGuard(receive_time);
            }
        }
        else {
            HandleEventWithGuard(receive_time);
        }
        
    }

private:
    void Update() {
        added_to_loop_ = true;
        loop_->RemoveChannel(this);
    }

    void HandleEventWithGuard(time_point receive_time) {
        event_handling_ = true;
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
            if (close_callback_) close_callback_();
        }
        if (revents_ & EPOLLERR) {
            if (error_callback_) close_callback_();
        }
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
            if (read_callback_) read_callback_(receive_time);
        }
        if (revents_ & EPOLLOUT) {
            if (write_callback_) write_callback_();
        }
        event_handling_ = false;
    }

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    Status status_;

    std::weak_ptr<void> tie_;
    bool event_handling_;
    bool added_to_loop_;
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

class EventLoop;
using Channel = ChannelImpl<EventLoop>;
MUDUO_STUDY_END_NAMESPACE