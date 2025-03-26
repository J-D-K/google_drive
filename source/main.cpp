#include "CommandReader.hpp"
#include "GoogleDrive.hpp"
#include "Local.hpp"
#include "command.hpp"
#include "curl.hpp"
#include "logger.hpp"
#include <iostream>
#include <map>
#include <optional>
#include <string>

namespace
{
    /// @brief This is the name of the JKSV folder on Google Drive.
    constexpr std::string_view DIR_JKSV_FOLDER = "JKSV";
}; // namespace

/// @brief Inline declaration of function to select the target storage. This keeps the main loop looking cleaner.
/// @param target String containing the target string.
/// @param local Reference to the Local storage instance.
/// @param drive Reference to the Google Drive instance.
/// @return Reference to Storage on success. null on failure.
static inline std::optional<std::reference_wrapper<Storage>> select_storage(std::string_view target,
                                                                            Local &local,
                                                                            GoogleDrive &drive);

int main(int argc, const char *argv[])
{
    if (!curl::initialize())
    {
        return -1;
    }

    // Init logger.
    logger::initialize();

    // Init local.
    std::string localRoot;
    std::cout << "Local root: ";
    std::getline(std::cin, localRoot);
    Local local{localRoot};

    // Test drive instance.
    GoogleDrive drive{"./client_secret.json"};
    if (!drive.is_initialized())
    {
        std::cout << "Error initializing drive!" << std::endl;
        return -2;
    }

    // This is the string used to get the target storage.
    std::string storage;

    while (CommandReader::read_line())
    {
        if (!CommandReader::get_next_parameter(storage))
        {
            break;
        }

        // Get the target.
        auto target = select_storage(storage, local, drive);
        if (!target.has_value())
        {
            continue;
        }

        // Execute the command.
        execute_command(target.value().get());
    }

    curl::exit();
    return 0;
}

static inline std::optional<std::reference_wrapper<Storage>> select_storage(std::string_view target,
                                                                            Local &local,
                                                                            GoogleDrive &drive)
{
    if (target == "local")
    {
        return local;
    }
    else if (target == "drive")
    {
        return drive;
    }
    // Nothing good ever happens.
    std::cout << "Invalid storage medium \"" << target << "\" passed!" << std::endl;
    return std::nullopt;
}