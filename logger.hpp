#pragma once
#include "core.hpp"
#include <vector>

MUDUO_STUDY_BEGIN_NAMESPACE

class Logger
{
public:
    enum Level {
        kInfo,
        kDebug,
        kWarning,
        kError,
        kFatal
    };

    static void set_ostream(Level lv, std::ostream& out) {
        out_manager_[lv].second = std::ref(out);
    }

    Logger(std::string_view filename, std::string_view func_name, size_t line, Level lv, int saved_errno=0) :
        filename_{filename},
        func_name_{func_name},
        line_{line},
        lv_{lv},
        saved_errno_{saved_errno} {}

    template<typename... Args>
    void Output(std::string_view fmt, Args&&... args) const {
#ifdef NDEBUG
        if (lv_ == kDebug) return;
#endif
        out_manager_[lv_].second.get() << message(std::vformat(fmt, std::make_format_args(args...)));
        switch (lv_)
        {
        case kError:
            throw std::runtime_error("Error occur!");
        case kFatal:
            abort();
        default:
            break;
        }
    }

    std::string message(std::string_view msg) const {
        if (lv_ == kInfo) {
            return std::format("[{}] {}\n", out_manager_[lv_].first, msg);
        }
        if (saved_errno_ != 0) {
            return std::format("[{}] {} strerr:{}-{} '{}() at {}:{}'\n",
                out_manager_[lv_].first, msg,
                strerror(saved_errno_), saved_errno_,
                func_name_, filename_, line_);
        }
        return std::format("[{}] {} '{}() at {}:{}'\n", out_manager_[lv_].first, msg, func_name_, filename_, line_);
    }

private:
    static inline std::vector<std::pair<std::string_view, std::reference_wrapper<std::ostream>>> out_manager_ = {
        {"Info", std::ref(std::cout)},
        {"Debug", std::ref(std::clog)},
        {"Warning", std::ref(std::cout)},
        {"Error", std::ref(std::cerr)},
        {"Fatal", std::ref(std::cerr)}
    };

    std::string_view filename_;
    std::string_view func_name_;
    size_t line_;
    Level lv_;
    int saved_errno_;
};

MUDUO_STUDY_END_NAMESPACE

#ifndef _MUDUO_STUDY_LOG
#define _MUDUO_STUDY_LOG(lv, saved_errno, ...) muduo_study::Logger{__FILE__, __func__, __LINE__, lv, saved_errno}.Output(__VA_ARGS__)
#define MUDUO_STUDY_LOG_INFO(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kInfo, 0, __VA_ARGS__)
#define MUDUO_STUDY_LOG_DEBUG(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kDebug, 0, __VA_ARGS__)
#define MUDUO_STUDY_LOG_WARNING(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kWarning, 0, __VA_ARGS__)
#define MUDUO_STUDY_LOG_ERROR(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kError, 0, __VA_ARGS__)
#define MUDUO_STUDY_LOG_FATAL(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kFatal, 0, __VA_ARGS__)
#define MUDUO_STUDY_LOG_SYSERR(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kError, errno, __VA_ARGS__)
#define MUDUO_STUDY_LOG_SYSFATAL(...) _MUDUO_STUDY_LOG(muduo_study::Logger::kFatal, errno, __VA_ARGS__)
#endif