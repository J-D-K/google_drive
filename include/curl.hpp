#pragma once
#include "logger.hpp"
#include <curl/curl.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace curl
{
    /// @brief User agent string.
    static constexpr std::string_view USER_AGENT_STRING = "JKSV";

    /// @brief Definition for a self cleaning CURL handle.
    using Handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

    /// @brief Definition for a self cleaning CURL slist/header list.
    using HeaderList = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;

    /// @brief Definition for a vector of header strings from CURL.
    using HeaderArray = std::vector<std::string>;

    /// @brief Initializes libCURL
    /// @return True on success. False on failure.
    static inline bool initialize(void)
    {
        return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    }

    /// @brief Exits libCURL.
    static inline void exit(void)
    {
        curl_global_cleanup();
    }

    /// @brief Inline function that returns a unique_ptr wrapped, self cleaning CURL handle.
    /// @return Self cleaning CURL handle.
    static inline curl::Handle new_handle(void)
    {
        return curl::Handle(curl_easy_init(), curl_easy_cleanup);
    }

    /// @brief Returns a unique_ptr for creating a curl_slist that will free itself.
    /// @return nullptr'd unique_ptr to create an slist with.
    /// @note curl_slists created this way must be built using <unique_ptr>.reset(curl_slist_append(<unique_ptr>.get(), PARAM))
    static inline curl::HeaderList new_header_list(void)
    {
        return curl::HeaderList(nullptr, curl_slist_free_all);
    }

    /// @brief Inline function to reset a curl::Handle.
    /// @param handle Handle to reset.
    static inline void reset(curl::Handle &handle)
    {
        curl_easy_reset(handle.get());
    }

    static inline bool perform(curl::Handle &handle)
    {
        CURLcode error = curl_easy_perform(handle.get());
        if (error != CURLE_OK)
        {
            logger::log("Error performing CURL: %i.", error);
            return false;
        }
        return true;
    }

    /// @brief Inline templated function to wrap curl_easy_setopt and make using curl::Handle slightly easier.
    /// @tparam Option Templated type of the option. This is a headache so let the compiler figure it out.
    /// @tparam Value Templated type of the value to set the option too. See above.
    /// @param handle curl::Handle the option is being set for.
    /// @param option CURLOPT to set.
    /// @param value Value to set the CURLOPT to.
    /// @return CURLcode returned from curl_easy_setopt.
    template <typename Option, typename Value>
    static inline CURLcode set_option(curl::Handle &handle, Option option, Value value)
    {
        return curl_easy_setopt(handle.get(), option, value);
    }

    /// @brief Inline function to append a header to a curl_slist.
    /// @param headerList Header list to append header to.
    /// @param header Header to append.
    static inline void append_header(curl::HeaderList &headerList, std::string_view header)
    {
        // This might be the only real way to pull this off. I tried a couple others, but this is the only way that
        // worked consistently.

        // Release the list.
        curl_slist *list = headerList.release();

        // Append the new header.
        list = curl_slist_append(list, header.data());

        // Make the header list point to the new head of the list.
        headerList.reset(list);
    }

    /// @brief Curl callback function that reads data from a file.
    /// @param buffer Incoming buffer from curl to read to.
    /// @param size Element size.
    /// @param count Element count.
    /// @param file Pointer to file to read from.
    /// @return Number of bytes successfully read from the file.
    size_t read_data_file(char *buffer, size_t size, size_t count, std::ifstream *file);

    /// @brief Curl callback function to store headers in a HeaderArray.
    /// @param buffer Incoming buffer from CURL.
    /// @param size Element size
    /// @param count Element count.
    /// @param array Array to write header to.
    /// @return size * count so curl thinks everything went totally probably fine.
    size_t write_headers_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *array);

    /// @brief Curl callback function that writes the response received to a C++ string.
    /// @param buffer Incoming buffer from CURL.
    /// @param size Element size.
    /// @param count Element count.
    /// @param string String to write/append to.
    /// @return size * count so curl thinks everything went fine nothing bad totally happened at all!
    size_t write_response_string(const char *buffer, size_t size, size_t count, std::string *string);

    /// @brief Tries to locate and extract the value of header and write it to valueOut.
    /// @param list List to search for the header for.
    /// @param header Header string to search for.
    /// @param valueOut String to write the value of the header to.
    /// @return True on success. False on failure/header not found.
    bool get_header_value(const curl::HeaderArray &array, std::string_view header, std::string &valueOut);

    /// @brief Prepares a curl handle for a get request.
    /// @param handle Handle to reset and prepare.
    void prepare_get(curl::Handle &handle);

    /// @brief Prepares the curl handle for a post request.
    /// @param handle Handle to reset and prepare.
    void prepare_post(curl::Handle &handle);

    /// @brief Prepares the curl handle for upload.
    /// @param handle Handle to reset and prepare.
    void prepare_upload(curl::Handle &handle);
} // namespace curl
