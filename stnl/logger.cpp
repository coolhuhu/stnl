#include "logger.h"
#include <assert.h>
#include <iomanip>

namespace stnl
{

    void formatTime(struct timeval &tv, char *buffer, int buffer_size = 20)
    {
        struct tm t;
        gmtime_r(&(tv.tv_sec), &t);
        strftime(buffer, buffer_size, "%Y%m%d %H:%M:%S", &t);
    }

    const char *LogLevelName[LogLevel::NUM_LOG_LEVELS] = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
    };

    /* --------------------- Logger ----------------------------- */

    Logger::Logger(std::string_view loggerName) : loggerName_(loggerName) {}

    Logger &Logger::instance()
    {
        static Ptr instance_(new Logger());
        static Logger &s_instance_ref = *instance_;
        return s_instance_ref;
    }

    Logger::~Logger()
    {
    }

    void Logger::setLoggerName(const std::string_view loggerName)
    {
        loggerName_ = loggerName;
    }

    void Logger::addHandler(const std::shared_ptr<LogHandler> &handler)
    {
        // FIXME: key已存在的情况怎么处理
        handlers_.emplace(handler->name(), handler);
    }

    void Logger::delHandler(std::string_view name)
    {
        auto iter = handlers_.find(name.data());
        if (iter == handlers_.end())
        {
            return;
        }
        handlers_.erase(iter);
    }

    void Logger::write(const LogContextPtr &context)
    {
        if (asyncLogger_)
        {
            asyncLogger_->append(context);
        }
        else
        {
            handlerWrite(context);
        }
    }

    void Logger::handlerWrite(const LogContextPtr &context)
    {
        // 若没有设置LogHandler,则添加一个ConsoleHandler作为默认的LogHandler
        if (handlers_.empty())
        {
            handlers_.emplace("DefaultConsoleHandler", std::make_shared<ConsoleHandler>("DefaultConsoleHandler", LogLevel::INFO));
        }

        for (auto &handler : handlers_)
        {
            handler.second->write(context);
        }
    }

    void Logger::setAsyncLogger(const std::shared_ptr<AsyncLogging> &asyncLogger)
    {
        asyncLogger_ = asyncLogger;
    }

    /* --------------- LogContenxt ---------------------*/

    LogContext::LogContext(std::string_view sourceFilename, std::string funcname, int line, LogLevel logLevel) : logLevel_(logLevel)
    {
        if (logLevel == LogLevel::FATAL)
        {
        }

        struct timeval tv_;
        gettimeofday(&tv_, nullptr);

        char timeBuffer[18];
        formatTime(tv_, timeBuffer, sizeof(timeBuffer));
        *this << timeBuffer << ' ';
        *this << std::setw(6) << std::setfill('0') << tv_.tv_usec;
        *this << ' ' << LogLevelName[logLevel] << ' ';

        const char *filename = strrchr(sourceFilename.data(), '/');
        if (filename)
        {
            *this << filename + 1 << ' ';
        }

        *this << funcname << ' ' << line << " | ";

        // @todo 获取线程id
    }

    /* --------------------------- LogRecorder --------------------------*/

    LogRecorder::LogRecorder(const std::string_view sourceFilename,
                             std::string funcname, int line, 
                             LogLevel logLevel, 
                             Logger &logger) : 
                             context_(new LogContext(sourceFilename, funcname, line, logLevel)), 
                             logger_(logger)
    {
    }

    LogRecorder::~LogRecorder()
    {
        (*this) << "\n";
        logger_.write(context_);
        context_.reset();
    }

    /* --------------------------- LogHandler --------------------------*/

    LogHandler::LogHandler(std::string_view name, LogLevel logLevel) : name_(name), logLevel_(logLevel) {}

    LogHandler::~LogHandler() {}

    void LogHandler::setLogLevel(LogLevel LogLevel)
    {
        logLevel_ = LogLevel;
    }

    const std::string &LogHandler::name() const
    {
        return name_;
    }

    /* --------------------------- ConsoleHandler ------------------------*/

    ConsoleHandler::ConsoleHandler(std::string_view name, LogLevel logLevel) : LogHandler(name, logLevel) {}

    void ConsoleHandler::write(const LogContextPtr &context)
    {
        // 对小于LogHandler日志级别的日志消息直接忽略
        if (context->logLevel_ < logLevel_)
        {
            return;
        }

        std::cout << context->view();

        if (context->logLevel_ == LogLevel::FATAL)
        {
            std::cout.flush();
            abort();
        }
    }

    /* ------------------------- LogFile -------------------------------- */

    LogFile::LogFile(std::string_view filename) : filename_(filename), writtenBytes_(0)
    {
        /* ae，追加或者为写打开 */
        fp_ = fopen(filename_.c_str(), "ae");
        assert(fp_);
        setbuffer(fp_, buffer_, LogFileBufferSize);
    }

    LogFile::~LogFile()
    {
        fclose(fp_);
    }

    void LogFile::flush()
    {
        fflush(fp_);
    }

    off_t LogFile::writtenBytes() const
    {
        return writtenBytes_;
    }

    void LogFile::append(const LogContextPtr &context)
    {
        // @todo 添加线程id信息

        std::string logMessage(context->view());

        size_t written = 0;
        while (written != logMessage.size())
        {
            size_t remain = logMessage.size() - written;
            size_t n = fwrite_unlocked(logMessage.data() + written, 1, remain, fp_);
            if (n != remain)
            {
                int err = ferror(fp_);
                if (err)
                {
                    fprintf(stderr, "LogFile Append failed %s\n", strerror(err));
                    break;
                }
            }
            written += n;
        }

        writtenBytes_ += written;
    }

    // @FIXME:
    void LogFile::append(const char *buf, int len)
    {
        size_t written = 0;
        while (written != len)
        {
            size_t remain = len - written;
            size_t n = fwrite_unlocked(buf + written, 1, remain, fp_);
            if (n != remain)
            {
                int err = ferror(fp_);
                if (err)
                {
                    fprintf(stderr, "LogFile Append failed %s\n", strerror(err));
                    break;
                }
            }
            written += n;
        }
        // std::cout << __LINE__ << "written = " << written << " len = " << len << std::endl;
        writtenBytes_ += written;
    }

    /* --------------------------- FileHandler ---------------------------------- */

    FileHandler::FileHandler(std::string_view name, LogLevel logLevel, std::string_view filepath,
                             int flushInterval, int flushEveryNLine, int fileRollSize, bool threadSafe) : LogHandler(name, logLevel), filepath_(filepath), flushInterval_(flushInterval),
                                                                                                          flushEveryNLine_(flushEveryNLine), fileRollSize_(fileRollSize), lineCount_(0),
                                                                                                          lastRollTime_(0), lastFlushTime_(0), lastRollPeriod_(0), mutex_(threadSafe ? new std::mutex() : nullptr)
    {
        // @todo 对传入的日志路径进行校验，是否合法
        rollFile();
    }

    /**
     * 日志滚动（存档、归档）。日志文件达到指定大小（例如1GB）或周期存档（例如一天）。
     */
    bool FileHandler::rollFile()
    {
        time_t now = time(NULL);
        std::string filename = getFilename(filepath_, now);
        time_t newPeriod = now / rollPeriodPerSeconds_ * rollPeriodPerSeconds_;

        if (now > lastRollTime_) /* 这个判断必要吗？ 在调用rollFile的外部进行判断*/
        {
            // std::cout << __LINE__ << "now = " << now << " lastRollTime_ = " << lastRollTime_ << std::endl;
            lastRollTime_ = now;
            lastFlushTime_ = now;
            lastRollPeriod_ = newPeriod; /* 更新回滚周期 */

            /*
             TODO:
             这一步可以进行优化改进吗？不使用智能指针管理 LogFile 对象。
             复用同一块 buffer，绑定行的文件流，即绑定一个新的 FILE 对象。
            */
            logFile_.reset(new LogFile(filename));

            return true;
        }

        return false;
    }

    std::string FileHandler::getFilename(std::string_view filepath, time_t &now)
    {
        /**
         * 日志文件的名称应该是统一的，由程序库默认。
         * 而日志文件的保存路径可作为配置选项由用户端确定。
         */
        // FIXME: 文件路径和文件名拼接
        char buf[20];
        struct tm t;
        gmtime_r(&now, &t);
        strftime(buf, 20, "%Y%m%d-%H%M%H", &t);
        std::string filename(buf);
        filename += ".log";
        return filename;
    }

    void FileHandler::write(const LogContextPtr &context)
    {
        if (context->logLevel_ < logLevel_)
        {
            return;
        }

        // @todo write log to file
        if (mutex_)
        {
            std::unique_lock<std::mutex> locker(*mutex_);
            append(context);
        }
        else
        {
            append(context);
        }

        if (context->logLevel_ == LogLevel::FATAL)
        {
            flush();
            abort();
        }
    }

    void FileHandler::append(const LogContextPtr &context)
    {
        logFile_->append(context);

        append();
    }

    void FileHandler::append(const char *buf, int len)
    {
        logFile_->append(buf, len);
        append();
    }

    void FileHandler::append()
    {
        if (logFile_->writtenBytes() >= fileRollSize_)
        {
            rollFile();
        }
        else
        {
            ++lineCount_;
            if (lineCount_ >= flushEveryNLine_)
            {
                lineCount_ = 0;
                time_t now = time(NULL);
                time_t thisPeriod = now / rollPeriodPerSeconds_ * rollPeriodPerSeconds_;
                if (thisPeriod != lastRollPeriod_)
                {
                    rollFile();
                }
                else if (now - lastFlushTime_ > flushInterval_)
                {
                    lastFlushTime_ = now;
                    logFile_->flush();
                }
            }
        }
    }

    void FileHandler::flush()
    {
        if (mutex_)
        {
            std::unique_lock<std::mutex> locker(*mutex_);
            logFile_->flush();
        }
        else
        {
            logFile_->flush();
        }
    }

    /* -------------------------------- AsyncLogging ------------------------------- */

    AsyncLogging::AsyncLogging(std::string_view logFilepath, LogLevel logLevel, int fileRollSize,
                               int flushInterval, int flushEveryNLine) : logFilepath_(logFilepath), fileRollSize_(fileRollSize),
                                                                         logLevel_(logLevel), flushEveryNLine_(flushEveryNLine),
                                                                         running_(false), currentBuffer_(new Buffer(AsyncBufferSize)), latch_(1), flushInterval_(flushInterval),
                                                                         nextBuffer_(new Buffer(AsyncBufferSize)), thread_("AsyncLogging", std::bind(&AsyncLogging::run, this))
    {
        currentBuffer_->clear();
        nextBuffer_->clear();
        buffers_.reserve(16);
    }

    AsyncLogging::~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    void AsyncLogging::start()
    {
        assert(!running_);
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void AsyncLogging::stop()
    {
        running_ = false;
        cv_.notify_all();
        thread_.join();
    }

    void AsyncLogging::run()
    {
        assert(running_ == true);
        latch_.countDown();
        FileHandler output("AsyncLogging-FileHandler", logLevel_, logFilepath_, flushInterval_, flushEveryNLine_, fileRollSize_);
        BufferPtr newBuffer1(new Buffer(AsyncBufferSize));
        BufferPtr newBuffer2(new Buffer(AsyncBufferSize));
        newBuffer1->clear();
        newBuffer1->clear();
        BufferVector buffersToWrite;
        buffersToWrite.reserve(16);
        while (running_)
        {
            assert(newBuffer1 && newBuffer1->size() == 0);
            assert(newBuffer2 && newBuffer2->size() == 0);
            assert(buffersToWrite.empty());

            {
                std::unique_lock<std::mutex> locker(mutex_);
                if (buffers_.empty())
                {
                    // std::cout << __LINE__ << "后端buffer为空，等待buffer交换" << std::endl;

                    // flushInterval_ 时间间隔过后，即使前端线程的buffer还没有写满，
                    // 仍然将未写满的buffer与后端进行交换，以保证及时写日志。
                    cv_.wait_for(locker, std::chrono::seconds(flushInterval_));
                }
                buffers_.push_back(std::move(currentBuffer_));
                currentBuffer_ = std::move(newBuffer1);
                buffersToWrite.swap(buffers_);
                if (!nextBuffer_)
                {
                    nextBuffer_ = std::move(newBuffer2);
                }
            }

            assert(!buffersToWrite.empty());

            // FIXME:
            if (buffersToWrite.size() > 25)
            {
                struct timeval tv_;
                gettimeofday(&tv_, nullptr);

                char timeBuffer[18];
                formatTime(tv_, timeBuffer, sizeof(timeBuffer));

                std::ostringstream sout;
                sout << "Dropped log messages at " << timeBuffer << ' ';
                sout << std::setw(6) << std::setfill('0') << tv_.tv_usec << ' ';
                sout << buffersToWrite.size() - 2 << " larger buffers\n";

                fputs(sout.view().data(), stderr);
                output.append(sout.view().data(), static_cast<int>(sout.view().size()));
                buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
            }

            for (const auto &buffer : buffersToWrite)
            {
                output.append(buffer->begin(), buffer->size());
            }

            if (!newBuffer1)
            {
                assert(!buffersToWrite.empty());
                newBuffer1 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer1->reset();
            }

            if (!newBuffer2)
            {
                assert(!buffersToWrite.empty());
                newBuffer2 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer2->reset();
            }

            buffersToWrite.clear();
            output.flush();
        }

        output.flush();
    }

    void AsyncLogging::append(const LogContextPtr &context)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (currentBuffer_->remainingCapacity() > context->view().size())
        {
            currentBuffer_->append(context->view());
        }
        else
        {
            buffers_.push_back(std::move(currentBuffer_));

            if (nextBuffer_)
            {
                currentBuffer_ = std::move(nextBuffer_);
            }
            else
            {
                currentBuffer_.reset(new Buffer(AsyncBufferSize));
            }

            currentBuffer_->append(context->view());

            // 测试使用
            // std::cout << __LINE__ << "前端写满一块buffer" << std::endl;

            cv_.notify_one();
        }
    }

} // namespace cpp_example
