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

        // Find the currently selected animation clip
        std::string currentAnimName = timelinePanel_.getCurrentAnimationName();
        Animation* anim = nullptr;
        for (auto& a : currentProject_->animations) {
            if (a.name == currentAnimName) {
                anim = &a;
                break;
            }
        }
        if (!anim) return;

        // Find or create track
        Track* targetTrack = nullptr;
        for (auto& t : anim->tracks) {
            if (t.nodeId == nodeId && t.property == property) {
                targetTrack = &t;
                break;
            }
        }
        if (!targetTrack) {
            anim->tracks.push_back(Track{nodeId, property, {}});
            targetTrack = &anim->tracks.back();
        }
        float time = timelinePanel_.getCurrentTime();
        targetTrack->keyframes.push_back(Keyframe{time, 0.0f, EasingType::Linear});
    });

    timelinePanel_.setOnKeyframeRemoved([this](const std::string& nodeId, const std::string& property, int index) {
        if (!currentProject_ || currentProject_->animations.empty()) return;

        std::string currentAnimName = timelinePanel_.getCurrentAnimationName();
        Animation* anim = nullptr;
        for (auto& a : currentProject_->animations) {
            if (a.name == currentAnimName) {
                anim = &a;
                break;
            }
        }
        if (!anim) return;

        for (auto& t : anim->tracks) {
            if (t.nodeId == nodeId && t.property == property) {
                if (index >= 0 && index < static_cast<int>(t.keyframes.size())) {
                    t.keyframes.erase(t.keyframes.begin() + index);
                }
                return;
            }
        }
    });

    // Canvas node dragging — update selected node position
    previewCanvas_.setOnNodeDragged([this](float dx, float dy) {
        const std::string& selectedId = nodeTreePanel_.getSelectedNode();
        if (selectedId.empty()) return;

        auto nodeOpt = sceneGraph_.findById(selectedId);
        if (!nodeOpt) return;

        auto& node = *nodeOpt;
        node->properties.position.x += dx;
        node->properties.position.y += dy;

        // Refresh the property panel to reflect new position
        propertyPanel_.setNode(node);
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
    renderPopups();
    fileBrowser_.render();

    // Preview canvas with FBO-rendered content
    previewCanvas_.render();

    // Properties panel (docked to top-right by DockBuilder)
    propertyPanel_.render();

    // Nodes panel (docked to bottom-left by DockBuilder)
    nodeTreePanel_.render();

    // Timeline panel (docked to bottom-right by DockBuilder)
    timelinePanel_.render();
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

                Animation defaultAnim;
                defaultAnim.name = "New Animation";
                defaultAnim.duration = 2.0f;
                defaultAnim.loop = false;
                currentProject_->animations.push_back(std::move(defaultAnim));

                syncProjectToUI();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (currentProject_) {
                    if (!currentFilePath_.empty()) {
                        Serializer::saveToFile(*currentProject_, currentFilePath_);
                    } else {
                        // No file path yet — open Save As popup
                        showSaveAsPopup_ = true;
                        popupTextBuf_[0] = '\0';
                    }
                }
            }
            if (ImGui::MenuItem("Save As...")) {
                showSaveAsPopup_ = true;
                popupTextBuf_[0] = '\0';
            }
            if (ImGui::MenuItem("Open .anim...")) {
                showOpenPopup_ = true;
                popupTextBuf_[0] = '\0';
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Animation")) {
            if (ImGui::MenuItem("New Clip")) {
                showNewClipPopup_ = true;
                popupTextBuf_[0] = '\0';
            }
            if (ImGui::MenuItem("Delete Clip", nullptr, false,
                                currentProject_ && !currentProject_->animations.empty())) {
                if (currentProject_) {
                    const std::string& animName = timelinePanel_.getCurrentAnimationName();
                    auto eraseIt = std::remove_if(
                        currentProject_->animations.begin(),
                        currentProject_->animations.end(),
                        [&animName](const Animation& a) { return a.name == animName; });

                    if (eraseIt != currentProject_->animations.end()) {
                        currentProject_->animations.erase(eraseIt, currentProject_->animations.end());
                    }

                    // Switch to first remaining clip or clear
                    if (!currentProject_->animations.empty()) {
                        timelinePanel_.setCurrentAnimation(currentProject_->animations.front().name);
                        timelinePanel_.setCurrentTime(0.0f);
                    } else {
                        timelinePanel_.setCurrentAnimation("");
                    }
                }
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

void EditorUI::renderPopups() {
    // New Clip popup
    if (showNewClipPopup_) {
        ImGui::OpenPopup("New Clip");
        showNewClipPopup_ = false;
    }
    if (ImGui::BeginPopupModal("New Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter clip name:");
        bool confirmed = ImGui::InputText("##clipName", popupTextBuf_, sizeof(popupTextBuf_),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("OK") || confirmed) {
            if (popupTextBuf_[0] != '\0' && currentProject_) {
                Animation newClip;
                newClip.name = popupTextBuf_;
                newClip.duration = 2.0f;
                newClip.loop = false;
                currentProject_->animations.push_back(std::move(newClip));

                timelinePanel_.setProject(currentProject_.get());
                timelinePanel_.setCurrentAnimation(currentProject_->animations.back().name);
                timelinePanel_.setCurrentTime(0.0f);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Save As popup
    if (showSaveAsPopup_) {
        ImGui::OpenPopup("Save As");
        showSaveAsPopup_ = false;
    }
    if (ImGui::BeginPopupModal("Save As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File path:");
        bool confirmed = ImGui::InputText("##savePath", popupTextBuf_, sizeof(popupTextBuf_),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Save") || confirmed) {
            if (popupTextBuf_[0] != '\0' && currentProject_) {
                saveAs(popupTextBuf_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Open .anim popup
    if (showOpenPopup_) {
        ImGui::OpenPopup("Open .anim");
        showOpenPopup_ = false;
    }
    if (ImGui::BeginPopupModal("Open .anim", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File path:");
        bool confirmed = ImGui::InputText("##openPath", popupTextBuf_, sizeof(popupTextBuf_),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Open") || confirmed) {
            if (popupTextBuf_[0] != '\0') {
                openAnimFile(popupTextBuf_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorUI::syncProjectToUI() {
    if (!currentProject_) return;

    // Rebuild scene graph from project node tree
    sceneGraph_.clear();
    for (const auto& nodePtr : currentProject_->nodeTree) {
        // Add each root node to the scene graph
        auto added = sceneGraph_.addNode(nodePtr->id, nodePtr->type, nodePtr->name, std::nullopt);
        // TODO: recursively add children when SceneGraph supports deeper insertion
    }

    // Sync timeline to the first animation clip
    timelinePanel_.setProject(currentProject_.get());
    if (!currentProject_->animations.empty()) {
        timelinePanel_.setCurrentAnimation(currentProject_->animations.front().name);
    } else {
        timelinePanel_.setCurrentAnimation("");
    }
    timelinePanel_.setCurrentTime(0.0f);
    timelinePanel_.setSelectedNode("");

    // Reset selection state
    nodeTreePanel_.setSelectedNode("");
    propertyPanel_.setNode(nullptr);
    undoSystem_.clear();
}

void EditorUI::openProject(const std::string& folderPath) {
    fileBrowser_.setRootPath(folderPath);
}

void EditorUI::saveAs(const std::string& path) {
    if (!currentProject_) return;
    currentFilePath_ = path;
    Serializer::saveToFile(*currentProject_, currentFilePath_);
}

void EditorUI::openAnimFile(const std::string& filePath) {
    auto project = Serializer::loadFromFile(filePath);
    if (project) {
        currentProject_ = std::make_shared<AnimProject>(std::move(*project));
        currentFilePath_ = filePath;
        syncProjectToUI();
    }
}

} // namespace anim
