#pragma once
#include "core.hpp"
#include "event_loop_thread.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class EventLoopThreadPool
{
public:
    MUDUO_STUDY_NONCOPYABLE(EventLoopThreadPool)

    EventLoopThreadPool(EventLoop* basic_loop) :
        basic_loop_{basic_loop},
        started_(false),
        num_threads_{0},
        next_{0}
        {}
    ~EventLoopThreadPool() = default;

    void set_thread_num(size_t num) {
        num_threads_ = num;
    }
    auto next_loop() {
        basic_loop_->AssertInLoopThread();
        assert(started_);
        EventLoop* loop = basic_loop_;
        if (!loops_.empty()) {
            loop = loops_[next_];
            ++next_;
            if (next_ >= loops_.size()) {
                next_ = 0;
            }
        }
        return loop;
    }
    auto all_loops() { return loops_; }
    auto started() { return started_; }

    void Start(ThreadInitCallBack&& cb) {
        assert(!started_);
        basic_loop_->AssertInLoopThread();
        started_ = true;
        for (size_t i = 0; i < num_threads_; i++) {
            threads_.push_back(std::move(cb));
        }
        if (num_threads_ == 0 && cb) {
            cb(basic_loop_);
        }
    }

private:
    EventLoop* basic_loop_;
    size_t num_threads_;
    size_t next_;
    std::vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;
    bool started_;
};


MUDUO_STUDY_END_NAMESPACE