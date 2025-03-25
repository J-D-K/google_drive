#include "CommandReader.hpp"
#include <iostream>

bool CommandReader::read_line(void)
{
    // Get instance.
    CommandReader &reader = CommandReader::get_instance();

    // Read the string passed.
    std::getline(std::cin, reader.m_line);

    // Bail if the line is empty or just a new line.
    if (reader.m_line.empty() || reader.m_line == "\n")
    {
        return false;
    }

    // Reset offset.
    reader.m_offset = 0;

    // Success.
    return true;
}

bool CommandReader::get_next_parameter(std::string &out)
{
    // Get instance of command reader.
    CommandReader &reader = CommandReader::get_instance();

    // Just bail if this is true.
    if (reader.m_offset == reader.m_line.npos)
    {
        return false;
    }

    // Next space or line end.
    size_t nextSpace = reader.m_line.find_first_of(" ", reader.m_offset);

    // Assign the output string.
    out = reader.m_line.substr(reader.m_offset, nextSpace - reader.m_offset);

    // Update the line offset.
    reader.m_offset = nextSpace == reader.m_line.npos ? reader.m_line.npos : ++nextSpace;

    return true;
}