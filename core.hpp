#pragma once
#include <format>
#include <string.h>
#include <string_view>
#include <concepts>
#include <errno.h>
#include <assert.h>
#include <iostream>
#include <chrono>
#include <sys/epoll.h>
#include <ranges>
#include <thread>
#include <sstream>
#include <optional>
#include <expected>

#ifndef MUDUO_STUDY_BEGIN_NAMESPACE
#define MUDUO_STUDY_BEGIN_NAMESPACE namespace muduo_study {
#endif

#ifndef MUDUO_STUDY_END_NAMESPACE
#define MUDUO_STUDY_END_NAMESPACE }
#endif

#ifndef MUDUO_STUDY
#define MUDUO_STUDY muduo_study::
#endif

#ifndef MUDUO_STUDY_NONCOPYABLE
#define MUDUO_STUDY_NONCOPYABLE(x) \
    x(const x&) = delete; \
    x& operator=(const x&) = delete;
#endif

MUDUO_STUDY_BEGIN_NAMESPACE

using namespace std::chrono_literals;

template<typename T>
void ZeroMemory(T& s) {
    if constexpr (std::is_array_v<T>) {
        memset(s, 0, std::extent_v<T> * sizeof(std::remove_extent_t<T>));
    }
    else if constexpr (std::is_pointer_v<T>) {
        memset(s, 0, sizeof(std::remove_pointer_t<T>));
    }
    else {
        memset(std::addressof(s), 0, sizeof(std::decay_t<T>));
    }
}

template<typename T>
void ZeroMemory(T* s, size_t count) {
    memset(s, 0, sizeof(T) * count);
}

using time_point = std::chrono::system_clock::time_point;

MUDUO_STUDY_END_NAMESPACE

template <>
struct std::formatter<std::jthread::id> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) const {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::jthread::id& tid, FormatContext& ctx) const {
        std::stringstream ss;
        ss << tid;
        return format_to(ctx.out(), "{}", ss.str());
    }
};