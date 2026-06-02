#include "ui/EditorUI.h"
#include "core/Serializer.h"
#include "platform/NativeDialogs.h"
#include "imgui.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>

namespace anim {

// ── Recent files persistence ───────────────────────────────────────────

std::string EditorUI::recentFilesPath() {
    const char* home = getenv("HOME");
    if (!home) return ".AnimEditor/recent.json";
    std::string dir = std::string(home) + "/.AnimEditor";
    mkdir(dir.c_str(), 0755);
    return dir + "/recent.json";
}

void EditorUI::loadRecentFiles() {
    recentFiles_.clear();
    std::ifstream in(recentFilesPath());
    if (!in.is_open()) return;

    try {
        auto json = nlohmann::json::parse(in);
        if (json.is_array()) {
            for (const auto& entry : json) {
                if (entry.is_string() && recentFiles_.size() < kMaxRecentFiles) {
                    recentFiles_.push_back(entry.get<std::string>());
                }
            }
        }
    } catch (...) {
        recentFiles_.clear();
    }
}

void EditorUI::saveRecentFiles() {
    nlohmann::json json = nlohmann::json::array();
    for (const auto& path : recentFiles_) {
        json.push_back(path);
    }
    std::ofstream out(recentFilesPath());
    if (out.is_open()) {
        out << json.dump(2);
    }
}

void EditorUI::addRecentFile(const std::string& path) {
    auto it = std::find(recentFiles_.begin(), recentFiles_.end(), path);
    if (it != recentFiles_.end()) {
        recentFiles_.erase(it);
    }
    recentFiles_.insert(recentFiles_.begin(), path);
    if (recentFiles_.size() > kMaxRecentFiles) {
        recentFiles_.resize(kMaxRecentFiles);
    }
    saveRecentFiles();
}

// ── Init ───────────────────────────────────────────────────────────────

bool EditorUI::init() {
    loadRecentFiles();

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
    });

    timelinePanel_.setOnTimeChanged([this](float time) {
    });

    timelinePanel_.setOnKeyframeAdded([this](const std::string& nodeId, const std::string& property) {
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

    previewCanvas_.setOnNodeDragged([this](float dx, float dy) {
        const std::string& selectedId = nodeTreePanel_.getSelectedNode();
        if (selectedId.empty()) return;

        auto nodeOpt = sceneGraph_.findById(selectedId);
        if (!nodeOpt) return;

        auto& node = *nodeOpt;
        node->properties.position.x += dx;
        node->properties.position.y += dy;

        propertyPanel_.setNode(node);
    });

    sceneGraph_.setOnChanged([this]() {
    });

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

    previewCanvas_.render();
    propertyPanel_.render();
    nodeTreePanel_.render();
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
                        addRecentFile(currentFilePath_);
                    } else {
                        std::string path = nativeSaveDialog("untitled.anim");
                        if (!path.empty()) {
                            saveAs(path);
                            addRecentFile(path);
                        }
                    }
                }
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                std::string path = nativeSaveDialog("untitled.anim");
                if (!path.empty()) {
                    saveAs(path);
                    addRecentFile(path);
                }
            }
            if (ImGui::MenuItem("Open .anim...", "Ctrl+O")) {
                std::string path = nativeOpenDialog();
                if (!path.empty()) {
                    openAnimFile(path);
                    addRecentFile(path);
                }
            }
            ImGui::Separator();

            // Recent files
            if (!recentFiles_.empty()) {
                ImGui::TextDisabled("Recent Files");
                int removeIdx = -1;
                for (size_t i = 0; i < recentFiles_.size(); ++i) {
                    std::string label = std::to_string(i + 1) + ". " + recentFiles_[i];
                    if (ImGui::MenuItem(label.c_str())) {
                        struct stat st;
                        if (stat(recentFiles_[i].c_str(), &st) == 0) {
                            openAnimFile(recentFiles_[i]);
                            addRecentFile(recentFiles_[i]);
                        } else {
                            removeIdx = static_cast<int>(i);
                        }
                    }
                }
                if (removeIdx >= 0) {
                    recentFiles_.erase(recentFiles_.begin() + removeIdx);
                    saveRecentFiles();
                }
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
}

void EditorUI::syncProjectToUI() {
    if (!currentProject_) return;

    sceneGraph_.clear();
    for (const auto& nodePtr : currentProject_->nodeTree) {
        auto added = sceneGraph_.addNode(nodePtr->id, nodePtr->type, nodePtr->name, std::nullopt);
    }

    timelinePanel_.setProject(currentProject_.get());
    if (!currentProject_->animations.empty()) {
        timelinePanel_.setCurrentAnimation(currentProject_->animations.front().name);
    } else {
        timelinePanel_.setCurrentAnimation("");
    }
    timelinePanel_.setCurrentTime(0.0f);
    timelinePanel_.setSelectedNode("");

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
