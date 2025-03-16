#pragma once
#include <string>

namespace stringutil
{
    /// @brief Strips a character from the input string.
    /// @param target Target string to strip the character from.
    /// @param c Character to strip from the string.
    void strip_character(std::string &target, char c);
} // namespace stringutil
