#include "GoogleDrive.hpp"
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

    // Test drive instance.
    GoogleDrive drive{"./client_secret.json"};
    if (!drive.is_initialized())
    {
        std::cout << "Error initializing drive!" << std::endl;
        return -2;
    }

    // Get the drive listing. The parent is set to the root directory by default upon construction.
    drive.request_listing({}, true);

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

    drive.set_parent(jksvId);

    drive.upload_file("Rayman Forever.exe", "./setup_rayman_forever_1.21_(28045).exe");

    curl::exit();
    return 0;
}
