#include "Storage.hpp"
#include <algorithm>
#include <iostream>

Storage::Storage(std::string_view root) : m_root(root), m_parent(root) {};

bool Storage::is_initialized(void) const
{
    return m_isInitialized;
}

void Storage::return_to_root(void)
{
    m_parent = m_root;
}

bool Storage::directory_exists(std::string_view name)
{
    return Storage::find_directory(name) != m_list.end();
}

bool Storage::file_exists(std::string_view name)
{
    return Storage::find_file(name) != m_list.end();
}

bool Storage::get_directory_id(std::string_view name, std::string &out)
{
    Storage::ItemIterator findDir = Storage::find_directory(name);
    if (findDir == m_list.end())
    {
        return false;
    }
    // Set out to the ID of the dir found.
    out = findDir->get_id();
    // Success!?
    return true;
}

bool Storage::get_directory_id(int index, std::string &out)
{
    if (index < 0 || index >= m_list.size())
    {
        return false;
    }
    out = m_list.at(index).get_id();
    return true;
}

bool Storage::get_file_id(std::string_view name, std::string &out)
{
    Storage::ItemIterator findFile = Storage::find_file(name);
    if (findFile == m_list.end())
    {
        return false;
    }
    out = findFile->get_id();
    return true;
}

bool Storage::get_file_id(int index, std::string &out)
{
    if (index < 0 || index >= m_list.size())
    {
        return false;
    }
    out = m_list.at(index).get_id();
    return true;
}

void Storage::list_contents(void) const
{
    for (const Item &item : m_list)
    {
        std::cout << item.get_name() << ":" << std::endl;
        std::cout << "\tID: " << item.get_id() << std::endl;
        std::cout << "\tParent: " << item.get_parent_id() << std::endl;
        std::cout << "\tDirectory: " << item.is_directory() << std::endl;
    }
}

Storage::ItemIterator Storage::find_directory(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return item.is_directory() && item.get_parent_id() == this->m_parent && name == item.get_name();
    });
}

Storage::ItemIterator Storage::find_file(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return !item.is_directory() && item.get_parent_id() == this->m_parent && item.get_name() == name;
    });
}