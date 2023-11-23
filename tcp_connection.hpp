#pragma once
#include "core.hpp"
#include "callbacks.hpp"
#include "buffer.hpp"
#include "inet_address.hpp"
#include "socket.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(
        EventLoop* loop,
        std::string_view name,
        int sockfd,
        const InetAddress& local_addr,
        const InetAddress& peer_addr)
    {
        channel_->set_read_callback([this](auto rt){ HandleRead(rt); });
        channel_->set_write_callback([this](){ HandleWrite(); });
        channel_->set_close_callback([this](){ HandleClose(); });
        channel_->set_error_callback([this](){ HandleError(); });
        MUDUO_STUDY_LOG_DEBUG("TcpConnection::ctor[{}] at fd={}", name, sockfd);
        socket_->set_keep_alive(true);
    }
    ~TcpConnection() {
        MUDUO_STUDY_LOG_DEBUG("TcpConnection::dtor[{}] at fd={}", name_, channel_->fd());
        assert(state_ == kDisconnected);
    }

    auto loop() const noexcept { return loop_; }
    auto name() { return name_; }
    auto local_addr() { return local_addr_; }
    auto peer_addr() { return peer_addr_; }
    bool connected() { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }
    bool reading() const noexcept { return reading_; }
    auto tcp_info() const noexcept { return socket_->tcp_info(); }
    auto input_buffer() { return &input_buffer_; }
    auto output_buffer() { return &output_buffer_; }
    
    void set_connection_callback(ConnectionCallback cb) { connection_callback_ = std::move(cb); }
    void set_message_callback(MessageCallback cb) { message_callback_ = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { write_complete_callback_ = std::move(cb); }
    void set_high_water_mark_callback(HighWaterMarkCallback cb) { high_water_mark_callback_ = std::move(cb); }
    void set_close_callback(CloseCallback cb) { close_callback_ = std::move(cb); }
    void set_tcp_no_dealy(bool b) { socket_->set_tcp_no_delay(b); }

    void Send(const std::span<char> msg);
    void Send(Buffer* msg);
    void Shutdown();
    void ForceClose();
    void StartRead();
    void StopRead();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    void set_state(StateE s) noexcept { state_ = s; }

    void HandleRead(time_point receive_time) {
        loop_->AssertInLoopThread();
        auto exp = input_buffer_.ReadFd(channel_->fd());
        if (exp.has_value()) {
            if (exp.value() > 0) {
                message_callback_(shared_from_this(), &input_buffer_, receive_time);
            }
            else {
                HandleClose();
            }
        }
        else {
            errno = exp.error();
            MUDUO_STUDY_LOG_SYSERR("muduo_study::Buffer::ReadFd failed!");
            HandleError();
        }
    }
    void HandleWrite() {
        loop_->AssertInLoopThread();
        if (channel_->IsWriting()) {
            auto n = output_buffer_.WriteFd(channel_->fd());
            if (n > 0) {
                output_buffer_.Retrieve(n);
                if (output_buffer_.readable_bytes() == 0) {
                    channel_->DisableWriting();
                    if (write_complete_callback_) {
                        loop_->QueueInLoop([this](){ write_complete_callback_(shared_from_this()); });
                    }
                    if (state_ == kDisconnecting) {
                        ShutdownInLoop();
                    }
                }
            }
            else {
                MUDUO_STUDY_LOG_SYSERR("muduo_study::Buffer::WriteFd failed!");
            }
        }
        else {
            MUDUO_STUDY_LOG_DEBUG("Connection fd={} is down, no more writing", channel_->fd());
        }
    }
    void HandleClose() {
        loop_->AssertInLoopThread();
        assert(state_ == kConnected || state_ == kDisconnecting);
        set_state(kDisconnected);
        channel_->DisableAll();
        connection_callback_(shared_from_this());
        close_callback_(shared_from_this());
    }
    void HandleError() {
        MUDUO_STUDY_LOG_ERROR2(socket_->socket_error(), "SO_ERROR");
    }
    void SendInLoop(std::span<char> msg);
    void ShutdownInLoop();
    void ForceCloseInLoop();
    void StartReadInLoop();
    void StopReadInLoop();

    EventLoop* loop_;
    const std::string name_;
    StateE state_;
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress local_addr_;
    const InetAddress peer_addr_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_water_mark_callback_;
    CloseCallback close_callback_;
    size_t high_water_mark_;
    Buffer input_buffer_;
    Buffer output_buffer_;
};

MUDUO_STUDY_END_NAMESPACE