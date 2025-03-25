#pragma once
#include <string>

class CommandReader
{
    public:
        // No copying.
        CommandReader(const CommandReader &) = delete;
        CommandReader(CommandReader &&) = delete;
        CommandReader &operator=(const CommandReader &) = delete;
        CommandReader &operator=(CommandReader &&) = delete;

        /// @brief Reads the line from the terminal.
        /// @return True on success. False on failure.
        static bool read_line(void);

        /// @brief Reads the next parameter of the string until the next space and writes it to out.
        /// @param out String to write the next parameter to.
        /// @return True on success. False if an error occurs.
        static bool get_next_parameter(std::string &out);

    private:
        /// @brief String the line is read into.
        std::string m_line;

        /// @brief Current offset/position in line.
        size_t m_offset = 0;

        /// @brief Default, private constructor.
        CommandReader(void) = default;

        /// @brief Returns the only instance allowed.
        /// @return Reference to the instance.
        static CommandReader &get_instance(void)
        {
            static CommandReader instance{};
            return instance;
        }
};