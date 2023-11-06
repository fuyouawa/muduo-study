#pragma once
#include "core.hpp"
#include "event_loop.hpp"
#include "inet_address.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class TcpServer
{
public:
    MUDUO_STUDY_NONCOPYABLE(TcpServer)

    TcpServer();
    ~TcpServer();
};

MUDUO_STUDY_END_NAMESPACE