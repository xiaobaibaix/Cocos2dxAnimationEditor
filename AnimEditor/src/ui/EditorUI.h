#pragma once
#include "ui/FileBrowserPanel.h"
#include "ui/NodeTreePanel.h"
#include "ui/PreviewCanvas.h"
#include "ui/PropertyPanel.h"
#include "ui/TimelinePanel.h"
#include "core/AnimData.h"
#include "core/SceneGraph.h"
#include "core/UndoSystem.h"
#include <memory>
#include <string>
#include <vector>

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
    NodeTreePanel nodeTreePanel_;
    PreviewCanvas previewCanvas_;
    PropertyPanel propertyPanel_;
    TimelinePanel timelinePanel_;
    SceneGraph sceneGraph_;
    UndoSystem undoSystem_;
    std::shared_ptr<anim::AnimProject> currentProject_;
    std::string currentFilePath_;

    // Recent files
    std::vector<std::string> recentFiles_;
    static constexpr size_t kMaxRecentFiles = 10;
    void loadRecentFiles();
    void saveRecentFiles();
    void addRecentFile(const std::string& path);
    std::string recentFilesPath();

    // Popup state
    bool showNewClipPopup_ = false;
    char popupTextBuf_[256] = {};

    void renderMenuBar();
    void renderPopups();
    void syncProjectToUI();
    void saveAs(const std::string& path);
};

} // namespace anim
