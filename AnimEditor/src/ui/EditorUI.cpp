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

    ImGui::Begin("Properties");
    ImGui::Text("Properties panel (placeholder)");
    ImGui::End();

    ImGui::Begin("Node Tree + Timeline");
    ImGui::Text("Node Tree + Timeline panel (placeholder)");
    ImGui::End();
}

void EditorUI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Folder...")) {
                // TODO: native folder dialog
            }
            if (ImGui::MenuItem("New Animation")) {
                // TODO: create new animation
            }
            if (ImGui::MenuItem("Save")) {
                // TODO: save current project
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
