#pragma once
#include "Item.hpp"
#include <string>
#include <vector>

/// @brief This is the base storage class.
class Storage
{
    public:
        /// @brief List type definition.
        using ItemList = std::vector<Item>;

        /// @brief Item list iterator.
        using ItemIterator = std::vector<Item>::iterator;

        /// @brief Base, default storage constructor.
        Storage(void) = default;

        /// @brief Constructor that sets the root and parent right away.
        Storage(std::string_view root);

        /// @brief Returns whether or not initializing the derived storage type was successful.
        /// @return True if init was successful. False if it wasn't.
        bool is_initialized(void) const;

        /// @brief Returns the parent directory to the root/starting directory.
        void return_to_root(void);

        /// @brief Returns whether or not a directory exists within the current parent.
        /// @param name Name of the directory to search for.
        /// @return True if one is found. False if one isn't.
        bool directory_exists(std::string_view name);

        /// @brief Returns whether or not a file exists within the current parent.
        /// @param name Name of the file to search for.
        /// @return True if one is found. False if one isn't.
        bool file_exists(std::string_view name);

        /// @brief Gets the ID string of the directory named name.
        /// @param name Name of the directory to get the ID of.
        /// @param out String to write the ID to.
        /// @return True on success. False if the directory wasn't found.
        bool get_directory_id(std::string_view name, std::string &out);

        /// @brief Gets the ID string of the directory at index.
        /// @param index Index of the directory to get.
        /// @param out String to write the ID to.
        /// @return True on success. False if index is out of bounds.
        bool get_directory_id(int index, std::string &out);

        /// @brief Gets the ID string of the file with name.
        /// @param name Name of the file to get the ID of.
        /// @param out String to write the ID to.
        /// @return True on success. False if no file with name is found.
        bool get_file_id(std::string_view name, std::string &out);

        /// @brief Gets the ID of the file at index.
        /// @param index Index of the file to get the ID of.
        /// @param out String to write the ID to.
        /// @return True on success. False if index is out of bounds.
        bool get_file_id(int index, std::string &out);


        /// @brief Changes the current parent directory.
        /// @param name Name/ID of the folder to change to.
        virtual void change_directory(std::string_view name) = 0;

        /// @brief Virtual function to create a new directory in the current parent.
        /// @param name Name of the directory to create.
        /// @return True on success. False on failure.
        virtual bool create_directory(std::string_view name) = 0;

        /// @brief Virtual function to delete a directory from the current parent.
        /// @param name Name/ID of the directory to delete.
        /// @return True on success. False on failure.
        virtual bool delete_directory(std::string_view name) = 0;

        /// @brief Virtual function to delete a file in the currently set parent.
        /// @param name Name/ID of the file to delete.
        /// @return True on success. False on failure.
        virtual bool delete_file(std::string_view name) = 0;

        /// @brief Prints the contents of m_list.
        virtual void list_contents(void) const = 0;

    protected:
        /// @brief Stores whether or not init'ing the Storage was successful.
        bool m_isInitialized = false;

        /// @brief The starting, root directory.
        std::string m_root;

        /// @brief Current parent directory name/id.
        std::string m_parent;

        /// @brief Storage item list.
        Storage::ItemList m_list;

        /// @brief Returns if a directory with the currently set parent can be found.
        /// @param name Name of the directory to search for.
        /// @return Iterator to the directory. m_list.end() on failure.
        Storage::ItemIterator find_directory(std::string_view name);

        /// @brief Returns if a file with the currently set parent can be found.
        /// @param name Name of the file to search for.
        /// @return Iterator to the file. m_list.end() on failure.
        Storage::ItemIterator find_file(std::string_view name);
};