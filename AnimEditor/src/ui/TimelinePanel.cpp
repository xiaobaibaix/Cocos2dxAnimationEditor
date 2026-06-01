#include "ui/TimelinePanel.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace anim {

void TimelinePanel::renderClipSelector(AnimProject* project) {
    if (!project) return;

    ImGui::Text("Clip:");
    ImGui::SameLine();

    if (project->animations.empty()) {
        ImGui::Text("No clips");
        ImGui::SameLine();
        if (ImGui::Button("New")) {
            // Auto-name new clip
            Animation newClip;
            newClip.name = "clip_0";
            newClip.duration = 1.0f;
            newClip.loop = false;
            project->animations.push_back(std::move(newClip));
            currentAnim_ = project->animations.back().name;
            currentTime_ = 0.0f;
            if (onTimeChanged_) {
                onTimeChanged_(0.0f);
            }
        }
        return;
    }

    // Build a list of clip names for the combo
    int currentIndex = -1;
    std::vector<const char*> clipNames;
    for (int i = 0; i < static_cast<int>(project->animations.size()); ++i) {
        clipNames.push_back(project->animations[i].name.c_str());
        if (project->animations[i].name == currentAnim_) {
            currentIndex = i;
        }
    }
    if (currentIndex < 0) currentIndex = 0;

    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("##clipSelector", &currentIndex, clipNames.data(),
                     static_cast<int>(clipNames.size()))) {
        if (currentIndex >= 0 && currentIndex < static_cast<int>(project->animations.size())) {
            currentAnim_ = project->animations[currentIndex].name;
            currentTime_ = 0.0f;
            if (onTimeChanged_) {
                onTimeChanged_(0.0f);
            }
        }
    }

    ImGui::SameLine();

    // New button
    if (ImGui::Button("New")) {
        // Auto-name: clip_0, clip_1, clip_2, ...
        int clipNum = 0;
        std::string candidate;
        bool collision;
        do {
            candidate = "clip_" + std::to_string(clipNum++);
            collision = false;
            for (const auto& a : project->animations) {
                if (a.name == candidate) { collision = true; break; }
            }
        } while (collision);

        Animation newClip;
        newClip.name = candidate;
        newClip.duration = 1.0f;
        newClip.loop = false;
        project->animations.push_back(std::move(newClip));
        currentAnim_ = project->animations.back().name;
        currentTime_ = 0.0f;
        if (onTimeChanged_) {
            onTimeChanged_(0.0f);
        }
    }

    ImGui::SameLine();

    // Del button
    if (ImGui::Button("Del")) {
        if (currentIndex >= 0 && currentIndex < static_cast<int>(project->animations.size())) {
            project->animations.erase(project->animations.begin() + currentIndex);
            if (!project->animations.empty()) {
                currentAnim_ = project->animations.front().name;
            } else {
                currentAnim_ = "";
            }
            currentTime_ = 0.0f;
            if (onTimeChanged_) {
                onTimeChanged_(0.0f);
            }
            renameMode_ = false;
        }
    }

    // Rename support — find the current animation
    if (currentIndex >= 0 && currentIndex < static_cast<int>(project->animations.size())) {
        ImGui::SameLine();
        if (ImGui::Button(renameMode_ ? "OK" : "Rename")) {
            if (renameMode_) {
                // Confirm rename
                if (clipNameBuf_[0] != '\0') {
                    project->animations[currentIndex].name = clipNameBuf_;
                    currentAnim_ = clipNameBuf_;
                }
                renameMode_ = false;
            } else {
                // Enter rename mode
                auto& anim = project->animations[currentIndex];
                std::strncpy(clipNameBuf_, anim.name.c_str(), sizeof(clipNameBuf_) - 1);
                clipNameBuf_[sizeof(clipNameBuf_) - 1] = '\0';
                renameMode_ = true;
            }
        }

        if (renameMode_) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(140.0f);
            bool confirmed = ImGui::InputText("##renameClip", clipNameBuf_, sizeof(clipNameBuf_),
                                              ImGuiInputTextFlags_EnterReturnsTrue);
            if (confirmed && clipNameBuf_[0] != '\0') {
                project->animations[currentIndex].name = clipNameBuf_;
                currentAnim_ = clipNameBuf_;
                renameMode_ = false;
            }
        }
    }
}

void TimelinePanel::render() {
    ImGui::Begin("Timeline");

    // Clip selector at top
    renderClipSelector(project_);

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
