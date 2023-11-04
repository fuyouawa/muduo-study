#include "core.hpp"
#include <sys/epoll.h>
#include <chrono>
#include <functional>

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

    Channel(EventLoop* loop, int fd) : 
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

    bool IsNoneEvent() const {return events_ == kNoneEvent; }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    void Update() {
        added_to_loop_ = true;
    }
    void Remove() {
        added_to_loop_ = false;
    }

private:
    EventLoop* loop_;
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