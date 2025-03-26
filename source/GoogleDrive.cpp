#include "GoogleDrive.hpp"
#include "json.hpp"
#include "logger.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace
{
    /// @brief Size for buffers used for formatting URLs.
    constexpr size_t SIZE_URL_BUFFER = 0x800;

    /// @brief Header string for JSON formatted requests.
    constexpr std::string_view HEADER_CONTENT_TYPE_JSON = "Content-Type: application/json";
    /// @brief Header string for URL encoded requests.
    constexpr std::string_view HEADER_CONTENT_TYPE_URL_ENCODED = "Content-Type: application/x-www-form-urlencoded";
    /// @brief Authorization header string that is appended with the token after it's received.
    constexpr std::string_view HEADER_AUTHORIZATION_BEARER = "Authorization: Bearer ";

    /// @brief Format string for getting the initial login code.
    constexpr std::string_view URL_OAUTH2_DEVICE_CODE_FORMAT = "https://oauth2.googleapis.com/device/code";
    /// @brief This is the OAUTH2 token URL.
    constexpr std::string_view URL_OAUTH2_TOKEN_URL = "https://oauth2.googleapis.com/token";
    /// @brief This endpoint is used once when first initializing Drive to get the ID of the root directory.
    /** @note Version 3 of Google's API doesn't support that and expects you to always use the alias root
     * for the root directory, which doesn't work with how I want this written. */
    constexpr std::string_view URL_DRIVE_ABOUT_API = "https://www.googleapis.com/drive/v2/about";
    /// @brief This is the main drive api endpoint used for pretty much everything?
    constexpr std::string_view URL_DRIVE_FILE_API = "https://www.googleapis.com/drive/v3/files";
    /// @brief API URL for starting file uploads.
    constexpr std::string_view URL_DRIVE_UPLOAD_API = "https://www.googleapis.com/upload/drive/v3/files";

    /// @brief Drive.file scope.
    constexpr std::string_view PARAM_DRIVE_FILE_SCOPE = "https://www.googleapis.com/auth/drive.file";
    /// @brief Grant type for the drive login polling loop.
    constexpr std::string_view PARAM_POLL_GRANT_TYPE = "urn:ietf:params:oauth:grant-type:device_code";
    /// @brief These are the base query parameters for getting drive listings.
    constexpr std::string_view PARAM_DEFAULT_LIST_QUERY =
        "fields=nextPageToken,files(name,id,size,parents,mimeType)&orderBy=name_natural&pageSize=256&q=trashed=false";

    // These are various keys I use repeatedly.
    constexpr std::string_view JSON_KEY_ACCESS_TOKEN = "access_token";
    /// @brief JSON key for the installed object in the client_secret.json
    constexpr std::string_view JSON_KEY_INSTALLED = "installed";
    /// @brief Client ID key.
    constexpr std::string_view JSON_KEY_CLIENT_ID = "client_id";
    /// @brief Client secret key.
    constexpr std::string_view JSON_KEY_CLIENT_SECRET = "client_secret";
    /// @brief This is the device code key for logging in.
    constexpr std::string_view JSON_KEY_DEVICE_CODE = "device_code";
    /// @brief JSON key for errors.
    constexpr std::string_view JSON_KEY_ERROR = "error";
    /// @brief This is the grant type key used for various requests.
    constexpr std::string_view JSON_KEY_GRANT_TYPE = "grant_type";
    /// @brief This is the key for ID.
    constexpr std::string_view JSON_KEY_ID = "id";
    /// @brief JSON key for the mime type.
    constexpr std::string_view JSON_KEY_MIME_TYPE = "mimeType";
    /// @brief JSON key for names.
    constexpr std::string_view JSON_KEY_NAME = "name";
    /// @brief Key for the nextPageToken for reading listings.
    constexpr std::string_view JSON_KEY_NEXT_PAGE_TOKEN = "nextPageToken";
    /// @brief Key for the parents array that doesn't need to be an array.
    constexpr std::string_view JSON_KEY_PARENTS = "parents";
    /// @brief Refresh token key.
    constexpr std::string_view JSON_KEY_REFRESH_TOKEN = "refresh_token";
    /// @brief Key for the time remaining for the token.
    constexpr std::string_view JSON_KEY_EXPIRES_IN = "expires_in";

    /// @brief This is the mimetype string for folders.
    constexpr std::string_view MIME_TYPE_DIRECTORY = "application/vnd.google-apps.folder";
} // namespace

GoogleDrive::GoogleDrive(std::string_view configFile) : m_curl(curl::new_handle())
{
    json::Object clientJson = json::new_object(json_object_from_file, configFile.data());
    if (!clientJson)
    {
        logger::log("Error reading Google Drive configuration!");
        return;
    }

    // Everything is inside this for some reason?
    json_object *installed = json_object_object_get(clientJson.get(), JSON_KEY_INSTALLED.data());
    if (!installed)
    {
        logger::log("Drive configuration is invalid!");
        return;
    }

    json_object *clientId = json_object_object_get(installed, JSON_KEY_CLIENT_ID.data());
    json_object *clientSecret = json_object_object_get(installed, JSON_KEY_CLIENT_SECRET.data());
    if (!clientId || !clientSecret)
    {
        logger::log("Drive configuration is corrupted or invalid.");
        return;
    }
    m_clientId = json_object_get_string(clientId);
    m_clientSecret = json_object_get_string(clientSecret);

    // Check if the refresh_token is appended.
    json_object *refreshToken = json_object_object_get(installed, JSON_KEY_REFRESH_TOKEN.data());
    if (refreshToken)
    {
        m_refreshToken = json_object_get_string(refreshToken);
        // Try to refresh it right away. If it fails, bail.
        if (!GoogleDrive::refresh_token())
        {
            return;
        }
    }
    else if (!refreshToken && GoogleDrive::sign_in()) // If the refresh token isn't in the config and sign_in succeeds.
    {
        // Append the refresh token to the config.
        json_object *refreshToken = json_object_new_string(m_refreshToken.c_str());
        json_object_object_add(installed, JSON_KEY_REFRESH_TOKEN.data(), refreshToken);

        // Write the changes immediately to avoid writing it to JKSV's config file.
        std::ofstream driveConfig(configFile.data());
        if (!driveConfig.is_open())
        {
            return;
        }
        // Write
        driveConfig << json_object_get_string(clientJson.get());

        logger::log("Refresh token written.");
    }
    else // Should always bail in this case.
    {
        return;
    }

    // The root ID is needed to make sure this all operates as it should.
    if (!GoogleDrive::get_set_root_id() || !GoogleDrive::request_listing())
    {
        return;
    }

    m_isInitialized = true;
}

void GoogleDrive::change_directory(std::string_view name)
{
    // This is the iterator.
    Storage::ItemIterator targetDir;
    if (name == ".." && (targetDir = GoogleDrive::find_directory_by_id(m_parent)) != m_list.end())
    {
        // Set the parent to the parent of our current parent.
        m_parent = targetDir->get_parent_id();
    }
    else if ((targetDir = Storage::find_directory(name)) != m_list.end())
    {
        m_parent = targetDir->get_id();
    }
    else
    {
        std::cout << "Drive error changing directory: Unable to locate target directory." << std::endl;
        return;
    }
}

bool GoogleDrive::create_directory(std::string_view name)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Headers.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader.c_str());
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // Json
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *dirName = json_object_new_string(name.data());
    json_object *mimeType = json_object_new_string(MIME_TYPE_DIRECTORY.data());
    json::add_object(postJson, JSON_KEY_NAME.data(), dirName);
    json::add_object(postJson, JSON_KEY_MIME_TYPE.data(), mimeType);
    // Add the parent array if the parent string isn't empty.
    if (!m_parent.empty())
    {
        json_object *parentArray = json_object_new_array();
        json_object *parentString = json_object_new_string(m_parent.c_str());
        json_object_array_add(parentArray, parentString);
        json::add_object(postJson, JSON_KEY_PARENTS.data(), parentArray);
    }

    // Response string.
    std::string response;
    // Curl post.
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_DRIVE_FILE_API.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    // This is all I really care about.
    json_object *id = json::get_object(responseParser, JSON_KEY_ID.data());
    if (!id)
    {
        return false;
    }

    // Emplace the new directory. Requesting a listing is a waste of time.
    m_list.emplace_back(name, json_object_get_string(id), m_parent, true);

    return true;
}

bool GoogleDrive::delete_directory(std::string_view name)
{
    // For now, this will just call delete_file. To do: Make recursive deleting if needed?
    return GoogleDrive::delete_file(name);
}

bool GoogleDrive::delete_file(std::string_view name)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Header
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // URL.
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s", URL_DRIVE_FILE_API.data(), name.data());

    // Response string. For this request, it's only to check for errors.
    std::string response;
    // Curl. This one is different.
    curl_easy_reset(m_curl.get());
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // This request is weird. There is no response from the server if everything worked. This is only checked to see
    // if we got an error response.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    return true;
}

void GoogleDrive::list_contents(void) const
{
    for (const Item &item : m_list)
    {
        if (item.get_parent_id() != m_parent)
        {
            continue;
        }

        // Print information.
        std::cout << item.get_name() << ":" << std::endl;
        std::cout << "\tID: " << item.get_id() << std::endl;
        std::cout << "\tParent: " << item.get_parent_id() << std::endl;
        std::cout << "\tDirectory: " << (item.is_directory() ? "true" : "false") << std::endl;
    }
}

bool GoogleDrive::upload_file(const std::filesystem::path &path)
{
    // Make sure the file can even be read before trying to continue.
    std::ifstream target(path, target.binary);
    if (!target.is_open())
    {
        return false;
    }

    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Headers.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // URL.
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s?uploadType=resumable", URL_DRIVE_UPLOAD_API.data());

    // Post JSON.
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *driveName = json_object_new_string(path.filename().c_str());
    json::add_object(postJson, JSON_KEY_NAME.data(), driveName);
    if (!m_parent.empty())
    {
        json_object *parents = json_object_new_array();
        json_object *parent = json_object_new_string(m_parent.c_str());
        json_object_array_add(parents, parent);
        json::add_object(postJson, JSON_KEY_PARENTS.data(), parents);
    }

    // Header array.
    curl::HeaderArray headerArray;
    // Curl
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_HEADERFUNCTION, curl::write_headers_array);
    curl::set_option(m_curl, CURLOPT_HEADERDATA, &headerArray);
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // Extract upload location from headers.
    std::string location;
    if (!curl::get_header_value(headerArray, "location", location))
    {
        logger::log("Error extracting location from upload request headers.");
        return false;
    }

    // Response string.
    std::string response;
    // This is the actual upload. IIRC, this doesn't need the token to work.
    curl::prepare_upload(m_curl);
    curl::set_option(m_curl, CURLOPT_URL, location.c_str());
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &target);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    // Try to grab these.
    json_object *id = json::get_object(responseParser, JSON_KEY_ID.data());
    json_object *filename = json::get_object(responseParser, JSON_KEY_NAME.data());
    json_object *mimeType = json::get_object(responseParser, JSON_KEY_MIME_TYPE.data());
    // All of them are needed to continue.
    if (!id || !filename || !mimeType)
    {
        return false;
    }

    // Emplace it.
    m_list.emplace_back(json_object_get_string(filename),
                        json_object_get_string(id),
                        m_parent,
                        std::strcmp(MIME_TYPE_DIRECTORY.data(), json_object_get_string(mimeType)) == 0);

    // Assume it worked and everything is fine!
    return true;
}

bool GoogleDrive::download_file(std::string_view name, const std::filesystem::path &path)
{
    // Header
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // URL
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s/download", URL_DRIVE_FILE_API.data(), name.data());

    // Response string.
    std::string response;
    // Curl
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    return true;
}

bool GoogleDrive::sign_in(void)
{
    // Header list
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // Post JSON
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *clientId = json_object_new_string(m_clientId.data());
    json_object *scope = json_object_new_string(PARAM_DRIVE_FILE_SCOPE.data());
    json::add_object(postJson, JSON_KEY_CLIENT_ID.data(), clientId);
    json::add_object(postJson, "scope", scope);

    // Response string.
    std::string response;
    // Curl get.
    curl::prepare_get(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_OAUTH2_DEVICE_CODE_FORMAT.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        // curl::perform should log any errors.
        return false;
    }

    // Response parser and error check.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    // This is what we need from the response.
    json_object *deviceCode = json::get_object(responseParser, JSON_KEY_DEVICE_CODE.data());
    json_object *userCode = json::get_object(responseParser, "user_code");
    json_object *verificationUrl = json::get_object(responseParser, "verification_url");
    json_object *expiresIn = json::get_object(responseParser, JSON_KEY_EXPIRES_IN.data());
    json_object *interval = json::get_object(responseParser, "interval");
    // If any of this aren't found, bail.
    if (!deviceCode || !userCode || !verificationUrl || !expiresIn || !interval)
    {
        logger::log("Error: Drive sign in response is abnormal.");
        return false;
    }

    // This is the time the login counter dies at.
    std::time_t expirationTime = std::time(NULL) + json_object_get_uint64(expiresIn);

    // Make sure the user is aware they need to hurry their lazy ass up and login for me.
    std::cout << "The countdown has begun. You have " << json_object_get_uint64(expiresIn)
              << " seconds to log in. Are you man enough to do it in time? Get moving, boy... "
              << json_object_get_string(verificationUrl) << " & " << json_object_get_string(userCode) << "."
              << std::endl;

    // This is the json for polling Google's server. I don't think creating a new one every loop is a smart idea.
    json::Object pollingJson = json::new_object(json_object_new_object);
    // These two are the only two new things I need.
    json_object *clientSecret = json_object_new_string(m_clientSecret.c_str());
    json_object *grantType = json_object_new_string(PARAM_POLL_GRANT_TYPE.data());
    // Just gonna reuse the rest.
    json::add_object(pollingJson, JSON_KEY_CLIENT_ID.data(), clientId);
    json::add_object(pollingJson, JSON_KEY_CLIENT_SECRET.data(), clientSecret);
    json::add_object(pollingJson, JSON_KEY_DEVICE_CODE.data(), deviceCode);
    json::add_object(pollingJson, JSON_KEY_GRANT_TYPE.data(), grantType);

    // Response string.
    std::string pollingResponse;
    // Setup the curl request.
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_OAUTH2_TOKEN_URL.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &pollingResponse);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(pollingJson.get()));

    // Get the polling interval.
    int pollingInterval = json_object_get_int64(interval);
    // Loop until time runs out.
    while (std::time(NULL) < expirationTime && curl::perform(m_curl))
    {
        // Parse the response.
        json::Object pollingParser = json::new_object(json_tokener_parse, pollingResponse.c_str());

        // Check for the error key. This is how Google responds until the login is confirmed.
        json_object *error = json::get_object(pollingParser, JSON_KEY_ERROR.data());
        // If there isn't an error key, break the loop and continue the process.
        if (!error)
        {
            break;
        }

        // Clear the response string.
        pollingResponse.clear();

        std::cout << "Still waiting..." << std::endl;

        // Interval sleep.
        std::this_thread::sleep_for(std::chrono::seconds(pollingInterval));
    }

    // To do: This might not be safe. I'm going to assume if the response is empty, the user was too fat and slow to log in.
    if (pollingResponse.empty())
    {
        return false;
    }

    // To do: Maybe try to figure out how to reuse the parser from the loop? The one from the loop is no more at this point.
    json::Object loginParser = json::new_object(json_tokener_parse, pollingResponse.c_str());
    // This is just in case access was denied.
    if (GoogleDrive::error_occurred(loginParser))
    {
        return false;
    }
    json_object *accessToken = json::get_object(loginParser, JSON_KEY_ACCESS_TOKEN.data());
    expiresIn = json::get_object(loginParser, JSON_KEY_EXPIRES_IN.data());
    json_object *refreshToken = json::get_object(loginParser, JSON_KEY_REFRESH_TOKEN.data());
    // Don't need the rest.

    // Save these.
    m_token = json_object_get_string(accessToken);
    m_refreshToken = json_object_get_string(refreshToken);
    // Calc this
    m_tokenExpiration = std::time(NULL) + json_object_get_int64(expiresIn);

    // Make header string
    m_authHeader = std::string(HEADER_AUTHORIZATION_BEARER) + m_token;

    // Should be good to go.
    return true;
}

bool GoogleDrive::get_set_root_id(void)
{
    // This should take place so early that this shouldn't be an issue, but you never know.
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    logger::log("get_set_root_id");

    // Headers
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);
    logger::log("headers");

    // URL
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s?fields=rootFolderId", URL_DRIVE_ABOUT_API.data());
    logger::log(urlBuffer);

    // Response string.
    std::string response;
    // Curl
    curl::prepare_get(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    logger::log("curl");

    if (!curl::perform(m_curl))
    {
        return false;
    }
    logger::log("curl::perform");

    // Response parsing.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }
    logger::log("response");

    // Get the root ID.
    json_object *rootId = json::get_object(responseParser, "rootFolderId");
    if (!rootId)
    {
        logger::log("Error getting root directory ID from Google Drive!");
        return false;
    }
    logger::log("rootId");

    // Save it and also set the current parent to it.
    m_root = json_object_get_string(rootId);
    m_parent = m_root;

    logger::log("Root obtained: %s", m_root.c_str());

    return true;
}

bool GoogleDrive::token_is_valid(void) const
{
    // Gonna give this a 10 second grace window.
    return std::time(NULL) < (m_tokenExpiration - 10);
}

bool GoogleDrive::refresh_token(void)
{
    // Add JSON content type headers.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // JSON to post.
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *clientId = json_object_new_string(m_clientId.c_str());
    json_object *clientSecret = json_object_new_string(m_clientSecret.c_str());
    json_object *grantType = json_object_new_string(JSON_KEY_REFRESH_TOKEN.data()); // This JSON key works here too.
    json_object *refreshToken = json_object_new_string(m_refreshToken.c_str());
    json::add_object(postJson, JSON_KEY_CLIENT_ID.data(), clientId);
    json::add_object(postJson, JSON_KEY_CLIENT_SECRET.data(), clientSecret);
    json::add_object(postJson, JSON_KEY_GRANT_TYPE.data(), grantType);
    json::add_object(postJson, JSON_KEY_REFRESH_TOKEN.data(), refreshToken);

    // Response string.
    std::string response;
    // Post request.
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_OAUTH2_TOKEN_URL.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    // This is all I care about.
    json_object *accessToken = json::get_object(responseParser, JSON_KEY_ACCESS_TOKEN.data());
    json_object *expiresIn = json::get_object(responseParser, JSON_KEY_EXPIRES_IN.data());

    // Save them
    m_token = json_object_get_string(accessToken);
    m_tokenExpiration = std::time(NULL) + json_object_get_int64(expiresIn);

    // Re-do header string
    m_authHeader = std::string(HEADER_AUTHORIZATION_BEARER) + m_token;

    return true;
}

bool GoogleDrive::request_listing(void)
{
    // Block against even trying if either of these fail.
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Initial URL.
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s?%s", URL_DRIVE_FILE_API.data(), PARAM_DEFAULT_LIST_QUERY.data());

    // Header
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // Response string.
    std::string response;
    // Curl request. The URL will get updated in the loop processing the listing.
    curl::prepare_get(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    // This is used as our loop condition.
    json_object *nextPageToken = nullptr;
    do
    {
        // Clear the string first.
        response.clear();

        if (!curl::perform(m_curl))
        {
            // Bail. To do: Handle this better? Maybe?
            return false;
        }

        // Response token/parser
        json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
        if (!responseParser || GoogleDrive::error_occurred(responseParser) ||
            !GoogleDrive::process_listing(responseParser))
        {
            // Just bail. An error occurred.
            return false;
        }

        // Try to get the nextPageToken.
        nextPageToken = json::get_object(responseParser, JSON_KEY_NEXT_PAGE_TOKEN.data());
        if (!nextPageToken)
        {
            // Break the loop, making the condition kind of redundant...
            break;
        }

        // Update the URL.
        std::snprintf(urlBuffer,
                      SIZE_URL_BUFFER,
                      "%s?%s&pageToken=%s",
                      URL_DRIVE_FILE_API.data(),
                      PARAM_DEFAULT_LIST_QUERY.data(),
                      json_object_get_string(nextPageToken));
        curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    } while (nextPageToken);

    return true;
}

bool GoogleDrive::process_listing(json::Object &json)
{
    json_object *files = json::get_object(json, "files");
    if (!files)
    {
        return false;
    }

    // Loop through the array and read off everything.
    size_t arrayLength = json_object_array_length(files);
    for (size_t i = 0; i < arrayLength; i++)
    {
        // Grab the current object at i
        json_object *currentFile = json_object_array_get_idx(files, i);
        if (!currentFile)
        {
            return false;
        }

        json_object *mimeType = json_object_object_get(currentFile, JSON_KEY_MIME_TYPE.data());
        json_object *parents = json_object_object_get(currentFile, JSON_KEY_PARENTS.data());
        json_object *id = json_object_object_get(currentFile, JSON_KEY_ID.data());
        json_object *name = json_object_object_get(currentFile, JSON_KEY_NAME.data());
        if (!mimeType || !parents || !id || !name)
        {
            // Just bail. Something is really wrong.
            logger::log("Error processing Google Drive list: Malformed or corrupted response.");
            return false;
        }

        // I don't understand why this needs to be an array if files can only have one parent.
        json_object *parent = json_object_array_get_idx(parents, 0);
        if (!parent)
        {
            logger::log("Error reading parents array.");
            return false;
        }

        // Emplace
        m_list.emplace_back(json_object_get_string(name),
                            json_object_get_string(id),
                            json_object_get_string(parent),
                            std::strcmp(MIME_TYPE_DIRECTORY.data(), json_object_get_string(mimeType)) == 0);
    }

    return true;
}

Storage::ItemIterator GoogleDrive::find_directory_by_id(std::string_view id)
{
    return std::find_if(m_list.begin(), m_list.end(), [id](const Item &item) {
        return item.is_directory() && item.get_id() == id;
    });
}

bool GoogleDrive::error_occurred(json::Object &json)
{
    json_object *error = json::get_object(json, JSON_KEY_ERROR.data());
    if (error)
    {
        // Grab the description too.
        json_object *errorDescription = json::get_object(json, "error_description");

        // Log them. Google is inconsistent with the structure of errors and I don't feel like handling every
        // single different format of them.
        logger::log("Google Drive Error: %s: %s.",
                    json_object_get_string(error),
                    json_object_get_string(errorDescription));

        return true;
    }
    return false;
}
