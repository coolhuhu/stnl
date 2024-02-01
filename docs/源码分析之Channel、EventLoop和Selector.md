在 [one-loop-per-thread核心原理](one-loop-per-thread核心原理.md) 中，介绍了 one loop per thread 网络模型的核心原理，在本篇本章中，我将重点介绍该模型中的IO事件处理部分在 muduo 网络库中是如何实现的，而涉及 TCP 连接处理部分，也即 socket 编程部分，将在另外的博客中进行讨论。

先以单线程下的 one loop per thread 为例，看看 muduo 中是如何设计实现的，然后再转向多线程下的 one loop per thread。

![](./images/one-loop-per-thread.png)

对于单线程下的 one loop per thread 模型，主要由 EventLoop、Channel 以及 Selector（在muduo的实现中，为 Poller）三个类来实现。
- EventLoop。EventLoop类是对事件循环的抽象和表示。对于单线程下的事件循环而言，如上图所示，一个事件循环主要负责以下两件事。（注意：我们应该将`事件`的概念抽象化，在本文中所指的`事件`应该是抽象层面的事件，而不是具体的事件，例如socket上的读写事件、timerfd的读事件，或者是普通文件描述符的读写事件）
	- 管理文件描述符上（socket、timerfd等）的读写事件。对于上面的这幅图，我们需要管理监听socket的读事件和已连接socket的读写事件。这部分由 Channel 类负责，一个 EventLoop 实例对象包含一个 Channel 数组成员。
	- 调用 IO multiplexing 函数，监听被当前IO线程所管理的文件描述符上的读写事件。这部分由 Selector 类负责。
- Channel。Channel类负责管理具体的文件描述符上的读写事件，以及读写事件到来时对应的回调函数。
- Selector。IO multiplexing 的抽象类，具体的实现由其子类继承实现。目前我实现的版本中，只实现了 Linux 下的 epoll IO multiplexing 函数，对应的实现类为 Epoll。一个EventLoop实例对象只拥有一个Selector的实例对象，EventLoop通过其Selector成员变量来管理多个Channel。

下面依次介绍 Channel、Selector 和 EventLoop 三个类的实现。
# Channel
Channel类包含的成员变量如下：
```cpp
class Channel {
// ...
public:
	enum class EventState {
		NEW,
		ADDED,
		DELETED
	};

	using WriteEventCallback = std::function<void()>;
	using ReadEventCallback = std::function<void()>;
	using ErrorEventCallback = std::function<void()>;
	using CloseEventCallback = std::function<void()>;
	
private:
	EventLoop *loop_;
	const int fd_;

	int requestedEvents_; // 关注的事件类型
	int returnedEvents_;  // 发生的事件类型

	EventState eventState_; 

	WriteEventCallback writeEventCallback_;
	ReadEventCallback readEventCallback_;
	ErrorEventCallback errorEventCallback_;
	CloseEventCallback closeEventCallback_;

	static const int kNoneEvent;      // kNoneEvent = 0
	static const int kReadEvent;      // kReadEvent = POLLIN | POLLPRI
	static const int kWriteEvent;     // kWriteEvent = POLLOUT
};
```

Channel类中每个成员变量的作用：
- `EventLoop *loop_` 。EventLoop 和 Channel 是一对多的关系，一个EventLoop实例对象通过其Selector成员变量来间接管理多个Channel实例对象。一个Channel实例对象通过包含其所属的EventLoop实例对象的指针来保持这对应关系。
- `const int fd_`。Channel实例对象管理的事件所属的文件描述符。这里需要注意，Channel只负责管理文件描述符上的需要关注哪些事件，以及关注的事件发生后调用对应的回调函数。而对于这些事件所属文件描述符的生命周期，不用关心，即文件描述符什么时候需要调用 close(fd_) 关闭，Channel不负责任。至于具体的文件描述符的生命周期，由不同的类来管理，这里不进行展开。
- `int requestedEvents_` 和 `int returnedEvents_`。`requestedEvents_` 表示 IO multiplexing 函数关注文件描述符上的事件类型，成员函数 `enableRead()` 和 `enableWrite()` 设置该变量的值。`returnedEvents_` 表示 IO multiplexing 检测到有事件到来时，到来的事件类型是什么。
- `EventState eventState_`。表示文件描述符与IO multiplexing之间的状态关系。即 IO multiplexing 函数是否将检测文件描述符上的事件。以 epoll 为例，NEW 表示 fd 未添加到 epollfd 中，时 Channel 所指文件描述符的初始状态；ADDED 表示以添加到 epollfd 中，DELETED 表示已从 epollfd 中删除。
- `xxxCallback` 表示对应事件的回调函数，一般在Channel实例对象初始化后由对应的成员函数设置。
- 三个静态成员变量，用于表示事件类型。因为本网络库是在linux下开发，在linux下，poll和epoll中的读写宏定义中，其值相等。因此使用了poll中的宏定义表示可读可写，更好的做法是，自定义一个类来封装poll和epoll的底层事件类型，做到对外统一，后续对这部分进行改进。

结合 Channel 类中每个成员变量的作用弄清 Channel 类的职责后，其成员函数的实现就很好理解了，这里就不在一一罗列。

# Selector
Selector类是IO复用函数的抽象类，定义了实现的接口。目前本库只实现了linux下的epoll IO 复用函数，其实现类为 Epoll。Selector类和Epoll类包含的成员变量如下：
```cpp
class Selector: public noncopyable
{
// ...
protected:
	// key为fd，即Channel中的fd_成员变量
	using ChannelMap = std::map<int, Channel*>;
	ChannelMap channelMap_;

private:
	EventLoop* loop_;
};

class Epoll: public Selector
{
// ...
private:
	using EventVector = std::vector<struct epoll_event>;
	EventVector events_;
	const int epollFd_;        
	static const int InitEventVectorSize = 16;

};
```

Selector类中每个成员变量的作用：
- `EventLoop* loop_` 。事件循环是基于 IO multiplexing 函数来实现的，EventLoop 和 Selector 是一一对应的，即一个 EventLoop 实例对象拥有一个 Selector 成员对象；反过来，一个 Selector 成员实例拥有包含它的EventLoop指针。
- `ChannelMap channelMap_`。map类型，用来保存 Selector 监控了哪些文件描述符，key为文件描述符，value为对应的 Channel 实例对象的指针。需要注意，map中的value为指针类型，即Selector不负责Channel实例对象的生命周期。

Epoll类继承自Selector，其每个成员变量的作用为：
- `const int epollFd_`。epoll实例对应的文件描述符。
- `EventVector events_`。用于接收 epoll_wait 从内核拷贝回用户态的 epoll_event 数组。
- `static const int InitEventVectorSize = 16` 。表示初始时 event_ 的数组大小。

Epoll类的实现较为简单，其成员函数 select 是对 epoll_wait 的封装，updateChannel 是对 epoll_ctl 的封装，这里不粘贴出具体的实现细节。

这里举一个代码示例来帮助理解 Channel 类是如何和 Selector 类协作实现文件描述符的事件管理的。以使用 epoll IO 复用函数为例，使用 epoll 管理一个文件描述符上的读事件的一般写法如下：
```cpp
// 省略文件描述符 fd 的创建/获取
// 省略 epoll 文件描述符 epollfd 的创建/获取

struct epoll_event event;
event.data.fd = fd;
event.events = EPOLLIN;
epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

// ...
struct epoll_event events[EVENT_NUMBER];
int event_numbers = epoll_wait(epollfd, events, EVENT_NUMBER, -1);
for (int i = 0; i < event_numbers; ++i) {
	// 根据 events[i].events 的值来判断事件类型
	if (events[i].events == EPOLLIN) {
		// 处理读事件
	}
}
```

使用 Channel 和 Selector 封装后，上述代码等价如下：
```cpp
// 创建一个 Epoll 对象，初始化过程中会调用 epoll_create 创建一个 epollfd 文件描述符
Selector selector = new Epoll(loop);

// 创建一个 Channel 实例对象，管理 fd 上的读写事件
Channel channel(loop, fd);

// 调用 enable() 方法，其内部封装了 `epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event)` 的实现
/*
enableRead 的内部实现等价于：
struct epoll_event event;
event.events = channel.events();
event.data.ptr = &channel;
*/
channel.enableRead();

/*
	其内部实现为：
	int num = epoll_wait();
	for (int i = 0; i < num; ++i) {
		Channel* channel = events[i].data.ptr;
		channel->handEvents();
	}
*/
selector.select();
```

在网络编程中，我们关注文件描述符上的读、写事件，异常行为可以通过读文件描述符来捕获，当读或写事件发生时，根据文件描述符的类型执行对应的操作。除了读、写事件发生后，执行的读写操作不同外，使用 IO multiplexing 函数管理、监听文件描述符可读可写状态等行为都是相同的逻辑流程。Channel 和 Selector 对这套相同的逻辑流程进行了封装，Channel 类通过暴露 `setWriteEventCallback`、`setReadEventCallback` 等接口，来实现不同文件描述符上的读写操作；Selector 类通过其 updateChannel 成员方法将 Channel 对象添加到 IO multiplexing 函数中管理。

# EventLoop
EventLoop 成员变量如下。
```cpp
class EventLoop
{
public:
	using Func = std::function<void()>;

private:
	using ChannelVector = std::vector<Channel*>;

	const std::thread::id tid_;     // EventLoop所在线程的唯一标识
	std::unique_ptr<Selector> selector_;

	std::atomic_bool looping_;
	std::atomic_bool running_;

	std::unique_ptr<TimerQueue> timerQueue_;

	// 下面这些成员变量与跨线程调用有关
	std::mutex mutex_;
	int wakeupFd_;
	std::unique_ptr<Channel> wakeupChannel_;
	std::vector<Func> pendingFunctions_;
	bool callingPendingFunctions_;
};
```

one loop per thread 对应的实现上，loop 即指 EventLoop。EventLoop 类自己不负责具体的事物处理，具体的事件循环委托给其 Selector 成员变量来实现，它起到桥梁的作用，把Channel 和 Selector 连接起来，Selector 管理并监听文件描述符是可读还是可写，Channel 负责读写事件发生后需要进行的操作（通过回调函数实现），EventLoop 将两者串联起来。看下 EventLoop 中事件循环的核心代码。
```cpp
void EventLoop::loop()
{
	// using ChannelVector = std::vector<Channel*>;
	// 保存IO multiplexing检测到的有事件发生的文件描述符对应的Channel
	ChannelVector activeChannels;
	while (running_)
	{
		activeChannels.clear();

		// selector = new Epoll(...);
		selector_->select(activeChannels, Epoll::EPOLL_TIMEOUT);

		// 执行 events 上注册的回调函数
		for (auto channel : activeChannels)
		{
			channel->handleEvents();
		}
	}

	looping_ = false;
}
```

这里贴出 Epoll 类中 select 成员函数和 Channel 类中 handleEvents 成员函数的实现，方便与上述代码对照。
```cpp
void Epoll::select(ChannelVector& activeChannels, int timeout)
{
	int returnedEventsNum = epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeout);
	LOG_DEBUG << "epoll_wait once...";
	if (returnedEventsNum < 0) {
		// FIXME: error
	}
	else if (returnedEventsNum == 0) {
		// no events happening, maybe timeout
	}
	else {
		// 取出发生的事件
		assert(static_cast<size_t>(returnedEventsNum) <= events_.size());
		for (int i = 0; i < returnedEventsNum; ++i) {
			Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		
			channel->setReturnedEvent(events_[i].events);
			activeChannels.emplace_back(channel);
		}

		/**
		 * 当前用户态中用于接收事件的数组太小,不能一次性从内核态拷贝到用户态,把数组大小扩大一倍
		*/
		if (static_cast<size_t>(returnedEventsNum) == events_.size()) {
			events_.resize(events_.size() * 2);
		}
	}
}

void Channel::handleEvents()
{
	if (returnedEvents_ & POLLOUT) {
		if (writeEventCallback_) {
			writeEventCallback_();
		}
	}

	if (returnedEvents_ & POLLNVAL) {
		// POLLNVAL - Invalid request: fd not open (only returned in revents; ignored in events).
	}

	if (returnedEvents_ & POLLERR) {
		if (errorEventCallback_) {
			errorEventCallback_();
		}
	}

	if (returnedEvents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
		if (readEventCallback_) {
			readEventCallback_();
		}
	}

	if (returnedEvents_ & POLLHUP && !(returnedEvents_ & POLLIN)) {
		if (closeEventCallback_) {
			closeEventCallback_();
		}
	}
}
```

one loop per thread 中的 "per thread" 指每个线程中最多有一个事件循环，一个事件循环只属于一个线程，将创建了EventLoop实例对象的线程称为IO线程。muduo的实现中，EventLoop实例对象可以跨线程调用，但都会被转到创建 EventLoop 实例对象所属的线程中执行，至于为什么这么做，muduo书中的4.6节多线程与IO给出了很好的解释。下面看下EventLoop的跨线程调用是如何实现的。

EventLoop 对外提供了 runInLoop 接口，允许跨线程调用，若调用 loop.runInLoop 函数的线程是创建 loop 对象时所在的线程，则直接执行，否则调用 queueInLoop 函数。
```cpp
bool isInLoopThread() const { return tid_ == std::this_thread::get_id(); }

void EventLoop::runInLoop(Func func)
{
	if (func)
	{
		if (isInLoopThread())
		{
			func();
		}
		else
		{
			queueInLoop(std::move(func));
		}
	}
}

void EventLoop::queueInLoop(Func func)
{
	{
		std::unique_lock<std::mutex> locker(mutex_);
		pendingFunctions_.emplace_back(std::move(func));
	}

	if (!isInLoopThread() || callingPendingFunctions_)
	{
		wakeup();
	}
}
```

queueInLoop 将待执行的函数添加到一个函数队列中（`std::vector<Func>`），然后根据条件来判断是否调用 wakeup 函数唤醒 loop 被创建时所在的线程，让待执行的函数在 loop 所在的线程执行。因为 loop 所在的线程会被阻塞在IO 复用函数上（例如，epoll_wait），而 `loop.runInLoop(func)` 会将 func 转移到 loop 所在的线程执行，且我们是希望 func 能够被立马执行，而不要被阻塞，因此当 loop 所在的线程被阻塞在 epoll_wait 上时，我们需要唤醒它，然后执行该函数。执行流程如下所示：
```cpp
// 调用 eventfd 创建一个 fd
int createEventfd()
{
	int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
	if (fd < 0)
	{
		LOG_FATAL << "eventfd*() error";
	}
	return fd;
}

EventLoop::EventLoop() : looping_(false), running_(false),
                             tid_(std::this_thread::get_id()),
                             selector_(new Epoll(this)),
                             wakeupFd_(createEventfd()),
                             wakeupChannel_(new Channel(this, wakeupFd_)),
                             callingPendingFunctions_(false),
                             timerQueue_(new TimerQueue(this))
{
	LOG_DEBUG << "EventLoop created.";

	if (loopInCurrentThread)
	{
		// 一个线程中最多创建一个 EventLoop 对象
		LOG_FATAL << "A thread can create a maximum of one EventLoop object.";
	}
	else
	{
		loopInCurrentThread = this;
	}

	// EventLoop对象初始化时，创建一个 wakeupFd_ 和对应的 Channel，然后关注读事件
	wakeupChannel_->setReadEventCallback(std::bind(&EventLoop::wakeupReadCallback, this));
	wakeupChannel_->enableRead();
}

// wakeup 会往 wakeupFd_ 写入数据; 然后 epoll_wait 监测到 wakeupFd_ 可读，阻塞解除
void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
	{
		LOG_ERROR << "EventLoop::wakeup() should write 8 bytes, not " << n << " bytes";
	}
}

void EventLoop::loop()
{
	// ...

	ChannelVector activeChannels;
	while (running_)
	{
		activeChannels.clear();

		// 当没有事件发生，select被阻塞。wakeup() 函数将此调用唤醒
		selector_->select(activeChannels, Epoll::EPOLL_TIMEOUT);
	
		// ...

		// 执行 pendingFunctions_ 中保存的函数对象
		doPendingFunctions();
	}

	looping_ = false;
}

void EventLoop::doPendingFunctions()
{
	std::vector<Func> functions;
	callingPendingFunctions_ = true;

	{
		std::unique_lock<std::mutex> locker(mutex_);
		functions.swap(pendingFunctions_);
	}

	for (auto &func : functions)
	{
		func();
	}

	callingPendingFunctions_ = false;
}
```

EventLoop还提供了定时器的接口，在 [《定时器》](定时器.md)中介绍该部分。

---

至此，单线程下的 one loop per thread介绍完毕，使用 EventLoop 类就能够很方便的构建出一个简单的单线程TCP程序。多线程下的 one loop per thread 的实现在 《muduo源码分析之Thread、EventLoopThread和EventLoopThreadPool》中讲解。
