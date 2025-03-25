#include "CommandReader.hpp"
#include "GoogleDrive.hpp"
#include "Local.hpp"
#include "curl.hpp"
#include "logger.hpp"
#include <iostream>
#include <map>
#include <string>

namespace
{
    /// @brief This is the name of the JKSV folder on Google Drive.
    constexpr std::string_view DIR_JKSV_FOLDER = "JKSV";
}; // namespace

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

    // This is the pointer used to switch between client/remote.
    Storage *target = nullptr;

    while (CommandReader::read_line())
    {
        std::string targetString;
        if (!CommandReader::get_next_parameter(targetString))
        {
            break;
        }

        // Set the target storage system.
        if (targetString == "local")
        {
            target = &local;
        }
        else if (targetString == "drive")
        {
            target = &drive;
        }
        else if (targetString == "exit")
        {
            break;
        }
        else
        {
            std::cout << "Invalid storage system \"" << targetString << "\" specified." << std::endl;
            // Do not pass go. Do not collect $200.
            continue;
        }
    }

    curl::exit();
    return 0;
}
