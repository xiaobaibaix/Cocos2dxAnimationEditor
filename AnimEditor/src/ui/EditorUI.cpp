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
    });

    propertyPanel_.setOnPropertyChanged([](const std::string& /*nodeId*/) {
        // Property changed notification — can trigger auto-save or undo registration later
    });

    sceneGraph_.setOnChanged([this]() {
        // Scene graph changed — can trigger auto-save or dirty flag later
    });

    return true;
}

void EditorUI::shutdown() {
    // Nothing to clean up yet
}

void EditorUI::render() {
    renderMenuBar();
    fileBrowser_.render();

    ImGui::Begin("Preview");
    ImGui::Text("Preview panel (placeholder)");
    ImGui::End();

    // Node Tree + Timeline bottom panel
    ImGui::Begin("Node Tree + Timeline");
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float nodeTreeWidth = panelWidth * 0.35f;

    // Left side: Node Tree
    ImGui::BeginChild("NodeTreeSide", ImVec2(nodeTreeWidth, 0), true);
    nodeTreePanel_.render();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right side: placeholder for Timeline (Phase 3)
    ImGui::BeginChild("TimelineSide", ImVec2(0, 0), true);
    ImGui::TextDisabled("Timeline (Phase 3)");
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
