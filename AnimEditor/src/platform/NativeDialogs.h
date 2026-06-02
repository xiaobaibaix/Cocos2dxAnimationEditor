#pragma once
#include <string>

namespace anim {

// Opens a native Save File dialog. Returns empty string if cancelled.
std::string nativeSaveDialog(const std::string& defaultName);

// Opens a native Open File dialog filtered to .anim files.
// Returns empty string if cancelled.
std::string nativeOpenDialog();

} // namespace anim
