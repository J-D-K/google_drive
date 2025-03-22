#include "Local.hpp"
#include <algorithm>
#include <filesystem>

Local::Local(std::string_view root) : Storage(root)
{
    // Load the initial listing using the root passed as the parent.
    Local::load_parent_listing();
}

void Local::change_directory(std::string_view name)
{
    // If the passed name is .., go up a directory as long as it's not above the root.
    if (name == "..")
    {
        // Get the parent of the current parent.
        size_t parent = m_parent.find_last_of('/');

        // Set the parent to a substring of itself ending with the last slash in the path. Note: This needs a check
        // in the future.
        m_parent = m_parent.substr(0, parent - 1);
    }
    else
    {
        // Just append the parent to the end.
        m_parent += m_parent.back() == '/' ? name : std::string("/") + name.data();
    }
    // Reinit the listing for this directory.
    Local::load_parent_listing();
}

bool Local::create_directory(std::string_view name)
{
    // Full path.
    std::filesystem::path fullPath = std::filesystem::path(m_parent) / name;
    // This should be good enough.
    return std::filesystem::create_directory(fullPath);
}

bool Local::delete_directory(std::string_view name)
{
    // Path
    std::filesystem::path fullPath = std::filesystem::path(m_parent) / name;
    // This might need a better check for return some time?
    return std::filesystem::remove_all(fullPath) > 0;
}

bool Local::delete_file(std::string_view name)
{
    if (!Local::file_exists(name))
    {
        return false;
    }

    // Path
    std::filesystem::path fullPath = std::filesystem::path(m_parent) / name;

    // This already returns a bool.
    return std::filesystem::remove(fullPath);
}

void Local::load_parent_listing(void)
{
    // Clear the list vector.
    m_list.clear();

    // Load the listing for m_parent.
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(m_parent))
    {
        // This is used twice for this storage type and I think it looks stupid.
        std::string_view name = entry.path().filename().string();
        // Push back the name of the file.
        m_list.emplace_back(name, name, entry.path().parent_path().string(), entry.is_directory());
    }
}