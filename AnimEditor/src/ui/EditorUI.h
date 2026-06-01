#pragma once
#include "ui/FileBrowserPanel.h"
#include "ui/NodeTreePanel.h"
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
    PropertyPanel propertyPanel_;
    TimelinePanel timelinePanel_;
    SceneGraph sceneGraph_;
    UndoSystem undoSystem_;
    std::shared_ptr<anim::AnimProject> currentProject_;
    std::string currentFilePath_;

    void renderMenuBar();
};

} // namespace anim
