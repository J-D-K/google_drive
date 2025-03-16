#include "Item.hpp"

Item::Item(std::string_view name, std::string_view id, std::string_view parent, bool isDirectory)
    : m_name(name), m_id(id), m_parent(parent), m_isDirectory(isDirectory) {};

std::string_view Item::get_name(void) const
{
    return m_name;
}

std::string_view Item::get_id(void) const
{
    return m_id;
}

std::string_view Item::get_parent_id(void) const
{
    return m_parent;
}

bool Item::is_directory(void) const
{
    return m_isDirectory;
}

void Item::set_name(std::string_view name)
{
    m_name = name;
}

void Item::set_id(std::string_view id)
{
    m_id = id;
}

void Item::set_parent_id(std::string_view parent)
{
    m_parent = parent;
}

void Item::set_is_directory(bool isDirectory)
{
    m_isDirectory = isDirectory;
}
