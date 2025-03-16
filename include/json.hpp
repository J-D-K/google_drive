#pragma once
#include <json-c/json.h>
#include <memory>

namespace json
{
    /// @brief Definition for a self freeing json_object.
    using Object = std::unique_ptr<json_object, decltype(&json_object_put)>;

    /// @brief Templated function to create self freeing json_objects.
    /// @tparam Args Argument types to pass to the jsonFunction.
    /// @param jsonFunction Function used to allocate the json_object struct.
    /// @param args Arguments passed to the json function.
    /// @return New self-freeing json_object.
    template <typename... Args>
    static inline json::Object new_object(json_object *(*function)(Args...), Args... args)
    {
        return json::Object(function(args...), json_object_put);
    }

    /// @brief Wrapper function adding objects to a unique_ptr wrapper json_object.
    /// @param object Target object to add the object to.
    /// @param key JSON key.
    /// @param add Object to add.
    /// @return Same value that json_object_object_add returns.
    static inline int add_object(json::Object &object, const char *key, json_object *add)
    {
        return json_object_object_add(object.get(), key, add);
    }

    /// @brief Wrapper function for getting objects from a unique_ptr wrapped json_object.
    /// @param object Object to get from.
    /// @param key Key to get.
    /// @return Pointer on success. NULL on failure.
    static inline json_object *get_object(json::Object &object, const char *key)
    {
        return json_object_object_get(object.get(), key);
    }
} // namespace json
