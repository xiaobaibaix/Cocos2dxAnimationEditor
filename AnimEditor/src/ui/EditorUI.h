#pragma once
#include "ui/FileBrowserPanel.h"
#include "core/AnimData.h"
#include <memory>
#include <string>

namespace anim {

class EditorUI {
public:
    bool init();
    void shutdown();
    void render();

    void openProject(const std::string& folderPath);
    void openAnimFile(const std::string& filePath);

private:
    FileBrowserPanel fileBrowser_;
    std::shared_ptr<anim::AnimProject> currentProject_;
    std::string currentFilePath_;

    void renderMenuBar();
};

} // namespace anim
