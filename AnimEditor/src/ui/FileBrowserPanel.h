#pragma once
#include <string>
#include <vector>
#include <functional>

namespace anim {

struct FileEntry {
    std::string name;
    std::string path;
    bool isDirectory;
    bool isAnimFile;   // .anim
    bool isImageFile;  // .png .jpg
};

class FileBrowserPanel {
public:
    using OnFileOpen = std::function<void(const std::string& path)>;
    using OnFileDeleted = std::function<void(const std::string& path)>;

    void setRootPath(const std::string& path);
    void setOnFileOpen(OnFileOpen cb) { onFileOpen_ = std::move(cb); }
    void setOnFileDeleted(OnFileDeleted cb) { onFileDeleted_ = std::move(cb); }
    void render();

private:
    std::string rootPath_;
    std::string currentPath_;
    std::vector<FileEntry> entries_;
    OnFileOpen onFileOpen_;
    OnFileDeleted onFileDeleted_;

    void refreshDirectory(const std::string& path);
};

} // namespace anim
