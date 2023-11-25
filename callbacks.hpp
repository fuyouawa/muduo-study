#pragma once
#include "core.hpp"
#include "buffer.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::move_only_function<void()>;
using CloseCallback = std::move_only_function<void (const TcpConnectionPtr)>;
using HighWaterMarkCallback = std::move_only_function<void (const TcpConnectionPtr, size_t)>;

using ConnectionCallback = std::function<void (const TcpConnectionPtr)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr)>;
using MessageCallback = std::function<void (const TcpConnectionPtr,
                                            Buffer* buffer,
                                            time_point reveive_time)>;

MUDUO_STUDY_END_NAMESPACE