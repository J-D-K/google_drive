#include "Storage.hpp"

/// @brief Local type storage class.
class Local final : public Storage
{
    public:
        /// @brief Local storage constructor.
        /// @param root The starting/root directory for the local storage.
        Local(std::string_view root);

        /// @brief Changes the current parent or working directory.
        /// @param name Name of the directory to change to. Passing .. will go up one directory.
        void change_directory(std::string_view name) override;

        /// @brief Creates a new directory named name in the current parent directory.
        /// @param name Name of the directory to create.
        /// @return True on success. False on failure.
        bool create_directory(std::string_view name) override;

        /// @brief Recursively deletes a directory named name in the current parent/working directory.
        /// @param name Name of the directory to delete.
        /// @return True on success. False on failure.
        bool delete_directory(std::string_view name) override;

        /// @brief Attempts to delete a file named name from the current parent/working directory.
        /// @param name Name of the file to delete.
        /// @return True on success. False on failure.
        bool delete_file(std::string_view name) override;

    private:
        /// @brief Loads and stores the listing of the current parent/working directory.
        void load_parent_listing(void);
};