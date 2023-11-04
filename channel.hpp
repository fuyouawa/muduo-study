#include "core.hpp"
#include <chrono>
#include <functional>
#include <sys/epoll.h>

MUDUO_STUDY_BEGIN_NAMESPACE

class EventLoop;

class Channel
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(std::chrono::system_clock::time_point)>;

    enum Event {
        kNoneEvent = 0,
        kReadEvent = EPOLLIN | EPOLLPRI,
        kWriteEvent = EPOLLOUT
    };

    Channel(std::reference_wrapper<EventLoop> loop, int fd) : 
        loop_{loop},
        fd_{fd},
        events_{kNoneEvent},
        revents_{kNoneEvent},
        index_{-1},
        event_handling_{false},
        added_to_loop_{false} {}

    ~Channel() {

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
    int index() const noexcept { return index_; }
    void set_index(int idx) noexcept { index_ = idx; }
    auto owner_loop() const noexcept { return loop_; }

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

    std::reference_wrapper<EventLoop> loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    std::weak_ptr<void> tie_;
    bool event_handling_;
    bool added_to_loop_;
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

MUDUO_STUDY_END_NAMESPACE