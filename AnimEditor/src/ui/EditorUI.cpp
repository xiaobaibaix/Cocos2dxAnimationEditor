#include "ui/EditorUI.h"
#include "core/Serializer.h"
#include "imgui.h"

#include <algorithm>

namespace anim {

bool EditorUI::init() {
    fileBrowser_.setOnFileOpen([this](const std::string& path) {
        if (path.size() >= 5 &&
            path.compare(path.size() - 5, 5, ".anim") == 0) {
            openAnimFile(path);
        }
    });

    nodeTreePanel_.setSceneGraph(&sceneGraph_);
    nodeTreePanel_.setOnNodeSelected([this](const std::string& nodeId) {
        if (nodeId.empty()) {
            propertyPanel_.setNode(nullptr);
        } else {
            auto found = sceneGraph_.findById(nodeId);
            if (found) {
                propertyPanel_.setNode(*found);
            }
        }
        timelinePanel_.setSelectedNode(nodeId);
    });

    propertyPanel_.setOnPropertyChanged([](const std::string& /*nodeId*/) {
        // Property changed notification — can trigger auto-save or undo registration later
    });

    // Timeline panel callbacks
    timelinePanel_.setOnTimeChanged([this](float time) {
        // Update current time — Phase 4 will drive preview via AnimationEngine
        // Time is already updated inside TimelinePanel via slider/buttons
    });

    timelinePanel_.setOnKeyframeAdded([this](const std::string& nodeId, const std::string& property) {
        if (!currentProject_ || currentProject_->animations.empty()) return;
        auto& anim = currentProject_->animations.back();
        // Find or create track
        Track* targetTrack = nullptr;
        for (auto& t : anim.tracks) {
            if (t.nodeId == nodeId && t.property == property) {
                targetTrack = &t;
                break;
            }
        }
        if (!targetTrack) {
            anim.tracks.push_back(Track{nodeId, property, {}});
            targetTrack = &anim.tracks.back();
        }
        float time = timelinePanel_.getCurrentTime();
        targetTrack->keyframes.push_back(Keyframe{time, 0.0f, EasingType::Linear});
    });

    timelinePanel_.setOnKeyframeRemoved([this](const std::string& nodeId, const std::string& property, int index) {
        if (!currentProject_ || currentProject_->animations.empty()) return;
        auto& anim = currentProject_->animations.back();
        for (auto& t : anim.tracks) {
            if (t.nodeId == nodeId && t.property == property) {
                if (index >= 0 && index < static_cast<int>(t.keyframes.size())) {
                    t.keyframes.erase(t.keyframes.begin() + index);
                }
                return;
            }
        }
    });

    sceneGraph_.setOnChanged([this]() {
        // Scene graph changed — can trigger auto-save or dirty flag later
    });

    // Initialize preview canvas (renders to FBO, displayed via ImGui texture)
    previewCanvas_.init(800, 600);

    return true;
}

void EditorUI::shutdown() {
    previewCanvas_.shutdown();
}

void EditorUI::render() {
    renderMenuBar();
    fileBrowser_.render();

    // Preview canvas with FBO-rendered content
    previewCanvas_.render();

    // Node Tree + Timeline bottom panel
    ImGui::Begin("Node Tree + Timeline");
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float nodeTreeWidth = panelWidth * 0.35f;

    // Left side: Node Tree
    ImGui::BeginChild("NodeTreeSide", ImVec2(nodeTreeWidth, 0), true);
    nodeTreePanel_.render();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right side: Timeline panel
    ImGui::BeginChild("TimelineSide", ImVec2(0, 0), true);
    timelinePanel_.render();
    ImGui::EndChild();

    ImGui::End();

    // Properties panel
    propertyPanel_.render();
}

void EditorUI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Folder...")) {
                // TODO: native folder dialog
            }
            if (ImGui::MenuItem("New Animation")) {
                sceneGraph_.clear();
                undoSystem_.clear();
                currentProject_ = std::make_shared<AnimProject>();
                currentFilePath_.clear();
                nodeTreePanel_.setSelectedNode("");
                propertyPanel_.setNode(nullptr);

                // Create a default animation and wire it to the timeline
                Animation defaultAnim;
                defaultAnim.name = "New Animation";
                defaultAnim.duration = 2.0f;
                defaultAnim.loop = false;
                currentProject_->animations.push_back(std::move(defaultAnim));

                timelinePanel_.setProject(currentProject_.get());
                timelinePanel_.setCurrentAnimation("New Animation");
                timelinePanel_.setCurrentTime(0.0f);
                timelinePanel_.setSelectedNode("");
            }
            if (ImGui::MenuItem("Save")) {
                // TODO: save current project
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, undoSystem_.canUndo())) {
                undoSystem_.undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, undoSystem_.canRedo())) {
                undoSystem_.redo();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorUI::openProject(const std::string& folderPath) {
    fileBrowser_.setRootPath(folderPath);
}

void EditorUI::openAnimFile(const std::string& filePath) {
    auto project = Serializer::loadFromFile(filePath);
    if (project) {
        currentProject_ = std::make_shared<AnimProject>(std::move(*project));
        currentFilePath_ = filePath;
    }
}

} // namespace anim
