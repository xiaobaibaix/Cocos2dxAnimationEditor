#pragma once
#include <string>

namespace anim {

// Opens a native Save File dialog. Returns empty string if cancelled.
std::string nativeSaveDialog(const std::string& defaultName);
std::string nativeSaveDialog(const std::string& defaultName, const std::string& directory);

// Opens a native Open File dialog filtered to .anim files.
// Returns empty string if cancelled.
std::string nativeOpenDialog();

// Opens a native folder picker dialog. Returns empty string if cancelled.
std::string nativeFolderDialog();

} // namespace anim
