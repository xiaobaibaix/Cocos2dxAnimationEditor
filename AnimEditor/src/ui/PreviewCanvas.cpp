#include "ui/PreviewCanvas.h"
#include "imgui.h"
#include <cstdio>

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

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();

        ImGui::Image(
            static_cast<ImU64>(embed_.getTextureId()),
            avail,
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));

        ImVec2 mousePos = ImGui::GetIO().MousePos;
        hovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
                   && mousePos.x >= canvasPos.x
                   && mousePos.x <= canvasPos.x + avail.x
                   && mousePos.y >= canvasPos.y
                   && mousePos.y <= canvasPos.y + avail.y;

        if (hovered_) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                float factor = wheel > 0.0f ? 1.1f : 1.0f / 1.1f;
                float oldZoom = zoom_;
                float newZoom = zoom_ * factor;
                newZoom = newZoom < 0.1f ? 0.1f : newZoom > 10.0f ? 10.0f : newZoom;
                float ratio = newZoom / oldZoom;

                float mx = mousePos.x - canvasPos.x;
                float halfX = avail.x * 0.5f;
                panX_ = mx - halfX - (mx - halfX - panX_) * ratio;

                float my = mousePos.y - canvasPos.y;
                float halfY = avail.y * 0.5f;
                panY_ = my - halfY - (my - halfY - panY_) * ratio;

                zoom_ = newZoom;
            }
        }

        if (hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            dragging_ = true;
        }
        if (hovered_ && (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)
                         || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            panning_ = true;
        }

        embed_.setViewTransform(zoom_, panX_, panY_);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (zoom_ != 1.0f || panning_ || hovered_) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Zoom: %.0f%%  Pan: (%.0f, %.0f)",
                          zoom_ * 100.0f, panX_, panY_);
            dl->AddText(ImVec2(canvasPos.x + 8.0f, canvasPos.y + 8.0f),
                        IM_COL32(200, 200, 200, 180), buf);
        }
    } else {
        hovered_ = false;
    }

    if (dragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        if (delta.x != 0.0f || delta.y != 0.0f) {
            panX_ += delta.x;
            panY_ += delta.y;
            if (onNodeDragged_) {
                onNodeDragged_(delta.x, delta.y);
            }
        }
    }

    if (panning_ && (ImGui::IsMouseDown(ImGuiMouseButton_Middle)
                     || ImGui::IsMouseDown(ImGuiMouseButton_Right))) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        panX_ += delta.x;
        panY_ += delta.y;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dragging_ = false;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)
        && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        panning_ = false;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace anim
