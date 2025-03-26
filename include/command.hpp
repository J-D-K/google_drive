#pragma once
#include "Storage.hpp"

/// @brief Executes the command passed using the storage reference passed.
/// @param storage Reference to storage to use.
/// @return True on success. False on bad command parameters or error.
bool execute_command(Storage &storage);