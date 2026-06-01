#include "ui/PreviewCanvas.h"
#include "imgui.h"

namespace anim {

bool PreviewCanvas::init(int width, int height) {
    return embed_.init(width, height);
}

void PreviewCanvas::shutdown() {
    embed_.shutdown();
}

void PreviewCanvas::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Preview");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 1.0f && avail.y > 1.0f) {
        embed_.resize(static_cast<int>(avail.x), static_cast<int>(avail.y));
        embed_.renderFrame();

        // ImGui textures are flipped vertically relative to OpenGL.
        // UV (0,1)->(1,0) flips the image to correct orientation.
        // ImTextureID is ImU64 in ImGui 1.91.4+; cast via intptr_t as documented in FAQ.
        ImGui::Image(
            static_cast<ImU64>(embed_.getTextureId()),
            avail,
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));

        hovered_ = ImGui::IsItemHovered();

        // Drag detection on the preview image
        if (hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            dragging_ = true;
        }
    } else {
        hovered_ = false;
    }

    // Track drag delta while mouse button is held
    if (dragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        if ((delta.x != 0.0f || delta.y != 0.0f) && onNodeDragged_) {
            onNodeDragged_(delta.x, delta.y);
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dragging_ = false;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace anim
