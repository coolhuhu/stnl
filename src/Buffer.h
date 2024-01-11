#ifndef STNL_BUFFER_H
#define STNL_BUFFER_H

#include <memory>
#include <string_view>
#include <string.h>


class FixedBuffer
{
public:
    FixedBuffer(int buffer_size): capacity_(buffer_size), buffer_(new char[buffer_size]) {
        curIndex_ = buffer_.get();
    }

    ~FixedBuffer() = default;

    char* begin() const {
        return buffer_.get();
    }

    char* end() const {
        return buffer_.get() + capacity_;
    }

    /**
     * @brief buffer的总容量大小
     * 
     * @return std::size_t 
     */
    std::size_t capacity() const {
        return capacity_;
    }

    /**
     * @brief buffer中已被占用的大小
     * 
     * @return int 
     */
    int size() const {
        return static_cast<int>(curIndex_ - buffer_.get());
    }

    int remainingCapacity() const {
        return static_cast<int>(end() - current());
    }

    char* current() const {
        return curIndex_;
    }

    void append(const char* data, std::size_t len) {
        if (remainingCapacity() > len) {
            memcpy(curIndex_, data, len);
            curIndex_ += len;
        }
        // FIXME: 若缓冲区可用大小小于写入的字节数量，如何处理？
    }

    void append(std::string_view str) {
        if (remainingCapacity() > str.size()) {
            memcpy(curIndex_, str.data(), str.size());
            curIndex_ += str.size();
        }
        // FIXME: 若缓冲区可用大小小于写入的字节数量，如何处理？
    }

    void add(int len) {
        curIndex_ += len;
    }

    void clear() {
        memset(begin(), 0, capacity_);
        curIndex_ = buffer_.get();
    }

    void reset() {
        curIndex_ = buffer_.get();
    }
    
private:
    std::unique_ptr<char[]> buffer_;
    const std::size_t capacity_;
    char *curIndex_;
};


#endif