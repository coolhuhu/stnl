#include "Buffer.h"

using namespace stnl;



const size_t NetBuffer::ReservedPrependSize;
const size_t NetBuffer::InitialBufferSize;

ssize_t NetBuffer::readFD(int fd, int *savedErrno)
{
    char extrabuf[65536]; // 64KB
    struct iovec vec[2];
    const size_t writeable = writeableBytes();
    vec[0].iov_base = writeIndex();
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    /*
        详情见陈硕的《多线程服务器编程》p315 脚注部分。
        “64KB的buffer足够容纳千兆网在500us内全速收到的数据”
    */
    const int vec_size = writeable < sizeof(extrabuf) ? 2 : 1;
    ssize_t n = readv(fd, vec, vec_size);

    // NOTE: 使用C++中的异常处理
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) < writeable)
    {
        // buffer_中的可写空间足够
        writeIndex_ += n;
    }
    else
    {
        // buffer_中空间不够写，需要额外的空间
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writeable);
    }
    /**
     * FIXME: if (n == writeable + sizeof(extrabuf))
     * 说明文件描述符上缓冲区中的数据调用一次readv不能全部读出
    */

    return n;
}

void NetBuffer::memoryMoving()
{
    assert(ReservedPrependSize < readIndex_);
    size_t readable = readableBytes();
    std::copy(begin() + readIndex_, begin() + writeIndex_, begin() + ReservedPrependSize);
    readIndex_ = ReservedPrependSize;
    writeIndex_ = readIndex_ + readable;
    assert(readable == readableBytes());
}

void NetBuffer::append(const char *buf, size_t len)
{
    if (len < writeableBytes())
    {
        std::copy(buf, buf + len, writeIndex());
    }
    else
    {
        if (len <= writeableBytes() + prependableBytes() - ReservedPrependSize)
        {
            /*
                buffer 末端的连续可写空间不足，但总的可写空间大于 len。
                将buffer中的已写空间向前挪动
            */
            memoryMoving();
        }
        else
        {
            // @todo: 需要进行性能优化
            /*
                buffer总的可写空间不足小于 len。
                先分配更大的内存，然后再挪动位置。
            */
            buffer_.resize(writeIndex_ + len);
            memoryMoving();
        }
        std::copy(buf, buf + len, writeIndex());
    }
    writeIndex_ += len;
}