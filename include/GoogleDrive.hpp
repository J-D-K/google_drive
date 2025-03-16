#pragma once
#include "Item.hpp"
#include "curl.hpp"
#include "json.hpp"
#include <ctime>
#include <optional>
#include <string>
#include <vector>

class GoogleDrive
{
    public:
        /// @brief Definition for remote list.
        using RemoteList = std::vector<Item>;

        /// @brief Definition to make typing some stuff easier.
        using ListIterator = std::vector<Item>::iterator;

        /// @brief Initializes a new instance of the GoogleDrive class.
        /// @param clientSecret Path to the client secret from Google's API.
        GoogleDrive(std::string_view configFile);

        /// @brief Returns if Drive initialized successfully.
        /// @return True if it did. False if not.
        bool is_initialized(void) const;

        /// @brief Attempts to sign in to Google with the data read from the config file.
        /// @return True on success. False on failure.
        bool sign_in(void);

        /// @brief Returns whether or not the token is still valid based upon the time it was calculated to expire.
        /// @return True if the token is still valid. False if it isn't.
        bool token_is_valid(void) const;

        /// @brief Attempts to refresh the token with the refreshToken.
        /// @return True on success. False on failure.
        bool refresh_token(void);

        /// @brief Requests a listing.
        /// @param parameters Filters/options appended to the API call.
        /// @param clear Whether or the list should be reset first, or just appended.
        /// @return True on success. False on failure.
        bool request_listing(std::string_view parameters, bool clear);

        /// @brief Processes the json passed and appends it to m_remoteList.
        /// @param json JSON object to process.
        /// @return True on success. False if an error was detected or encountered.
        bool process_listing(json::Object &json);

        /// @brief Searches for a directory named 'name' with the current parent set.
        /// @param name Name of the directory to search for.
        /// @param idOut String to write the ID of the directory found to.
        /// @return True if one is found. False if not.
        bool get_directory_id(std::string_view name, std::string &idOut);

        /// @brief Searches for a file named 'name' with the current parent set.
        /// @param name Name of the file to search for.
        /// @param idOut String to the ID of the file to.
        /// @return True if one is found. False if not.
        bool get_file_id(std::string_view name, std::string &idOut);

        /// @brief Gets the ID of the current parent.
        /// @param idOut String to write the parent id to.
        void get_parent(std::string &idOut);

        /// @brief Sets the parent directory to use for creating and uploading files.
        /// @param parent ID of the parent folder to use.
        void set_parent(std::string_view parent);

        /// @brief Returns the parent to the root directory (clears it.)
        void return_to_root(void);

        /// @brief Returns
        /// @param name
        /// @return
        bool directory_exists(std::string_view name);

        /// @brief Creates a new directory on Google Drive.
        /// @param name Name of the new directory.
        /// @return True on success. False on failure.
        bool create_directory(std::string_view name);

        /// @brief Uploads a file to Google Drive under the currently set parent directory.
        /// @param name Name of the file on Google Drive.
        /// @param filePath Local path of the file to upload.
        /// @return True on success. False on failure.
        bool upload_file(std::string_view name, std::string_view filePath);

        /// @brief Downloads a file from Google Drive.
        /// @param id ID of the file to download.
        /// @param filePath File path to download the file to.
        /// @return True on success. False on failure.
        bool download_file(std::string_view id, std::string_view filePath);

        /// @brief Deletes the item with itemId from Google Drive.
        /// @param itemId ID of the item to delete.
        /// @return True on success. False on failure.
        bool delete_item(std::string_view itemId);

        /// @brief Writes the same listing as print to log.
        void debug_log_list(void);

        /// @brief Debug function that prints the current listing to the console.
        void debug_print_list(void);

        /// @brief Get file attributes.
        /// @param id ID of the file to get attributes for.
        /// @return True on success. False on failure.
        bool debug_get_attributes(std::string_view id);

        /// @brief Returns a reference to the internal remote list. For debugging and fixing mistakes that make the Google Drive browser UI crash hard.
        /// @return Reference to internal GoogleDrive::RemoteList.
        GoogleDrive::RemoteList &debug_get_remote_list(void);

        /// @brief Checks the json::Object for Google Drive errors and logs them if needed.
        /// @param json JSON to check.
        /// @return True if an error occurred. False if one didn't.
        bool error_occurred(json::Object &json);

    private:
        /// @brief Whether or not initialization was successful.
        bool m_isInitialized = false;

        /// @brief Client ID.
        std::string m_clientId;

        /// @brief Client Secret.
        std::string m_clientSecret;

        /// @brief Google authorization token.
        std::string m_token;

        /// @brief Google refresh token.
        std::string m_refreshToken;

        /// @brief The authorization header string.
        std::string m_authHeader;

        /// @brief ID of the current parent. This is only used for creating, uploading and searching.
        std::string m_parent;

        /// @brief Saves the ID of the root directory.
        std::string m_root;

        /// @brief Curl handle.
        curl::Handle m_curl;

        /// @brief This holds the time the token expires.
        std::time_t m_expirationTime;

        /// @brief File listing.
        RemoteList m_remoteList;

        /// @brief Performs a quick GET request using version 2 of Drive's API to get the ID of the root directory.
        bool get_root_id(void);

        /// @brief Searches the list for a directory matching name with the current parent set.
        /// @param name Name of the directory to search for.
        /// @return Iterator to the directory found. m_remoteList.end() on failure.
        ListIterator find_directory(std::string_view name);

        /// @brief Searches the list for a file matching name with the current parent set.
        /// @param name Name of the file to search for.
        /// @return Iterator to the file found. m_remoteList.end() on failure.
        ListIterator find_file(std::string_view name);
};
