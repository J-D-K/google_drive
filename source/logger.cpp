#include "logger.hpp"
#include <cstdarg>
#include <fstream>
#include <string_view>

namespace
{
    /// @brief Size of the buffer for logging messages.
    constexpr int VA_BUFFER_SIZE = 0x1000;

    /// @brief Path/name of the log file.
    constexpr std::string_view LOG_FILE_PATH = "./log.txt";
} // namespace

void logger::initialize(void)
{
    std::ofstream logFile(LOG_FILE_PATH.data());
}

void logger::log(const char *format, ...)
{
    // Buffer for formatting string.
    char vaBuffer[VA_BUFFER_SIZE] = {0};

    // VA
    std::va_list vaList;
    va_start(vaList, format);
    std::vsnprintf(vaBuffer, VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    std::ofstream logFile(LOG_FILE_PATH.data(), logFile.app);
    if (!logFile.is_open())
    {
        return;
    }
    // Write the buffer with std::endl for the flush.
    logFile << vaBuffer << std::endl;
}
