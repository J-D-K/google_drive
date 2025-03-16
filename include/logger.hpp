#pragma once

namespace logger
{
    /// @brief Initializes and creates the log file.
    void initialize(void);

    /// @brief Writes a formatted string to the log.
    /// @param format Format of the string.
    /// @param args Arguments to format the string with.
    void log(const char *format, ...);
} // namespace logger
