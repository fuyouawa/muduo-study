#pragma once
#include "core.hpp"

MUDUO_STUDY_BEGIN_NAMESPACE

class Buffer
{
public:
    static constexpr size_t kCheapPrepend = 8;
    static constexpr size_t kInitialSize = 1024;

    explicit Buffer(size_t initial_size = kInitialSize) :
        buffer_(kCheapPrepend + kInitialSize),
        reader_index_{kCheapPrepend},
        writer_index_{kCheapPrepend}
    {}

    auto readable_bytes() const noexcept { return writer_index_ - reader_index_; }
    auto writable_bytes() const noexcept { return buffer_.size() - writer_index_; }
    auto prependable_bytes() const noexcept { return reader_index_; }
    auto peek() const noexcept { return &buffer_[reader_index_]; }

    void Retrieve(size_t len) {
        assert(len <= readable_bytes());
        if (len < readable_bytes()) {
            reader_index_ += len;
        }
        else {
            RetrieveAll();
        }
    }
    void RetrieveAll() {
        reader_index_ = writer_index_ = kCheapPrepend;
    }

    std::string RetrieveAllAsString() {
        return RetrieveAsString(readable_bytes());
    }

    void EnsureWritableBytes(size_t len) {
        if (writable_bytes() < len) {
            MakeSpace(len);
        }
        assert(writable_bytes() >= len);
    }

    std::string RetrieveAsString(size_t len) {
        assert(len <= readable_bytes());
        std::string res{peek(), len};
        Retrieve(len);
        return res;
    }

private:
    void MakeSpace(size_t len) {
        if (writable_bytes() + prependable_bytes() < len + kCheapPrepend) {
            buffer_.resize(writer_index_ + len);
        }
        else {
            assert(kCheapPrepend < reader_index_);
            auto readable = readable_bytes();
            std::copy(buffer_.begin() + reader_index_,
                    buffer_.begin() + writer_index_,
                    buffer_.begin() + kCheapPrepend);
            reader_index_ = kCheapPrepend;
            writer_index_ = reader_index_ + readable;
            assert(readable == readable_bytes());
        }
    }

    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
};

MUDUO_STUDY_END_NAMESPACE