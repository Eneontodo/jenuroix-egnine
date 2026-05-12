#pragma once
#include <string>
#include <sstream>
#include <functional>
#include <vector>
#include <cstdio>
#include <chrono>

// LogLevel
enum class LogLevel { Trace, Info, Warning, Error, Fatal };

// Log entry
struct LogEntry {
    LogLevel    level;
    std::string message;
    std::string file;
    int         line;
    float       timestamp; // секунды с запуска
};

using LogSink = std::function<void(const LogEntry&)>;

// Central logger
class Log {
public:
    static Log& get() { static Log inst; return inst; }

    void addSink(LogSink sink) { m_sinks.push_back(std::move(sink)); }

    void write(LogLevel lvl, const std::string& msg,
                const char* file = "", int line = 0)
    {
        using namespace std::chrono;
        float ts = duration<float>(
            steady_clock::now() - m_start).count();

        LogEntry e { lvl, msg, file, line, ts };
        m_entries.push_back(e);
        if (m_entries.size() > MAX_ENTRIES)
            m_entries.erase(m_entries.begin());

        for (auto& sink : m_sinks) sink(e);
    }

    const std::vector<LogEntry>& entries() const { return m_entries; }

    // Filter by level
    std::vector<LogEntry> filter(LogLevel minLevel) const {
        std::vector<LogEntry> out;
        for (auto& e : m_entries)
            if (e.level >= minLevel) out.push_back(e);
        return out;
    }

private:
    Log() {
        // Default sink — stdout with color
        addSink([](const LogEntry& e) {
            const char* prefix = "";
            const char* color  = "";
            const char* reset  = "\033[0m";
            switch (e.level) {
                case LogLevel::Trace:   prefix="[TRC]"; color="\033[90m"; break;
                case LogLevel::Info:    prefix="[INF]"; color="\033[0m";  break;
                case LogLevel::Warning: prefix="[WRN]"; color="\033[33m"; break;
                case LogLevel::Error:   prefix="[ERR]"; color="\033[31m"; break;
                case LogLevel::Fatal:   prefix="[FTL]"; color="\033[35m"; break;
            }
            fprintf(stderr, "%s%s %.2fs  %s%s\n",
                color, prefix, e.timestamp, e.message.c_str(), reset);
        });
    }

    std::vector<LogEntry> m_entries;
    std::vector<LogSink>  m_sinks;
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
    static constexpr size_t MAX_ENTRIES = 1024;
};

// Logging macros
#define _LOG_WRITE(level, msg) \
    do { std::ostringstream _ss; _ss << msg; \
        Log::get().write(level, _ss.str(), __FILE__, __LINE__); } while(0)

#define LOG_TRACE(msg)   _LOG_WRITE(LogLevel::Trace,   msg)
#define LOG_INFO(msg)    _LOG_WRITE(LogLevel::Info,    msg)
#define LOG_WARN(msg)    _LOG_WRITE(LogLevel::Warning, msg)
#define LOG_ERROR(msg)   _LOG_WRITE(LogLevel::Error,   msg)
#define LOG_FATAL(msg)   _LOG_WRITE(LogLevel::Fatal,   msg)

// Compatibility with old LOG()
#define LOG(msg) LOG_INFO(msg)
#define LOGW(msg) LOG_WARN(msg)
#define LOGE(msg) LOG_ERROR(msg)
