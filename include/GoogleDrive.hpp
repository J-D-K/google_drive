#pragma once
#include "Item.hpp"
#include "Remote.hpp"
#include "curl.hpp"
#include "json.hpp"
#include <ctime>
#include <optional>
#include <string>
#include <vector>

class GoogleDrive final : public Remote
{
    public:
        /// @brief Initializes a new instance of the GoogleDrive class.
        /// @param clientSecret Path to the client secret from Google's API.
        GoogleDrive(std::string_view configFile);

        /// @brief Changes the current parent directory/ID.
        /// @param name ID of the directory to change to.
        void change_directory(std::string_view name) override;

        /// @brief Creates a new directory on Google Drive.
        /// @param name Name of the directory to create.
        /// @return True on success. False on failure.
        bool create_directory(std::string_view name) override;

        /// @brief Deletes a directory from Google Drive. To do: Make this recursive, even if it is a pain.
        /// @param name ID of the directory to delete.
        /// @return True on success. False on failure.
        bool delete_directory(std::string_view name) override;

        /// @brief Deletes a file from Google Drive.
        /// @param name ID of the file to delete.
        /// @return True on success. False on failure.
        bool delete_file(std::string_view name) override;

        /// @brief Uploads a file to Google Drive under the currently set parent.
        /// @param path Path of the file to upload.
        /// @return True on success. False on failure.
        bool upload_file(const std::filesystem::path &path) override;

        /// @brief Downloads a file from Google Drive.
        /// @param name ID of the file to download.
        /// @param path Path of the file to write the downloaded data to.
        /// @return True on success. False on failure.
        bool download_file(std::string_view name, const std::filesystem::path &path) override;

    private:
        /// @brief String for storing client ID.
        std::string m_clientId;

        /// @brief String for storing the client secret string.
        std::string m_clientSecret;

        /// @brief String for storing the access token.
        std::string m_token;

        /// @brief String for storing the refresh token.
        std::string m_refreshToken;

        /// @brief String for storing the authentication header.
        std::string m_authHeader;

        /// @brief Curl handle.
        curl::Handle m_curl;

        /// @brief This stores the time the token expires at.
        std::time_t m_tokenExpiration;

        /// @brief Signs in to Google Drive using the information read from the client_secret.json file.
        /// @return True on success. False on failure.
        bool sign_in(void);

        /// @brief Uses V2 of Drive's API to get the root ID and set it.
        /// @return True on success. False on failure.
        bool get_set_root_id(void);

        /// @brief Returns whether or not the token is valid. Includes a small grace period just to be 100% safe.
        /// @return True if the token is still valid. False if it isn't.
        bool token_is_valid(void) const;

        /// @brief Refreshes the access token using the refreshToken.
        /// @return True on success. False on failure.
        bool refresh_token(void);

        /// @brief Requests the full listing of everything JKSV has created and uploaded to Drive.
        /// @return True on success. False on failure.
        bool request_listing(void);

        /// @brief Processes a listing response from Google.
        /// @param json json::Object containing the response.
        /// @return True on success. False on failure.
        bool process_listing(json::Object &json);

        /// @brief Performs a quick check on the json::Object passed to see if the error key is present.
        /// @param json json::Object to check.
        /// @return True if the key is found. False if it isn't.
        bool error_occurred(json::Object &json);
};