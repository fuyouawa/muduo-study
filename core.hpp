#pragma once
#define FMT_HEADER_ONLY 
#include <fmt/format.h>
#include <string.h>
#include <string_view>
#include <concepts>
#include <errno.h>
#include <assert.h>
#include <iostream>

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

MUDUO_STUDY_END_NAMESPACE