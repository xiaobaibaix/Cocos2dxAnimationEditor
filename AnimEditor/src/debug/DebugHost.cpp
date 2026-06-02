#include "debug/DebugHost.h"
#include "imgui.h"

namespace anim {

void DebugHost::registerPanel(std::unique_ptr<DebugPanelBase> panel) {
    panels_.push_back(std::move(panel));
}

void DebugHost::renderMenu() {
    if (panels_.empty()) return;

    if (ImGui::BeginMenu("Debug")) {
        for (auto& panel : panels_) {
            ImGui::MenuItem(panel->name(), nullptr, &panel->open);
        }
        ImGui::EndMenu();
    }
}

void DebugHost::renderPanels() {
    for (auto& panel : panels_) {
        if (!panel->open) continue;
        if (ImGui::Begin(panel->name(), &panel->open)) {
            panel->render();
        }
        ImGui::End();
    }
}

} // namespace anim
