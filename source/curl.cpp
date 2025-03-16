#include "curl.hpp"
#include "stringutil.hpp"

namespace
{
    /// @brief This is the size of the upload buffer used by CURL uploads.
    constexpr size_t SIZE_CURL_UPLOAD_BUFFER = 0x10000;
} // namespace

size_t curl::read_data_file(char *buffer, size_t size, size_t count, std::ifstream *file)
{
    file->read(buffer, size * count);
    // Not sure if curl will automatically kill the upload once this returns less than size * count.
    return file->gcount();
}

size_t curl::write_headers_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *array)
{
    // Emplace the header.
    array->emplace_back(buffer, buffer + (size * count));
    // Strip new lines from the string.
    stringutil::strip_character(array->back(), '\n');
    stringutil::strip_character(array->back(), '\r');
    // Return what curl expects so it thinks everything went fine~
    return size * count;
}

size_t curl::write_response_string(const char *buffer, size_t size, size_t count, std::string *string)
{
    string->append(buffer, buffer + (size * count));
    return size * count;
}

bool curl::get_header_value(const curl::HeaderArray &array, std::string_view header, std::string &valueOut)
{
    for (const std::string &currentHeader : array)
    {
        // Get the colon position.
        size_t colon = currentHeader.find_first_of(':');
        if (colon == currentHeader.npos)
        {
            continue;
        }
        else if (currentHeader.substr(0, colon) != header)
        {
            continue;
        }

        size_t valueBegin = currentHeader.find_first_not_of(' ', colon + 1);
        if (valueBegin == currentHeader.npos)
        {
            continue;
        }

        // This should just cut out what's left after the colon and spaces.
        valueOut = currentHeader.substr(valueBegin, currentHeader.npos);

        return true;
    }
    return false;
}

void curl::prepare_get(curl::Handle &handle)
{
    // Reset
    curl_easy_reset(handle.get());

    curl::set_option(handle, CURLOPT_HTTPGET, 1L);
    curl::set_option(handle, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(handle, CURLOPT_ACCEPT_ENCODING, ""); // I think this is how you use the defaults?
}

void curl::prepare_post(curl::Handle &handle)
{
    curl_easy_reset(handle.get());

    curl::set_option(handle, CURLOPT_POST, 1L);
    curl::set_option(handle, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(handle, CURLOPT_ACCEPT_ENCODING, ""); // Same as above. Not sure if this does anything for POST.
}

void curl::prepare_upload(curl::Handle &handle)
{
    curl_easy_reset(handle.get());

    curl::set_option(handle, CURLOPT_UPLOAD, 1L);
    curl::set_option(handle, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(handle, CURLOPT_UPLOAD_BUFFERSIZE, SIZE_CURL_UPLOAD_BUFFER);
    curl::set_option(handle, CURLOPT_ACCEPT_ENCODING, "");
}
