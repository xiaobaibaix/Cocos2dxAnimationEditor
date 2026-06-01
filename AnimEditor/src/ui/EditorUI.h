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

    // Popup state
    bool showNewClipPopup_ = false;
    bool showSaveAsPopup_ = false;
    bool showOpenPopup_ = false;
    char popupTextBuf_[256] = {};

    void renderMenuBar();
    void renderPopups();
    void syncProjectToUI();
};

} // namespace anim
