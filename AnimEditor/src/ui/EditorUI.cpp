#include "ui/EditorUI.h"
#include "core/Easing.h"
#include "debug/DemoDebugPanel.h"
#include "core/Serializer.h"
#include "platform/NativeDialogs.h"
#include "imgui.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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

// ── Workspace persistence ──────────────────────────────────────────────

static std::string workspaceConfigPath() {
    const char* home = getenv("HOME");
    if (!home) return ".AnimEditor/workspace";
    std::string dir = std::string(home) + "/.AnimEditor";
    mkdir(dir.c_str(), 0755);
    return dir + "/workspace";
}

void EditorUI::loadWorkspacePath() {
    std::ifstream in(workspaceConfigPath());
    if (!in.is_open()) return;
    std::string path;
    if (std::getline(in, path)) {
        struct stat st;
        if (!path.empty() && stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFDIR)) {
            workspacePath_ = path;
            fileBrowser_.setRootPath(path);
        }
    }
}

void EditorUI::saveWorkspacePath() {
    std::ofstream out(workspaceConfigPath());
    if (out.is_open()) {
        out << workspacePath_;
    }
}

// ── Init ───────────────────────────────────────────────────────────────

bool EditorUI::init() {
    loadRecentFiles();
    loadWorkspacePath();

    fileBrowser_.setOnFileOpen([this](const std::string& path) {
        if (path.size() >= 5 &&
            path.compare(path.size() - 5, 5, ".anim") == 0) {
            if (dirty_) {
                pendingAction_ = PendingAction::OpenAnim;
                pendingOpenPath_ = path;
                showConfirmDiscard_ = true;
            } else {
                doOpenAnimFile(path);
            }
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

    nodeTreePanel_.setOnNodeChanged([this]() {
        markDirty();
    });

    nodeTreePanel_.setOnQueryNodeAnimated([this](const std::string& nodeId) -> bool {
        if (!currentProject_ || currentProject_->animations.empty()) return false;
        std::string animName = timelinePanel_.getCurrentAnimationName();
        for (const auto& anim : currentProject_->animations) {
            if (anim.name == animName) {
                for (const auto& track : anim.tracks) {
                    if (track.nodeId == nodeId && !track.keyframes.empty()) {
                        return true;
                    }
                }
                return false;
            }
        }
        return false;
    });

    propertyPanel_.setOnPropertyChanged([this](const std::string& /*nodeId*/) {
        markDirty();
    });

    timelinePanel_.setOnTimeChanged([this](float /*time*/) {
        applyAnimationToNode();
    });

    timelinePanel_.setOnKeyframeAdded([this](const std::string& nodeId, const std::string& property, float time) {
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
            markDirty();
            return;
        }
        bool isVec2 = (property == "position" || property == "scale" || property == "anchor");
        if (isVec2) {
            targetTrack->keyframes.push_back(Keyframe{time, Vec2{0.0f, 0.0f}, EasingType::Linear});
        } else {
            targetTrack->keyframes.push_back(Keyframe{time, 0.0f, EasingType::Linear});
        }
        markDirty();
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
                markDirty();
                return;
            }
        }
    });

    timelinePanel_.setOnKeyframeChanged([this](const std::string& nodeId, const std::string& property,
                                                int index, float newTime) {
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
                    t.keyframes[index].time = newTime;
                    // Sort keyframes by time after drag
                    std::sort(t.keyframes.begin(), t.keyframes.end(),
                              [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
                }
                markDirty();
                applyAnimationToNode();
                return;
            }
        }
    });

    timelinePanel_.setOnKeyframeValueChanged([this](const std::string& nodeId, const std::string& property,
                                                     int index, const Vec2& newValue) {
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
                    if (std::get_if<Vec2>(&t.keyframes[index].value)) {
                        t.keyframes[index].value = newValue;
                    } else {
                        t.keyframes[index].value = newValue.x;
                    }
                }
                markDirty();
                applyAnimationToNode();
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
        markDirty();
    });

    sceneGraph_.setOnChanged([this]() {
        markDirty();
    });

    previewCanvas_.init(800, 600);

    registerDebugPanel<DemoDebugPanel>();

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
    debugHost_.renderPanels();
}

void EditorUI::showConfirmDiscard() {
    showConfirmDiscard_ = true;
}

void EditorUI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Folder...")) {
                std::string folder = nativeFolderDialog();
                if (!folder.empty()) {
                    workspacePath_ = folder;
                    fileBrowser_.setRootPath(folder);
                    saveWorkspacePath();
                }
            }
            if (ImGui::MenuItem("New Animation", nullptr, false, !workspacePath_.empty())) {
                if (dirty_) {
                    pendingAction_ = PendingAction::NewAnimation;
                    showConfirmDiscard_ = true;
                } else {
                    doNewAnimation();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                doSave();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                std::string path = nativeSaveDialog("untitled.anim");
                if (!path.empty()) {
                    saveAs(path);
                    addRecentFile(path);
                }
            }
            if (ImGui::MenuItem("Open .anim...", "Ctrl+O")) {
                if (dirty_) {
                    pendingAction_ = PendingAction::OpenAnim;
                    pendingOpenPath_.clear();
                    showConfirmDiscard_ = true;
                } else {
                    std::string path = nativeOpenDialog();
                    if (!path.empty()) {
                        doOpenAnimFile(path);
                        addRecentFile(path);
                    }
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
                            if (dirty_) {
                                pendingAction_ = PendingAction::OpenAnim;
                                pendingOpenPath_ = recentFiles_[i];
                                showConfirmDiscard_ = true;
                            } else {
                                doOpenAnimFile(recentFiles_[i]);
                                addRecentFile(recentFiles_[i]);
                            }
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
                        markDirty();
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
                markDirty();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, undoSystem_.canRedo())) {
                undoSystem_.redo();
                markDirty();
            }
            ImGui::EndMenu();
        }
        debugHost_.renderMenu();
        ImGui::EndMainMenuBar();
    }
}

void EditorUI::renderPopups() {
    // Confirm Discard popup
    if (showConfirmDiscard_) {
        ImGui::OpenPopup("Unsaved Changes");
        showConfirmDiscard_ = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes. Save before continuing?");
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(100, 0))) {
            doSave();
            ImGui::CloseCurrentPopup();
            // Execute pending action after save
            if (pendingAction_ == PendingAction::NewAnimation) {
                doNewAnimation();
            } else if (pendingAction_ == PendingAction::OpenAnim) {
                if (pendingOpenPath_.empty()) {
                    std::string path = nativeOpenDialog();
                    if (!path.empty()) {
                        doOpenAnimFile(path);
                        addRecentFile(path);
                    }
                } else {
                    doOpenAnimFile(pendingOpenPath_);
                    addRecentFile(pendingOpenPath_);
                }
            }
            pendingAction_ = PendingAction::None;
            pendingOpenPath_.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            markClean();
            if (pendingAction_ == PendingAction::NewAnimation) {
                doNewAnimation();
            } else if (pendingAction_ == PendingAction::OpenAnim) {
                if (pendingOpenPath_.empty()) {
                    std::string path = nativeOpenDialog();
                    if (!path.empty()) {
                        doOpenAnimFile(path);
                        addRecentFile(path);
                    }
                } else {
                    doOpenAnimFile(pendingOpenPath_);
                    addRecentFile(pendingOpenPath_);
                }
            } else {
                wantsToQuit_ = true;
            }
            pendingAction_ = PendingAction::None;
            pendingOpenPath_.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            pendingAction_ = PendingAction::None;
            pendingOpenPath_.clear();
        }
        ImGui::EndPopup();
    }

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
                markDirty();
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

void EditorUI::doSave() {
    if (!currentProject_) return;
    if (!currentFilePath_.empty()) {
        Serializer::saveToFile(*currentProject_, currentFilePath_);
        addRecentFile(currentFilePath_);
        markClean();
    } else {
        std::string path = nativeSaveDialog("untitled.anim");
        if (!path.empty()) {
            saveAs(path);
            addRecentFile(path);
            markClean();
        }
    }
}

void EditorUI::doNewAnimation() {
    sceneGraph_.clear();
    undoSystem_.clear();
    currentProject_ = std::make_shared<AnimProject>();
    currentFilePath_.clear();

    Animation defaultAnim;
    defaultAnim.name = "New Animation";
    defaultAnim.duration = 2.0f;
    defaultAnim.loop = false;
    currentProject_->animations.push_back(std::move(defaultAnim));

    auto defaultNode = std::make_shared<Node>();
    defaultNode->id = "node_1";
    defaultNode->type = NodeType::Node;
    defaultNode->name = "Node";
    currentProject_->nodeTree.push_back(defaultNode);

    syncProjectToUI();

    if (!workspacePath_.empty()) {
        std::string path = nativeSaveDialog("untitled.anim", workspacePath_);
        if (!path.empty()) {
            saveAs(path);
            addRecentFile(path);
            fileBrowser_.setRootPath(workspacePath_);
        }
    }

    markClean();
}

void EditorUI::doOpenAnimFile(const std::string& filePath) {
    auto project = Serializer::loadFromFile(filePath);
    if (project) {
        currentProject_ = std::make_shared<AnimProject>(std::move(*project));
        currentFilePath_ = filePath;

        std::string parentDir = std::filesystem::path(filePath).parent_path().string();
        workspacePath_ = parentDir;
        fileBrowser_.setRootPath(parentDir);
        saveWorkspacePath();

        syncProjectToUI();
        markClean();
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
    markClean();
}

void EditorUI::openAnimFile(const std::string& filePath) {
    if (dirty_) {
        pendingAction_ = PendingAction::OpenAnim;
        pendingOpenPath_ = filePath;
        showConfirmDiscard_ = true;
    } else {
        doOpenAnimFile(filePath);
    }
}

void EditorUI::applyAnimationToNode() {
    if (!currentProject_ || currentProject_->animations.empty()) return;
    std::string nodeId = nodeTreePanel_.getSelectedNode();
    if (nodeId.empty()) return;
    auto nodeOpt = sceneGraph_.findById(nodeId);
    if (!nodeOpt) return;
    auto& node = *nodeOpt;

    std::string animName = timelinePanel_.getCurrentAnimationName();
    Animation* anim = nullptr;
    for (auto& a : currentProject_->animations) {
        if (a.name == animName) { anim = &a; break; }
    }
    if (!anim) return;

    float time = timelinePanel_.getCurrentTime();

    for (const auto& track : anim->tracks) {
        if (track.nodeId != nodeId || track.keyframes.empty()) continue;

        // Evaluate interpolated Vec2 or float at current time
        const auto& kfs = track.keyframes;
        Vec2 val2{0.0f, 0.0f};
        float val1 = 0.0f;
        bool isVec2 = std::get_if<Vec2>(&kfs[0].value) != nullptr;

        if (kfs.size() == 1) {
            if (isVec2) val2 = std::get<Vec2>(kfs[0].value);
            else val1 = std::get<float>(kfs[0].value);
        } else {
            float t = std::clamp(time, kfs.front().time, kfs.back().time);
            size_t i = 0;
            for (size_t j = 0; j + 1 < kfs.size(); ++j) {
                if (t >= kfs[j].time && t <= kfs[j + 1].time) { i = j; break; }
            }
            float dt = kfs[i + 1].time - kfs[i].time;
            float frac = dt > 1e-6f ? (t - kfs[i].time) / dt : 0.0f;
            frac = Easing::apply(kfs[i].easing, frac);

            if (isVec2) {
                auto v0 = std::get<Vec2>(kfs[i].value);
                auto v1 = std::get<Vec2>(kfs[i + 1].value);
                val2.x = v0.x + (v1.x - v0.x) * frac;
                val2.y = v0.y + (v1.y - v0.y) * frac;
            } else {
                float v0 = std::get<float>(kfs[i].value);
                float v1 = std::get<float>(kfs[i + 1].value);
                val1 = v0 + (v1 - v0) * frac;
            }
        }

        // Apply to node property
        const auto& prop = track.property;
        if (prop == "position") node->properties.position = val2;
        else if (prop == "scale") node->properties.scale = val2;
        else if (prop == "anchor") node->properties.anchor = val2;
        else if (prop == "rotation") node->properties.rotation = val1;
        else if (prop == "opacity") node->properties.opacity = static_cast<uint8_t>(std::clamp(val1, 0.0f, 255.0f));
        else if (prop == "visible") node->properties.visible = (val1 > 0.5f);
    }
}

} // namespace anim
