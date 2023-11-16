#pragma once
#include "core.hpp"
#include "callbacks.hpp"
#include "buffer.hpp"
#include "inet_address.hpp"

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

    }
    ~TcpConnection() {

    }

    auto loop() const noexcept { return loop_; }
    auto name() { return name_; }
    auto local_addr() { return local_addr_; }
    auto peer_addr() { return peer_addr_; }
    bool connected() { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }
    bool reading() const noexcept { return reading_; }
    std::optional<tcp_info*> tcp_info();
    std::string tcp_info_string();
    void set_tcp_no_dealy(bool on);
    auto input_buffer() { return &input_buffer_; }
    auto output_buffer() { return &output_buffer_; }
    
    void set_connection_callback(ConnectionCallback cb) { connection_callback_ = std::move(cb); }
    void set_message_callback(MessageCallback cb) { message_callback_ = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { write_complete_callback_ = std::move(cb); }
    void set_high_water_mark_callback(HighWaterMarkCallback cb) { high_water_mark_callback_ = std::move(cb); }
    void set_close_callback(CloseCallback cb) { close_callback_ = std::move(cb); }

    void Send(const std::span<char> msg);
    void Send(Buffer* msg);
    void Shutdown();
    void ForceClose();
    void StartRead();
    void StopRead();


private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

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