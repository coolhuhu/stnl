#ifndef STNL_BUFFER_H
#define STNL_BUFFER_H

#include <memory>
#include <string_view>
#include <string.h>

#include <vector>
#include <assert.h>
#include <string>
#include <sys/uio.h>

#include "noncopyable.h"

namespace stnl
{

    class FixedBuffer
    {
    public:
        FixedBuffer(int buffer_size) : capacity_(buffer_size), buffer_(new char[buffer_size])
        {
            curIndex_ = buffer_.get();
        }

        ~FixedBuffer() = default;

        char *begin() const
        {
            return buffer_.get();
        }

        char *end() const
        {
            return buffer_.get() + capacity_;
        }

        /**
         * @brief buffer的总容量大小
         *
         * @return std::size_t
         */
        std::size_t capacity() const
        {
            return capacity_;
        }

        /**
         * @brief buffer中已被占用的大小
         *
         * @return int
         */
        int size() const
        {
            return static_cast<int>(curIndex_ - buffer_.get());
        }

        int remainingCapacity() const
        {
            return static_cast<int>(end() - current());
        }

        char *current() const
        {
            return curIndex_;
        }

        void append(const char *data, std::size_t len)
        {
            if (remainingCapacity() > len)
            {
                memcpy(curIndex_, data, len);
                curIndex_ += len;
            }
            // FIXME: 若缓冲区可用大小小于写入的字节数量，如何处理？
        }

        void append(std::string_view str)
        {
            if (remainingCapacity() > str.size())
            {
                memcpy(curIndex_, str.data(), str.size());
                curIndex_ += str.size();
            }
            // FIXME: 若缓冲区可用大小小于写入的字节数量，如何处理？
        }

        void add(int len)
        {
            curIndex_ += len;
        }

        void clear()
        {
            memset(begin(), 0, capacity_);
            curIndex_ = buffer_.get();
        }

        void reset()
        {
            curIndex_ = buffer_.get();
        }

    private:
        std::unique_ptr<char[]> buffer_;
        const std::size_t capacity_;
        char *curIndex_;
    };



    /**
     * @brief 来源 muduo 的 Buffer
     *
     *  +-------------------+------------------+------------------+
     *  | prependable bytes |  readable bytes  |  writable bytes  |
     *  |                   |     (CONTENT)    |                  |
     *  +-------------------+------------------+------------------+
     *  |                   |                  |                  |
     *  0      <=       readIndex   <=    writeIndex    <=     size
     *
     * prependableBytes = readIndex
     * readableBytes = writeIndex - readIndex
     * writeableBytes = buffer.size() - writeIndex
     *
     */
    class NetBuffer : public noncopyable
    {
    public:
        static const size_t ReservedPrependSize = 8;
        static const size_t InitialBufferSize = 1024; // 1kB

        explicit NetBuffer(size_t initialBufferSize = InitialBufferSize) : buffer_(ReservedPrependSize + initialBufferSize),
                                                                           readIndex_(ReservedPrependSize),
                                                                           writeIndex_(ReservedPrependSize)
        {
            assert(readableBytes() == 0);
            assert(writeableBytes() == initialBufferSize);
            assert(prependableBytes() == ReservedPrependSize);
        }

        size_t capacity() const
        {
            return buffer_.capacity();
        }

        char *begin()
        {
            return &*buffer_.begin();
        }

        const char *begin() const
        {
            return &*buffer_.begin();
        }

        size_t readableBytes() const
        {
            return writeIndex_ - readIndex_;
        }

        size_t writeableBytes() const
        {
            return buffer_.size() - writeIndex_;
        }

        size_t prependableBytes() const
        {
            return readIndex_;
        }

        const char *peek() const
        {
            return begin() + readIndex_;
        }

        char *writeIndex()
        {
            return begin() + writeIndex_;
        }

        const char *writeIndex() const
        {
            return begin() + writeIndex_;
        }

        char *readIndex()
        {
            return begin() + readIndex_;
        }

        const char *readIndex() const
        {
            return begin() + readIndex_;
        }

        void append(const char *buf, size_t len);

        void append(const void *buf, size_t len)
        {
            append(static_cast<const char *>(buf), len);
        }

        void append(std::string_view buf)
        {
            append(buf.data(), buf.size());
        }

        /*
            在 muduo 中，prepred的大小为8字节，用来填充一条完整 "message" 的大小。
        */
        void prepend(const void *buf, size_t len)
        {
            assert(len <= prependableBytes());
            readIndex_ -= len;
            const char *data = static_cast<const char *>(buf);
            std::copy(data, data + len, begin() + readIndex_);
        }

        void prepend(int8_t val)
        {
            prepend(&val, sizeof(int8_t));
        }

        void prepend(int16_t val)
        {
            prepend(&val, sizeof(int16_t));
        }

        void prepend(int32_t val)
        {
            prepend(&val, sizeof(int32_t));
        }

        void prepend(int64_t val)
        {
            prepend(&val, sizeof(int64_t));
        }

        void retrieve(size_t len)
        {
            assert(len <= readableBytes());
            if (len < readableBytes()) {
                readIndex_ += len;
            }
            else {
                retrieveAll();
            }
        }

        void retrieveAll()
        {
            readIndex_ = ReservedPrependSize;
            writeIndex_ = ReservedPrependSize;
        }

        /**
         * 将 buffer中的数据读出，以字符串的形式返回。
         */
        std::string retrieveAsString(size_t len)
        {
            assert(len <= readableBytes());
            std::string res(peek(), len);
            if (len < readableBytes())
            {
                readIndex_ += len;
            }
            else
            {
                readIndex_ = ReservedPrependSize;
                writeIndex_ = ReservedPrependSize;
            }
            return res;
        }

        std::string retrieveAllAsString()
        {
            std::string res(peek(), readableBytes());
            readIndex_ = ReservedPrependSize;
            writeIndex_ = ReservedPrependSize;
            return res;
        }

        /**
         * 将文件描述符中的数据读入 buffer
         */
        ssize_t readFD(int fd, int *savedErrno);

    private:
        /**
         * 把已有的数据移动到buffer的前头，腾出 writeable 的空间。
         * 一种优化的方法是，使用 circular buffer，但是这样buffer中的数据存放就不是"连续的"。
         */
        void memoryMoving();

    private:
        std::vector<char> buffer_;
        size_t readIndex_;
        size_t writeIndex_;
    };

}

#endif