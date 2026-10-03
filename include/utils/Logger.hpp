#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

namespace lms {

enum class LogLevel {
    DEBUG_LVL,
    INFO_LVL,
    WARN_LVL,
    ERROR_LVL,
    SECURITY_LVL
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& logFilePath = "libsys_audit.log");
    void log(LogLevel level, const std::string& message);

    void debug(const std::string& msg) { log(LogLevel::DEBUG_LVL, msg); }
    void info(const std::string& msg)  { log(LogLevel::INFO_LVL, msg); }
    void warn(const std::string& msg)  { log(LogLevel::WARN_LVL, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR_LVL, msg); }
    void security(const std::string& msg) { log(LogLevel::SECURITY_LVL, msg); }

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex m_mutex;
    std::ofstream m_fileStream;
    bool m_initialized{false};
};

} // namespace lms

#endif // LOGGER_HPP
