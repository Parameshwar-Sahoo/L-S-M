#include "utils/Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace lms {

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

Logger::~Logger()
{
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}

void Logger::init(const std::string& logFilePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        m_fileStream.open(logFilePath, std::ios::out | std::ios::app);
        m_initialized = true;
    }
}

void Logger::log(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm tmStruct{};
    localtime_r(&in_time_t, &tmStruct);

    std::ostringstream timeStream;
    timeStream << std::put_time(&tmStruct, "%Y-%m-%d %H:%M:%S")
               << '.' << std::setfill('0') << std::setw(3) << ms.count();

    const char* levelStr = "INFO";
    const char* colorCode = "\033[0m";

    switch (level) {
    case LogLevel::DEBUG_LVL:
        levelStr = "DEBUG";
        colorCode = "\033[36m"; // Cyan
        break;
    case LogLevel::INFO_LVL:
        levelStr = "INFO";
        colorCode = "\033[32m"; // Green
        break;
    case LogLevel::WARN_LVL:
        levelStr = "WARN";
        colorCode = "\033[33m"; // Yellow
        break;
    case LogLevel::ERROR_LVL:
        levelStr = "ERROR";
        colorCode = "\033[31m"; // Red
        break;
    case LogLevel::SECURITY_LVL:
        levelStr = "ALERT";
        colorCode = "\033[1;35m"; // Bold Magenta
        break;
    }

    // Print to Console
    std::cout << "\033[90m[" << timeStream.str() << "]\033[0m "
              << colorCode << "[" << levelStr << "]\033[0m "
              << message << std::endl;

    // Append to file
    if (m_fileStream.is_open()) {
        m_fileStream << "[" << timeStream.str() << "] [" << levelStr << "] "
                     << message << "\n";
        m_fileStream.flush();
    }
}

} // namespace lms
