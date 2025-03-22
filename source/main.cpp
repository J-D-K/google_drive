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
    std::cin >> localRoot;
    Local local(localRoot);

    // Test drive instance.
    GoogleDrive drive{"./client_secret.json"};
    if (!drive.is_initialized())
    {
        std::cout << "Error initializing drive!" << std::endl;
        return -2;
    }

    if (!drive.directory_exists(DIR_JKSV_FOLDER.data()) && !drive.create_directory(DIR_JKSV_FOLDER.data()))
    {
        std::cout << "Error creating JKSV folder!" << std::endl;
        return -3;
    }

    // Grab the ID of the JKSV folder.
    std::string jksvId;
    if (!drive.get_directory_id(DIR_JKSV_FOLDER.data(), jksvId))
    {
        std::cout << "Error getting ID of JKSV folder!" << std::endl;
        return -4;
    }

    curl::exit();
    return 0;
}
