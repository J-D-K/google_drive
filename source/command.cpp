#include "command.hpp"
#include "CommandReader.hpp"
#include "GoogleDrive.hpp"
#include "Local.hpp"
#include "Storage.hpp"
#include <iostream>
#include <map>
#include <string>

namespace
{
    // Enum for map and switch case.
    enum COMMAND_IDS
    {
        ID_LIST,
        ID_CHDIR,
        ID_MKDIR,
        ID_DELETE
    };

    // Map of commands.
    std::map<std::string_view, int> COMMAND_MAP = {{"list", COMMAND_IDS::ID_LIST},
                                                   {"chdir", COMMAND_IDS::ID_CHDIR},
                                                   {"mkdir", COMMAND_IDS::ID_MKDIR},
                                                   {"delete", COMMAND_IDS::ID_DELETE}};

    // Error strings for commands.
    constexpr std::string_view ERROR_CHDIR = "Error executing command chdir: ";
    constexpr std::string_view ERROR_MKDIR = "Error executing command mkdir: ";
    constexpr std::string_view ERROR_DELETE = "Error executing command delete: ";
} // namespace

/// @brief Function for executing the command chdir.
/// @param storage Target storage system to change directories with.
/// @return True on success. False on failure.
static bool chdir(Storage &storage);

/// @brief Creates a new directory.
/// @param storage Target storage system.
/// @return True on success. False on failure.
static bool mkdir(Storage &storage);

/// @brief Deletes the target item.
/// @param storage Target storage system.
/// @return True on success. False on failure.
static bool deleteItem(Storage &storage);

bool execute_command(Storage &storage)
{
    // Start by grabbing the command string.
    std::string command;
    if (!CommandReader::get_next_parameter(command))
    {
        return false;
    }

    switch (COMMAND_MAP[command])
    {
        case ID_LIST:
        {
            storage.list_contents();
            return true;
        }
        break;

        case ID_CHDIR:
        {
            return chdir(storage);
        }
        break;

        case ID_MKDIR:
        {
            return mkdir(storage);
        }
        break;

        case ID_DELETE:
        {
            return deleteItem(storage);
        }
        break;
    }

    return true;
}

static bool chdir(Storage &storage)
{
    // Get directory name.
    std::string directory;
    if (!CommandReader::get_next_parameter(directory) || (!storage.directory_exists(directory) && directory != ".."))
    {
        std::cout << ERROR_CHDIR << "No directory passed or directory doesn't exist within current parent."
                  << std::endl;
        return false;
    }

    // Change directory.
    storage.change_directory(directory);

    // Assume everything worked.
    return true;
}

static bool mkdir(Storage &storage)
{
    std::string directory;
    if (!CommandReader::get_next_parameter(directory) || !storage.create_directory(directory))
    {
        std::cout << ERROR_MKDIR << "No directory passed or creating directory failed!" << std::endl;
        return false;
    }
    return true;
}

static bool deleteItem(Storage &storage)
{
    std::string type, target;
    if (!CommandReader::get_next_parameter(type) || !CommandReader::get_next_parameter(target))
    {
        std::cout << ERROR_DELETE << "Missing parameter" << std::endl;
        return false;
    }

    // Tell what type of item to delete.
    bool directory = false;
    if (type == "dir" || type == "folder" || type == "directory")
    {
        directory = true;
    }

    bool success = false;
    if (directory)
    {
        success = storage.delete_directory(target);
    }
    else
    {
        success = storage.delete_file(target);
    }

    return success;
}
