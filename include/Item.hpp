#pragma once
#include <string>

class Item
{
    public:
        /// @brief Instantiates a new Item.
        /// @param name Name of the item.
        /// @param id ID of the item.
        /// @param parent Parent ID of the item.
        /// @param isDirectory Whether or not the item is a directory.
        Item(std::string_view name, std::string_view id, std::string_view parent, bool isDirectory);

        /// @brief Returns the name of the item.
        /// @return Name of the item.
        std::string_view get_name(void) const;

        /// @brief Returns the ID of item.
        /// @return ID of the item.
        std::string_view get_id(void) const;

        /// @brief Returns the id of the parent of the item.
        /// @return Parent ID of the item.
        std::string_view get_parent_id(void) const;

        /// @brief Returns whether or not the item is a directory.
        /// @return True if the item is a directory. False if it isn't.
        bool is_directory(void) const;

        /// @brief Sets the name of the item.
        /// @param name New name of the item.
        void set_name(std::string_view name);

        /// @brief Sets the ID of the item.
        /// @param id New ID of the item.
        void set_id(std::string_view id);

        /// @brief Sets the parent ID of the item.
        /// @param parent New parent ID of the item.
        void set_parent_id(std::string_view parent);

        /// @brief Sets whether the item is a directory.
        /// @param isDirectory Whether the item is a directory.
        void set_is_directory(bool isDirectory);

    private:
        /// @brief Item's name.
        std::string m_name;

        /// @brief Item's ID.
        std::string m_id;

        /// @brief Item's parent.
        std::string m_parent;

        /// @brief Whether or not the item is a directory.
        bool m_isDirectory;
};
