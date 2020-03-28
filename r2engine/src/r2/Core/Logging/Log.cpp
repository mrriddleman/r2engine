//
//  Log.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-19.
//
#include "r2pch.h"
#include "Log.h"
#include <cstring>
#include "loguru.hpp"

namespace r2
{
    
    using Message = loguru::Message;
    
    Log::Verbosity Log::MAX = loguru::Verbosity_MAX;
    Log::Verbosity Log::FATAL = loguru::Verbosity_FATAL;
    Log::Verbosity Log::ERR = loguru::Verbosity_ERROR;
    Log::Verbosity Log::WARN = loguru::Verbosity_WARNING;
    Log::Verbosity Log::INFO = loguru::Verbosity_INFO;
    Log::Verbosity Log::STD_ERR_OFF = loguru::Verbosity_OFF;
    
    void Log::Init(Verbosity stderrVerbosity, const std::string& fullLogPath, const std::string& devLogPath)
    {
        loguru::r2_init(stderrVerbosity);
        loguru::add_file(fullLogPath.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
        if(!devLogPath.empty())
        {
            loguru::add_file(devLogPath.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
        }
    }
    
    void Log::AddLogFile(const std::string& path, Log::Filemode mode, Log::Verbosity verbosity)
    {
        loguru::add_file(path.c_str(), static_cast<loguru::FileMode>(mode), verbosity);
    }
    
    void Log::AddLogHandler(LogHandlerId id, LogHandler handler, UserData data, Log::Verbosity verbosity, CloseHandler closeHandler, FlushHandler flushHander)
    {
        loguru::add_callback(id, handler, data, verbosity, closeHandler, flushHander);
    }
    
    bool Log::RemoveLogHandler(LogHandlerId id)
    {
        return loguru::remove_callback(id);
    }
    
    void Log::AddFatalHandler(FatalHandler handler)
    {
        loguru::set_fatal_handler(handler);
    }
    
    void Log::LogPrint(Verbosity verbosity, const char* file, unsigned line, FormatString format, ...)
    {
        va_list argp;
        va_start(argp,format);
        loguru::r2log(verbosity, file, line, format, argp);
        va_end(argp);
    }
    
    void Log::LogAndAbort(int stack_trace_skip, const char* expr, const char* file, unsigned line, FormatString format, ...)
    {
        va_list argp;
        va_start(argp, format);
        loguru::r2_log_and_abort(stack_trace_skip, expr, file, line, format, argp);
        va_end(argp);
    }
    
    Log::Verbosity Log::CurrentVerbosityCutoff()
    {
        return static_cast<Log::Verbosity>(loguru::current_verbosity_cutoff());
    }
    
    void Log::GetMsg(Message& msg, const MessageHandle& msgHandle)
    {
        msg.filename = msgHandle.filename;
        msg.indentation = msgHandle.indentation;
        msg.message = msgHandle.message;
        msg.preamble = msgHandle.preamble;
        msg.prefix = msgHandle.prefix;
        msg.line = msgHandle.line;
        msg.verbosity = msgHandle.verbosity;
    }
}
