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

    renderTimeRuler(anim->duration);

    // Track area — existing tracks
    for (auto& track : anim->tracks) {
        renderTrackRow(track, anim->duration);
    }

    // Potential tracks for selected node — properties without a track yet
    if (project_ && !selectedNodeId_.empty()) {
        for (const auto& prop : getPropertyNames(selectedNodeId_)) {
            if (!hasTrackFor(anim, selectedNodeId_, prop)) {
                renderPotentialTrack(selectedNodeId_, prop, anim->duration);
            }
        }
    }

    if (anim->tracks.empty() && selectedNodeId_.empty()) {
        ImGui::TextDisabled("No tracks. Select a node in the Nodes panel to see its properties here.");
    } else if (anim->tracks.empty() && !selectedNodeId_.empty()) {
        ImGui::TextDisabled("Double-click a property row below to add the first keyframe.");
    }

    ImGui::End();
}

void TimelinePanel::renderTimeRuler(float duration) {
    float rulerHeight = 24.0f;

    ImGui::BeginChild("##timeRuler", ImVec2(0, rulerHeight), true, ImGuiWindowFlags_NoScrollbar);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float regionHeight = ImGui::GetContentRegionAvail().y;

    drawList->AddRectFilled(pMin, ImVec2(pMin.x + regionWidth, pMin.y + regionHeight),
                            IM_COL32(35, 35, 45, 255));

    // Adaptive tick spacing based on zoom level
    float tickStep = 0.1f;
    while (tickStep * pixelsPerSecond_ < 50.0f) tickStep *= 2.0f;
    while (tickStep * pixelsPerSecond_ > 150.0f) tickStep /= 2.0f;

    for (float t = 0.0f; t <= duration + 0.0001f; t += tickStep) {
        float x = pMin.x + t * pixelsPerSecond_;
        if (x > pMin.x + regionWidth) break;

        bool isMajor = (static_cast<int>(t / tickStep + 0.5f) % 5 == 0);
        float tickTop = isMajor ? pMin.y + 2.0f : pMin.y + regionHeight * 0.5f;

        drawList->AddLine(
            ImVec2(x, tickTop),
            ImVec2(x, pMin.y + regionHeight),
            IM_COL32(150, 150, 160, 200));

        if (isMajor) {
            char label[32];
            snprintf(label, sizeof(label), "%.1fs", t);
            drawList->AddText(ImVec2(x + 3, pMin.y + 2), IM_COL32(180, 180, 190, 255), label);
        }
    }

    // Red time cursor
    float cursorX = pMin.x + currentTime_ * pixelsPerSecond_;
    drawList->AddLine(
        ImVec2(cursorX, pMin.y),
        ImVec2(cursorX, pMin.y + regionHeight),
        IM_COL32(255, 60, 60, 220), 2.0f);

    ImGui::EndChild();
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

    // Hit area for adding keyframes by double-click (before keyframe buttons so they take priority)
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

    ImGui::EndChild();
}

void TimelinePanel::renderPotentialTrack(const std::string& nodeId,
                                           const std::string& property,
                                           float duration) {
    // Dimmed label for potential track
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 120, 120, 180));
    ImGui::Text("+ %s.%s", nodeId.c_str(), property.c_str());
    ImGui::PopStyleColor();

    // Invisible hit area covering the row for double-click
    std::string childId = "##pot_" + nodeId + "_" + property;
    float trackHeight = 30.0f;

    ImGui::BeginChild(childId.c_str(), ImVec2(0, trackHeight), true,
                      ImGuiWindowFlags_NoScrollbar);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    float regionWidth = ImGui::GetContentRegionAvail().x;
    float regionHeight = ImGui::GetContentRegionAvail().y;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(pMin, ImVec2(pMin.x + regionWidth, pMin.y + regionHeight),
                            IM_COL32(40, 40, 50, 60));

    // Double-click to create first keyframe
    ImGui::SetCursorScreenPos(pMin);
    ImGui::InvisibleButton("##pot_hit", ImVec2(regionWidth, regionHeight),
                            ImGuiButtonFlags_MouseButtonLeft);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (onKeyframeAdded_) {
            onKeyframeAdded_(nodeId, property);
        }
    }

    ImGui::EndChild();
}

bool TimelinePanel::hasTrackFor(const Animation* anim, const std::string& nodeId,
                                const std::string& property) const {
    if (!anim) return false;
    for (const auto& t : anim->tracks) {
        if (t.nodeId == nodeId && t.property == property) return true;
    }
    return false;
}

std::vector<std::string> TimelinePanel::getPropertyNames(const std::string& /*nodeId*/) const {
    // Return the common animatable properties for any node.
    // In the future this could vary by NodeType (e.g. Label nodes have fontSize).
    return {
        "position.x", "position.y",
        "scale.x", "scale.y",
        "rotation",
        "opacity",
        "anchor.x", "anchor.y",
        "color.r", "color.g", "color.b",
        "visible"
    };
}

} // namespace anim
