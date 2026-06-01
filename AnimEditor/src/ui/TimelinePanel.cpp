#include "ui/TimelinePanel.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace anim {

void TimelinePanel::render() {
    ImGui::Begin("Timeline");

    if (!project_ || currentAnim_.empty()) {
        ImGui::TextDisabled("No animation selected. Use File > New Animation or open a project.");
        ImGui::End();
        return;
    }

    // Find the current animation by name
    Animation* anim = nullptr;
    for (auto& a : project_->animations) {
        if (a.name == currentAnim_) {
            anim = &a;
            break;
        }
    }

    if (!anim) {
        ImGui::TextDisabled("Animation \"%s\" not found.", currentAnim_.c_str());
        ImGui::End();
        return;
    }

    renderTransportControls(anim);
    ImGui::Separator();

    // Track area
    for (auto& track : anim->tracks) {
        renderTrackRow(track, anim->duration);
    }

    if (anim->tracks.empty()) {
        ImGui::TextDisabled("No tracks. Select a node and double-click to add keyframes.");
    }

    ImGui::End();
}

void TimelinePanel::renderTransportControls(Animation* anim) {
    // Play / Pause
    if (isPlaying_) {
        if (ImGui::Button("Pause")) {
            isPlaying_ = false;
        }
    } else {
        if (ImGui::Button("Play")) {
            isPlaying_ = true;
        }
    }

    ImGui::SameLine();

    // Stop: reset to beginning
    if (ImGui::Button("Stop")) {
        isPlaying_ = false;
        currentTime_ = 0.0f;
        if (onTimeChanged_) {
            onTimeChanged_(currentTime_);
        }
    }

    ImGui::SameLine();

    // Time display
    char timeLabel[64];
    snprintf(timeLabel, sizeof(timeLabel), "%.2f / %.2f", currentTime_, anim->duration);
    ImGui::Text("%s", timeLabel);

    ImGui::SameLine();

    // Time slider
    float prevTime = currentTime_;
    ImGui::SetNextItemWidth(200.0f);
    ImGui::SliderFloat("##time", &currentTime_, 0.0f, anim->duration, "%.2f s");
    if (currentTime_ != prevTime) {
        if (onTimeChanged_) {
            onTimeChanged_(currentTime_);
        }
    }

    ImGui::SameLine();

    // Zoom control
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("##zoom", &pixelsPerSecond_, 50.0f, 600.0f, "%.0f px/s");
}

void TimelinePanel::renderTrackRow(const Track& track, float duration) {
    bool isSelected = (track.nodeId == selectedNodeId_);

    // Track label with highlight for selected node
    std::string label = track.nodeId + "." + track.property;
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 100, 255));
    }

    ImGui::Text("%s", label.c_str());

    if (isSelected) {
        ImGui::PopStyleColor();
    }

    // Track timeline area as a child region
    std::string childId = "##track_" + track.nodeId + "_" + track.property;
    float trackHeight = 30.0f;
    float totalWidth = duration * pixelsPerSecond_;

    ImGui::BeginChild(childId.c_str(), ImVec2(0, trackHeight), true, ImGuiWindowFlags_NoScrollbar);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float regionHeight = ImGui::GetContentRegionAvail().y;

    // Light background for selected node's track
    if (isSelected) {
        drawList->AddRectFilled(pMin, ImVec2(pMin.x + regionWidth, pMin.y + regionHeight),
                                IM_COL32(60, 55, 30, 80));
    }

    // Draw keyframe diamonds
    float diamondSize = 6.0f;
    for (int i = 0; i < static_cast<int>(track.keyframes.size()); ++i) {
        const auto& kf = track.keyframes[i];
        float x = pMin.x + kf.time * pixelsPerSecond_;
        float y = pMin.y + regionHeight * 0.5f;

        // Diamond shape (two triangles forming a rhombus)
        ImU32 diamondColor = IM_COL32(255, 220, 50, 255);
        drawList->AddTriangleFilled(
            ImVec2(x, y - diamondSize),
            ImVec2(x + diamondSize, y),
            ImVec2(x, y + diamondSize),
            diamondColor);
        drawList->AddTriangleFilled(
            ImVec2(x, y - diamondSize),
            ImVec2(x - diamondSize, y),
            ImVec2(x, y + diamondSize),
            diamondColor);

        // Right-click on keyframe to remove
        ImVec2 kfTopLeft(x - diamondSize, y - diamondSize);
        ImVec2 kfBotRight(x + diamondSize, y + diamondSize);
        ImGui::SetCursorScreenPos(kfTopLeft);
        ImGui::InvisibleButton(("##kf_" + std::to_string(i)).c_str(),
                                ImVec2(diamondSize * 2, diamondSize * 2));

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (onKeyframeRemoved_) {
                onKeyframeRemoved_(track.nodeId, track.property, i);
            }
        }
    }

    // Red time cursor
    float cursorX = pMin.x + currentTime_ * pixelsPerSecond_;
    drawList->AddLine(
        ImVec2(cursorX, pMin.y),
        ImVec2(cursorX, pMin.y + regionHeight),
        IM_COL32(255, 60, 60, 220), 2.0f);

    // Double-click on empty area to add keyframe
    ImGui::SetCursorScreenPos(pMin);
    ImGui::InvisibleButton("##track_hitarea", ImVec2(regionWidth, regionHeight),
                            ImGuiButtonFlags_MouseButtonLeft);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float clickTime = (mousePos.x - pMin.x) / pixelsPerSecond_;
        clickTime = std::clamp(clickTime, 0.0f, duration);

        if (onKeyframeAdded_) {
            onKeyframeAdded_(track.nodeId, track.property);
        }
    }

    ImGui::EndChild();
}

} // namespace anim
