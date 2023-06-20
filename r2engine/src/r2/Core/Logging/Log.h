//
//  Log.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-19.
//

#ifndef Log_h
#define Log_h

#if defined(__clang__) || defined(__GNUC__)
// Helper macro for declaring functions as having similar signature to printf.
// This allows the compiler to catch format errors at compile-time.
#define R2_PRINTF_LIKE(fmtarg, firstvararg) __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#elif defined(_MSC_VER)
#define R2_PRINTF_LIKE(fmtarg, firstvararg)
#else
#define R2_PRINTF_LIKE(fmtarg, firstvararg)
#endif

#if defined(_MSC_VER)
#define R2_PREDICT_FALSE(x) (x)
#define R2_PREDICT_TRUE(x)  (x)
#else
#define R2_PREDICT_FALSE(x) (__builtin_expect(x,     0))
#define R2_PREDICT_TRUE(x)  (__builtin_expect(!!(x), 1))
#endif

#include "r2/Platform/MacroDefines.h"
#include <string>

namespace loguru
{
    struct Message;
}

using MessageHandle = loguru::Message;

namespace r2
{
    class R2_API Log
    {
    public:
    
        using Verbosity = int;
        
        static Verbosity MAX;
        static Verbosity FATAL;
        static Verbosity ERR;
        static Verbosity WARN;
        static Verbosity INFO;
        static Verbosity STD_ERR_OFF;

        enum Filemode
        {
            TRUNC,
            APPEND
        };
        
        using LogHandlerId = const char*;
        using UserData = void*;
        
        struct Message
        {
            // You would generally print a Message by just concating the buffers without spacing.
            // Optionally, ignore preamble and indentation.
            Verbosity   verbosity;   // Already part of preamble
            const char* filename;    // Already part of preamble
            unsigned    line;        // Already part of preamble
            const char* preamble;    // Date, time, uptime, thread, file:line, verbosity.
            const char* indentation; // Just a bunch of spacing.
            const char* prefix;      // Assertion failure info goes here (or "").
            const char* message;     // User message goes here.
        };
        
        typedef void (*LogHandler)(UserData, const MessageHandle& message);
        typedef void (*CloseHandler)(UserData);
        typedef void (*FlushHandler)(UserData);
        typedef void (*FatalHandler)(const MessageHandle& message);

        using FormatString = const char*;
        
        static void Init(Verbosity stderrVerbosity, const std::string& appLogPath, const std::string& devLogPath = "");
        static void AddLogFile(const std::string& path, Filemode mode, Verbosity verbosity);
        
        static void AddLogHandler(LogHandlerId id, LogHandler handler, UserData data, Verbosity verbosity, CloseHandler closeHandler = nullptr, FlushHandler flushHander = nullptr );
        static bool RemoveLogHandler(LogHandlerId id);
        static void AddFatalHandler(FatalHandler handler);
        static void LogPrint(Verbosity verbosity, const char* file, unsigned line, FormatString format, ...) R2_PRINTF_LIKE(4, 5);
        static void LogAndAbort(int stack_trace_skip, const char* expr, const char* file, unsigned line, FormatString format, ...) R2_PRINTF_LIKE(5, 6);
    
        static Verbosity CurrentVerbosityCutoff();
        
        static void GetMsg(Message& msg, const MessageHandle& msgHandle);
    };
}

#define R2_VLOG(verbosity, ...)                                                                     \
((verbosity) > r2::Log::CurrentVerbosityCutoff()) ? (void)0                                   \
: r2::Log::LogPrint(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define R2_LOG(verbosity_name, ...) R2_VLOG(r2::Log::verbosity_name, __VA_ARGS__)

#define R2_LOGI(...) R2_LOG(INFO, __VA_ARGS__)
#define R2_LOGW(...) R2_LOG(WARN, __VA_ARGS__)
#define R2_LOGE(...) R2_LOG(ERR, __VA_ARGS__)

#define R2_VLOG_IF(verbosity, cond, ...)                                                            \
((verbosity) > r2::Log::CurrentVerbosityCutoff() || (cond) == false)                          \
? (void)0                                                                                  \
: r2::Log::LogPrint(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define R2_LOG_IF(verbosity_name, cond, ...)                                                        \
R2_VLOG(r2::Log::verbosity_name, cond, __VA_ARGS__)

#define R2_CHECK_WITH_INFO(test, info, ...)                                                         \
R2_PREDICT_TRUE((test) == true) ? (void)0 : r2::Log::LogAndAbort(0, "CHECK FAILED:  " info "  ", __FILE__,      \
__LINE__, ##__VA_ARGS__)

#if defined R2_DEBUG || defined R2_RELEASE
#define R2_CHECK(test, ...) R2_CHECK_WITH_INFO(test, #test, ##__VA_ARGS__)
#else
#define R2_CHECK(test, ...)
#endif

#if defined R2_DEBUG || defined R2_RELEASE
#define TODO R2_CHECK(false, "@TODO(Serge): implement")
#else
#define TODO
#endif

#endif /* Log_hpp */
