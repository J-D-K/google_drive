#pragma once
#include "Storage.hpp"
#include <filesystem>

class Remote : public Storage
{
    public:
        /// @brief Default Remote constructor.
        Remote(void) = default;

        // Everything here is inherited from the base storage class, still virtual, and needs to be defined in the
        // derived classes.
        virtual void change_directory(std::string_view name) = 0;
        virtual bool create_directory(std::string_view name) = 0;
        virtual bool delete_directory(std::string_view name) = 0;
        virtual bool delete_file(std::string_view name) = 0;

        /// @brief Virtual function for uploading a file from the local file system.
        /// @param path Path of the file to upload.
        /// @return True on success. False on failure.
        virtual bool upload_file(const std::filesystem::path &path) = 0;

        /// @brief Virtual function for downloading a file from the remote storage.
        /// @param name Name/ID of the file to download.
        /// @param path Path to write the downloaded file to.
        /// @return True on success. False on failure.
        virtual bool download_file(std::string_view name, const std::filesystem::path &path) = 0;
};