#include <iostream>
#include <sys/timerfd.h>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>


/**
 * 设置一个定时器，死等定时器超时（通过read进行阻塞读）
*/
void set_timer()
{
    struct timespec curr_time;
    bzero(&curr_time, sizeof(curr_time));
    // timerfd 为阻塞的
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    
    // 获取调用 timerfd_create 时的时间
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    std::cout << "timerfd_create time: " << "sec-" << curr_time.tv_sec 
			  << " nano-" << curr_time.tv_nsec << std::endl;

    sleep(3);

    struct itimerspec new_val;
    bzero(&new_val, sizeof(new_val));
    new_val.it_value.tv_sec = 5;    // 5s后定时器超时
    timerfd_settime(timerfd, 0, &new_val, nullptr);

    // 获取调用 timerfd_settime 时的时间
    bzero(&curr_time, sizeof(curr_time));
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    std::cout << "timerfd_settime time: " << "sec-" << curr_time.tv_sec 
			  << " nano-" << curr_time.tv_nsec << std::endl;

    uint64_t howmany;
    // 阻塞读
    ssize_t n = read(timerfd, &howmany, sizeof(howmany));

    // 获取定时器超时时刻
    bzero(&curr_time, sizeof(curr_time));
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    std::cout << "timerfd timeout time: " << "sec-" << curr_time.tv_sec 
			  << " nano-" << curr_time.tv_nsec << std::endl;

    close(timerfd);
}


/**
 * 设置一个周期定时器
*/
void set_period_timer()
{
    struct itimerspec expire_time;
    bzero(&expire_time, sizeof(expire_time));
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    
    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event events[5];
    bzero(events, sizeof(events));
    
    struct epoll_event timer_event;
    bzero(&timer_event, sizeof(timer_event));
    timer_event.events = EPOLLIN;
    timer_event.data.fd = timerfd;

    expire_time.it_value.tv_sec = 5;
    // 5s 的定时周期
    expire_time.it_interval.tv_sec = 5;
    if (timerfd_settime(timerfd, 0, &expire_time, nullptr) < 0) {
        // error handle
    }
   
    epoll_ctl(epollfd, EPOLL_CTL_ADD, timerfd, &timer_event);

    struct timespec curr_time;
    

    while (true) {
        int num = epoll_wait(epollfd, events, 5, -1);

        bzero(&curr_time, sizeof(curr_time));
        clock_gettime(CLOCK_MONOTONIC, &curr_time);
        std::cout << "curr time: sec-" << curr_time.tv_sec << ", nano-" << curr_time.tv_nsec << std::endl;

        for (int i = 0; i < num; ++i) {
            int fd = events[i].data.fd;
            if (fd == timerfd) {
                std::cout << "timer timeout once." << std::endl;

                uint64_t howmany;
                ssize_t n = read(timerfd, &howmany, sizeof(howmany));
                std::cout << "howmany: " << howmany << std::endl;
            }
        }
    }
}


/**
 * 使用 timerfd_gettime 查看还有定时器还有多长时间超时
*/
void test_timerfd_gettime()
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    struct itimerspec new_val, old_val;
    bzero(&new_val, sizeof(new_val));
    bzero(&old_val, sizeof(old_val));

    clock_gettime(CLOCK_MONOTONIC, &old_val.it_value);
    // 调用 timerfd_settiem 时的时间
    std::cout << "sec: " << old_val.it_value.tv_sec << ", nano: " << old_val.it_value.tv_nsec << std::endl;
    bzero(&old_val, sizeof(old_val));

    new_val.it_value.tv_sec = 10;
    timerfd_settime(timerfd, 0, &new_val, &old_val);
    // 因为 timerfd_settime 在此处是第一次调用，因此 old_val 的值为0
    std::cout << "sec: " << old_val.it_value.tv_sec << ", nano: " << old_val.it_value.tv_nsec << std::endl;

    sleep(3);

    bzero(&old_val, sizeof(old_val));
    clock_gettime(CLOCK_MONOTONIC, &old_val.it_value);
    // sleep 3s 后，获取当前时刻的时间
    std::cout << "sec: " << old_val.it_value.tv_sec << ", nano: " << old_val.it_value.tv_nsec << std::endl;


    bzero(&old_val, sizeof(old_val));
    timerfd_settime(timerfd, 0, &new_val, &old_val);
    // 第二次调用 timerfd_settime,
    // 因为上一次调用 timerfd_settime 设置的定时器还未超时，因此上次的定时器失效
    // 距离上次设置的定时器超时还剩的时间间隔会被设置到 old_val 中
    std::cout << "sec: " << old_val.it_value.tv_sec << ", nano: " << old_val.it_value.tv_nsec << std::endl;

    uint64_t howmany;
    // 阻塞读
    ssize_t n = read(timerfd, &howmany, sizeof(howmany));

    // 定时器超时时刻
    bzero(&old_val, sizeof(old_val));
    clock_gettime(CLOCK_MONOTONIC, &old_val.it_value);
    std::cout << "sec: " << old_val.it_value.tv_sec << ", nano: " << old_val.it_value.tv_nsec << std::endl;
}


int main()
{
	
}