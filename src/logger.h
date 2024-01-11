#ifndef LOGGER_H
#define LOGGER_H

#include <sys/time.h>
#include <string>
#include <iostream>
#include <string_view>
#include <sstream>
#include <memory>
#include <map>
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>

#include "noncopyable.h"
#include "Buffer.h"
#include "Thread.h"
#include "CountDownLatch.h"

namespace cpp_example
{
    class Logger;
    class LogContext;
    class LogRecorder; 
    class LogHandler;   
    class AsyncLogging;

    using LogContextPtr = std::shared_ptr<LogContext>;

    const int LogFileBufferSize = 1024 * 16;

    enum LogLevel
    {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    class Logger: public std::enable_shared_from_this<Logger>, public noncopyable
    {
    public:

        using Ptr = std::shared_ptr<Logger>;

        Logger() = default;
        explicit Logger(std::string_view loggerName);
        ~Logger();

        /* 单例模式 */
        static Logger& instance();

        /**
         * @brief 将实际的日志消息推送到不同的 LogHandler
         * 
         * @param context 
         */
        void write(const LogContextPtr& context);

        /**
         * @brief 添加 LogHandler
         * 
         * @param handler 
         */
        void addHandler(const std::shared_ptr<LogHandler>& handler);

        void delHandler(std::string_view name);

        void setLoggerName(const std::string_view loggerName);

        void setAsyncLogger(const std::shared_ptr<AsyncLogging>& asyncLogger);

    
    private:
        /**
         * @brief 调用注册到Logger中的LogHandler进行写日志
         * 
         * @param context 
         */
        void handlerWrite(const LogContextPtr& context);
        

    private:
        std::string loggerName_;
        std::map<std::string, std::shared_ptr<LogHandler>> handlers_;
        std::shared_ptr<AsyncLogging> asyncLogger_;
    };


    class LogContext: public std::ostringstream
    {
    public:
        LogContext() = default;
        LogContext(std::string_view sourceFilename, std::string funcname, int line, LogLevel logLevel);
        ~LogContext() = default;
    
    public:
        int threadId_;
        LogLevel logLevel_;
    };


    /**
     * @brief 记录日志消息。
     * 完整的日志消息格式为：20231226 20:45:32 652349 INFO main.cpp funcname 20 | log message
     * 
     */
    class LogRecorder
    {
    public:
        LogRecorder(const std::string_view sourceFilename, std::string funcname, int line, LogLevel logLevel, Logger &logger);
        
        ~LogRecorder();

        template<typename T>
        LogRecorder& operator<<(T&& data)
        {
            if (!context_) {
                return *this;
            }
            (*context_) << std::forward<T>(data);
            return *this;
        }

    private:
        LogContextPtr context_;
        Logger& logger_;
    };


    class LogHandler: public noncopyable
    {
    public:
        LogHandler(std::string_view name, LogLevel logLevel);
        virtual ~LogHandler();

        void setLogLevel(LogLevel LogLevel);

        const std::string& name() const;

        virtual void write(const LogContextPtr& context) = 0;
    
    protected:
        LogLevel logLevel_;
        std::string name_;
    };


    class ConsoleHandler: public LogHandler
    {
    public:
        ConsoleHandler(std::string_view name, LogLevel logLevel);

        ~ConsoleHandler() override = default;

        void write(const LogContextPtr& context) override;
    };

    
    /**
     * @brief write log to file, no thread safe
     * 
     */
    class LogFile: public noncopyable
    {
    public:
        explicit LogFile(std::string_view filename);

        ~LogFile();

        void append(const LogContextPtr& context);

        void append(const char* buf, int len);

        void flush(); 

        off_t writtenBytes() const;

    private:
        std::string filename_;
        FILE* fp_;
        char buffer_[LogFileBufferSize];
        off_t writtenBytes_;
    };


    class FileHandler: public LogHandler
    {
    public:
        FileHandler(std::string_view name, LogLevel logLevel, std::string_view filepath,
                    int flushInterval=3, int flushEveryNLine=100 * 100, int fileRollSize=1024*1024*64, bool threadSafe=true);

        ~FileHandler() override = default;

        void write(const LogContextPtr& context) override;

        void append(const LogContextPtr& context);

        void append(const char* buf, int len);

        bool rollFile();

        void flush();

        static std::string getFilename(std::string_view filepath, time_t& now);

    private:
        void append();

    private:
        std::string filepath_;
        std::unique_ptr<LogFile> logFile_;

        const int flushInterval_;
        const int flushEveryNLine_;
        const int fileRollSize_;
        int lineCount_;

        time_t lastRollTime_;       /* 上一次日志存档的时间 */
        time_t lastFlushTime_;      /* 上一次文件缓冲区刷新的时间 */
        time_t lastRollPeriod_;     /* 上一次日志按照周期存档的时间 */

        std::unique_ptr<std::mutex> mutex_;

        const int rollPeriodPerSeconds_ = 60 * 60 * 24;
    };


    const int AsyncBufferSize = 4000000;

    class AsyncLogging: public noncopyable
    {
    public:
        using Buffer = FixedBuffer;
        using BufferVector = std::vector<std::unique_ptr<Buffer>>;
        using BufferPtr = BufferVector::value_type;

        AsyncLogging(std::string_view logFilepath, LogLevel logLevel, int fileRollSize=64 * 1024, int flushInterval=3, int flushEveryNLine=100);

        ~AsyncLogging();

        void start();

        void stop();

        void append(const LogContextPtr& context);

    private:
        void run();

        std::atomic_bool running_;
        std::string logFilepath_;       /* 日志文件路径 */
        const int fileRollSize_;        /* 日志文件大小 */
        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;
        BufferVector buffers_;
        CountDownLatch latch_;
        std::mutex mutex_;
        std::condition_variable_any cv_;
        Thread thread_;
        LogLevel logLevel_;
        const int flushInterval_;
        const int flushEveryNLine_;
    };


#define LOGGER(LEVEL) LogRecorder(__FILE__, __FUNCTION__, __LINE__, LEVEL, Logger::instance())
#define LOG_TRACE LOGGER(LogLevel::TRACE)
#define LOG_DEBUG LOGGER(LogLevel::DEBUG)
#define LOG_INFO LOGGER(LogLevel::INFO)
#define LOG_WARN LOGGER(LogLevel::WARN)
#define LOG_ERROR LOGGER(LogLevel::ERROR)
#define LOG_FATAL LOGGER(LogLevel::FATAL)


} /* namespace cpp_example */

#endif