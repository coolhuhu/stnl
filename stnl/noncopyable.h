#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

/**
 * @brief 继承该类，以实现禁止拷贝
 * 
 */
class noncopyable {
public:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};


#endif

