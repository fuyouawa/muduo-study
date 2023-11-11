#pragma once
#include "core.hpp"
#include "tcp_connection.hpp"
#include "buffer.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void (const TcpConnectionPtr&,
                                            Buffer* buffer,
                                            time_point reveive_time)>;

MUDUO_STUDY_END_NAMESPACE