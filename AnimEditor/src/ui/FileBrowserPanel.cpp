#include "ui/FileBrowserPanel.h"
#include "imgui.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace anim {

namespace {

bool hasExtension(const std::string& filename, const std::vector<std::string>& exts) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& ext : exts) {
        if (lower.size() >= ext.size() &&
            lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

bool isAnimFile(const std::string& filename) {
    return hasExtension(filename, {".anim"});
}

bool isImageFile(const std::string& filename) {
    return hasExtension(filename, {".png", ".jpg", ".jpeg", ".webp"});
}

bool isFontFile(const std::string& filename) {
    return hasExtension(filename, {".ttf", ".otf"});
}

bool isPlistFile(const std::string& filename) {
    return hasExtension(filename, {".plist"});
}

bool isRelevantFile(const std::string& filename) {
    return isAnimFile(filename) || isImageFile(filename) ||
           isFontFile(filename) || isPlistFile(filename);
}

} // anonymous namespace

void FileBrowserPanel::setRootPath(const std::string& path) {
    rootPath_ = path;
    currentPath_ = path;
    refreshDirectory(currentPath_);
}

void FileBrowserPanel::refreshDirectory(const std::string& path) {
    entries_.clear();

    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }

    // Add ".." entry if we are above the root
    fs::path absCurrent = fs::canonical(path);
    fs::path absRoot = fs::canonical(rootPath_);

    if (absCurrent != absRoot) {
        FileEntry parent;
        parent.name = "..";
        parent.path = absCurrent.parent_path().string();
        parent.isDirectory = true;
        parent.isAnimFile = false;
        parent.isImageFile = false;
        entries_.push_back(parent);
    }

    // Collect directories and files separately
    std::vector<FileEntry> dirs;
    std::vector<FileEntry> files;

    for (const auto& entry : fs::directory_iterator(path)) {
        std::string name = entry.path().filename().string();

        // Skip hidden files/directories
        if (!name.empty() && name[0] == '.') {
            continue;
        }

        if (entry.is_directory()) {
            FileEntry fe;
            fe.name = name;
            fe.path = entry.path().string();
            fe.isDirectory = true;
            fe.isAnimFile = false;
            fe.isImageFile = false;
            dirs.push_back(std::move(fe));
        } else if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (!isRelevantFile(filename)) {
                continue;
            }

            FileEntry fe;
            fe.name = name;
            fe.path = entry.path().string();
            fe.isDirectory = false;
            fe.isAnimFile = isAnimFile(filename);
            fe.isImageFile = isImageFile(filename);
            files.push_back(std::move(fe));
        }
    }

    // Sort directories and files alphabetically (case-insensitive)
    auto caseInsensitiveLess = [](const FileEntry& a, const FileEntry& b) {
        std::string lowerA = a.name;
        std::string lowerB = b.name;
        std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lowerA < lowerB;
    };

    std::sort(dirs.begin(), dirs.end(), caseInsensitiveLess);
    std::sort(files.begin(), files.end(), caseInsensitiveLess);

    // Directories first, then files
    entries_.insert(entries_.end(), dirs.begin(), dirs.end());
    entries_.insert(entries_.end(), files.begin(), files.end());
}

void FileBrowserPanel::render() {
    ImGui::Begin("Files");

    // Display current path
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", currentPath_.c_str());
    ImGui::Separator();

    // Render each entry
    for (const auto& entry : entries_) {
        // Pick color based on entry type
        ImVec4 color;
        if (entry.isDirectory) {
            color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Blue for directories
        } else if (entry.isAnimFile) {
            color = ImVec4(1.0f, 0.9f, 0.3f, 1.0f);   // Yellow for .anim
        } else if (entry.isImageFile) {
            color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);   // Green for images
        } else {
            color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);   // Gray for others
        }

        // Build display label with prefix icon
        std::string label;
        if (entry.isDirectory) {
            label = "[D] " + entry.name;
        } else if (entry.isAnimFile) {
            label = "[A] " + entry.name;
        } else if (entry.isImageFile) {
            label = "[I] " + entry.name;
        } else {
            label = "    " + entry.name;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);

        bool selected = false;
        if (ImGui::Selectable(label.c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                // Double-click
                if (entry.isDirectory) {
                    currentPath_ = entry.path;
                    refreshDirectory(currentPath_);
                } else {
                    if (onFileOpen_) {
                        onFileOpen_(entry.path);
                    }
                }
            } else {
                // Single-click
                if (entry.isDirectory) {
                    currentPath_ = entry.path;
                    refreshDirectory(currentPath_);
                } else {
                    if (onFileOpen_) {
                        onFileOpen_(entry.path);
                    }
                }
            }
        }

        ImGui::PopStyleColor();
    }

    ImGui::End();
}

} // namespace anim
