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

    if (ImGui::Button("New")) {
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

    if (currentIndex >= 0 && currentIndex < static_cast<int>(project->animations.size())) {
        ImGui::SameLine();
        if (ImGui::Button(renameMode_ ? "OK" : "Rename")) {
            if (renameMode_) {
                if (clipNameBuf_[0] != '\0') {
                    project->animations[currentIndex].name = clipNameBuf_;
                    currentAnim_ = clipNameBuf_;
                }
                renameMode_ = false;
            } else {
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

    renderClipSelector(project_);

    if (!project_ || currentAnim_.empty()) {
        ImGui::TextDisabled("No animation selected. Use File > New Animation or open a project.");
        ImGui::End();
        return;
    }

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

    // Space bar toggles play/pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space) && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
        isPlaying_ = !isPlaying_;
        if (isPlaying_ && currentTime_ >= anim->duration) {
            currentTime_ = 0.0f;
        }
    }

    if (isPlaying_) {
        currentTime_ += ImGui::GetIO().DeltaTime;
        if (currentTime_ >= anim->duration) {
            if (anim->loop) {
                currentTime_ = std::fmod(currentTime_, anim->duration);
            } else {
                currentTime_ = anim->duration;
                isPlaying_ = false;
            }
        }
        if (onTimeChanged_) onTimeChanged_(currentTime_);
    }

    renderTransportControls(anim);
    ImGui::Separator();

    // Content width for the full timeline
    float contentWidth = anim->duration * pixelsPerSecond_ + 20.0f;

    // ── Single outer scroll area for ruler + all track rows ──
    ImGui::BeginChild("##trackScrollArea", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Horizontal scroll with Shift+wheel or horizontal wheel
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        float wheelH = ImGui::GetIO().MouseWheelH;
        float wheelV = ImGui::GetIO().MouseWheel;
        if (wheelH != 0.0f || (wheelV != 0.0f && ImGui::GetIO().KeyShift)) {
            float scrollX = ImGui::GetScrollX();
            scrollX -= (wheelH + (ImGui::GetIO().KeyShift ? wheelV : 0.0f)) * 50.0f;
            if (scrollX < 0.0f) scrollX = 0.0f;
            float maxScroll = std::max(0.0f, contentWidth - ImGui::GetWindowWidth());
            if (scrollX > maxScroll) scrollX = maxScroll;
            ImGui::SetScrollX(scrollX);
        }
    }

    renderTimeRuler(anim->duration, contentWidth);

    for (auto& track : anim->tracks) {
        if (!selectedNodeId_.empty() && track.nodeId != selectedNodeId_) continue;
        renderTrackRow(track, anim->duration, contentWidth);
    }

    // Spacer row for right-click → add property
    {
        float rowHeight = 24.0f;
        ImGui::BeginChild("##addPropRow", ImVec2(contentWidth, rowHeight), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImVec2 apMin = ImGui::GetCursorScreenPos();
        ImDrawList* apDraw = ImGui::GetWindowDrawList();
        apDraw->AddRectFilled(apMin, ImVec2(apMin.x + contentWidth, apMin.y + rowHeight),
                              IM_COL32(30, 30, 35, 255));

        ImGui::SetCursorScreenPos(apMin);
        ImGui::InvisibleButton("##addPropBtn", ImVec2(contentWidth, rowHeight));

        if (ImGui::IsItemHovered()) {
            apDraw->AddRectFilled(apMin, ImVec2(apMin.x + contentWidth, apMin.y + rowHeight),
                                  IM_COL32(50, 50, 55, 128));
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("##AddPropertyMenu");
            contextMenuType_ = 3;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 120, 130, 200));
        if (!selectedNodeId_.empty()) {
            ImGui::SetCursorScreenPos(ImVec2(apMin.x + 6, apMin.y + 4));
            ImGui::TextUnformatted("Right-click here to add properties...");
        } else {
            ImGui::SetCursorScreenPos(ImVec2(apMin.x + 6, apMin.y + 4));
            ImGui::TextUnformatted("Select a node, then right-click here to add properties.");
        }
        ImGui::PopStyleColor();

        // Property list popup
        if (ImGui::BeginPopup("##AddPropertyMenu")) {
            if (!selectedNodeId_.empty()) {
                ImGui::TextDisabled("Add property for %s", selectedNodeId_.c_str());
                ImGui::Separator();
                bool anyAvailable = false;
                for (const auto& group : getPropertyNames(selectedNodeId_)) {
                    if (!hasTrackGroup(anim, selectedNodeId_, group)) {
                        auto subs = subPropertiesForGroup(group);
                        if (subs.empty()) {
                            // Standalone property
                            if (!hasTrackFor(anim, selectedNodeId_, group)) {
                                anyAvailable = true;
                                if (ImGui::MenuItem(group.c_str())) {
                                    if (onKeyframeAdded_) {
                                        onKeyframeAdded_(selectedNodeId_, group, currentTime_);
                                    }
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        } else {
                            // Grouped property — expand to sub-properties
                            anyAvailable = true;
                            if (ImGui::MenuItem(group.c_str())) {
                                for (const auto& sub : subs) {
                                    std::string prop = group + "." + sub;
                                    if (onKeyframeAdded_) {
                                        onKeyframeAdded_(selectedNodeId_, prop, currentTime_);
                                    }
                                }
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
                if (!anyAvailable) {
                    ImGui::TextDisabled("All properties already added");
                }
            } else {
                ImGui::TextDisabled("Select a node first");
            }
            ImGui::EndPopup();
        }
        ImGui::EndChild();
    }

    if (selectedNodeId_.empty()) {
        ImGui::TextDisabled("Select a node in the Nodes panel to edit its animation.");
    } else if (anim->tracks.empty()) {
        ImGui::TextDisabled("No tracks for this node. Right-click below to add properties.");
    } else {
        ImGui::TextDisabled("Right-click the row above to add an animatable property.");
    }

    ImGui::EndChild(); // ##trackScrollArea
    ImGui::End(); // Timeline
}

void TimelinePanel::renderTimeRuler(float duration, float contentWidth) {
    float rulerHeight = 24.0f;

    ImGui::BeginChild("##timeRuler", ImVec2(contentWidth, rulerHeight), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float regionWidth = contentWidth;
    float regionHeight = ImGui::GetContentRegionAvail().y;

    drawList->AddRectFilled(pMin, ImVec2(pMin.x + regionWidth, pMin.y + regionHeight),
                            IM_COL32(35, 35, 45, 255));

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

    // Click/drag to seek
    ImGui::SetCursorScreenPos(pMin);
    ImGui::InvisibleButton("##rulerSeek", ImVec2(contentWidth, regionHeight));
    if (ImGui::IsItemActive()) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float newTime = (mouseX - pMin.x) / pixelsPerSecond_;
        newTime = std::clamp(newTime, 0.0f, duration);
        currentTime_ = newTime;
        if (onTimeChanged_) onTimeChanged_(currentTime_);
    }

    ImGui::EndChild();
}

void TimelinePanel::renderTransportControls(Animation* anim) {
    if (isPlaying_) {
        if (ImGui::Button("Pause")) {
            isPlaying_ = false;
        }
    } else {
        if (ImGui::Button("Play")) {
            if (currentTime_ >= anim->duration) {
                currentTime_ = 0.0f;
            }
            isPlaying_ = true;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop")) {
        isPlaying_ = false;
        currentTime_ = 0.0f;
        if (onTimeChanged_) {
            onTimeChanged_(currentTime_);
        }
    }

    ImGui::SameLine();

    bool wasLoop = anim->loop;
    if (wasLoop) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 0.6f));
    }
    if (ImGui::Button(wasLoop ? "Loop ON" : "Loop OFF")) {
        anim->loop = !anim->loop;
    }
    if (wasLoop) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    ImGui::Text("%.2f /", currentTime_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    float prevDuration = anim->duration;
    if (ImGui::InputFloat("##duration", &anim->duration, 0.1f, 1.0f, "%.2f")) {
        if (anim->duration < 0.1f) anim->duration = 0.1f;
        if (currentTime_ > anim->duration) {
            currentTime_ = anim->duration;
            if (onTimeChanged_) onTimeChanged_(currentTime_);
        }
    }

    ImGui::SameLine();

    float prevTime = currentTime_;
    float sliderWidth = ImGui::GetContentRegionAvail().x - 200.0f;
    if (sliderWidth < 80.0f) sliderWidth = 80.0f;
    ImGui::SetNextItemWidth(sliderWidth);
    ImGui::SliderFloat("##time", &currentTime_, 0.0f, anim->duration, "%.2f s");
    if (currentTime_ != prevTime) {
        if (onTimeChanged_) {
            onTimeChanged_(currentTime_);
        }
    }

    ImGui::SameLine();

    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("##zoom", &pixelsPerSecond_, 50.0f, 600.0f, "%.0f px/s");
}

void TimelinePanel::renderTrackRow(Track& track, float duration, float contentWidth) {
    bool isSelected = (track.nodeId == selectedNodeId_);

    std::string label = track.nodeId + "." + track.property;
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 100, 255));
    }

    ImGui::Text("%s", label.c_str());

    if (isSelected) {
        ImGui::PopStyleColor();
    }

    std::string childId = "##track_" + track.nodeId + "_" + track.property;
    float trackHeight = 30.0f;

    ImGui::BeginChild(childId.c_str(), ImVec2(contentWidth, trackHeight), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float regionWidth = contentWidth;
    float regionHeight = ImGui::GetContentRegionAvail().y;

    if (isSelected) {
        drawList->AddRectFilled(pMin, ImVec2(pMin.x + regionWidth, pMin.y + regionHeight),
                                IM_COL32(60, 55, 30, 80));
    }

    // Invisible button covering the full track area
    std::string hitId = "##track_hit_" + track.nodeId + "_" + track.property;
    ImGui::SetCursorScreenPos(pMin);
    ImGui::InvisibleButton(hitId.c_str(), ImVec2(regionWidth, regionHeight));

    // Track background right-click → open context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        contextClickTime_ = std::clamp((mousePos.x - pMin.x) / pixelsPerSecond_, 0.0f, duration);
        contextMenuType_ = 1;
        contextNodeId_ = track.nodeId;
        contextProperty_ = track.property;
        ImGui::OpenPopup("##TrackCtxMenu");
    }

    // Track context menu popup
    if (ImGui::BeginPopup("##TrackCtxMenu")) {
        if (contextMenuType_ == 1 &&
            contextNodeId_ == track.nodeId &&
            contextProperty_ == track.property) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Add Keyframe at %.2fs", contextClickTime_);
            if (ImGui::MenuItem(buf)) {
                if (onKeyframeAdded_) {
                    onKeyframeAdded_(track.nodeId, track.property, contextClickTime_);
                }
                ImGui::CloseCurrentPopup();
            }
            if (!track.keyframes.empty()) {
                ImGui::Separator();
                if (ImGui::MenuItem("Remove Track")) {
                    for (int i = static_cast<int>(track.keyframes.size()) - 1; i >= 0; --i) {
                        if (onKeyframeRemoved_) {
                            onKeyframeRemoved_(track.nodeId, track.property, i);
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }

    // Draw connection lines between consecutive keyframes
    for (int i = 0; i + 1 < static_cast<int>(track.keyframes.size()); ++i) {
        float x0 = pMin.x + track.keyframes[i].time * pixelsPerSecond_;
        float y0 = pMin.y + regionHeight * 0.5f;
        float x1 = pMin.x + track.keyframes[i + 1].time * pixelsPerSecond_;
        float y1 = pMin.y + regionHeight * 0.5f;
        drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1),
                          IM_COL32(255, 220, 50, 120), 1.5f);
    }

    // Draw keyframe diamonds
    float baseDiamondSize = 6.0f;
    float hitRadius = 12.0f;
    ImVec2 mousePos = ImGui::GetIO().MousePos;

    for (int i = 0; i < static_cast<int>(track.keyframes.size()); ++i) {
        const auto& kf = track.keyframes[i];
        float x = pMin.x + kf.time * pixelsPerSecond_;
        float y = pMin.y + regionHeight * 0.5f;

        bool isHovered = !draggingKeyframe_ &&
                         std::fabs(mousePos.x - x) <= hitRadius &&
                         std::fabs(mousePos.y - y) <= hitRadius;

        // Draw diamond — enlarge on hover
        float ds = isHovered ? baseDiamondSize * 1.5f : baseDiamondSize;
        ImU32 diamondColor = isHovered ? IM_COL32(255, 240, 80, 255) : IM_COL32(255, 220, 50, 255);
        drawList->AddTriangleFilled(
            ImVec2(x, y - ds),
            ImVec2(x + ds, y),
            ImVec2(x, y + ds),
            diamondColor);
        drawList->AddTriangleFilled(
            ImVec2(x, y - ds),
            ImVec2(x - ds, y),
            ImVec2(x, y + ds),
            diamondColor);

        if (isHovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        // Right-click → context menu
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            contextMenuType_ = 2;
            contextNodeId_ = track.nodeId;
            contextProperty_ = track.property;
            contextKeyframeIndex_ = i;
            ImGui::OpenPopup("##KeyframeCtxMenu");
        }

        // Left-click → start drag
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            draggingKeyframe_ = true;
            dragTrackNodeId_ = track.nodeId;
            dragTrackProperty_ = track.property;
            dragKeyframeIndex_ = i;
            dragStartTime_ = kf.time;
        }
    }

    // Keyframe context menu popup
    if (ImGui::BeginPopup("##KeyframeCtxMenu")) {
        if (contextMenuType_ == 2 &&
            contextNodeId_ == track.nodeId &&
            contextProperty_ == track.property &&
            contextKeyframeIndex_ >= 0 &&
            contextKeyframeIndex_ < static_cast<int>(track.keyframes.size())) {
            const auto& kf = track.keyframes[contextKeyframeIndex_];
            char buf[64];
            snprintf(buf, sizeof(buf), "Keyframe at %.2fs", kf.time);
            ImGui::TextDisabled("%s", buf);
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Keyframe")) {
                if (onKeyframeRemoved_) {
                    onKeyframeRemoved_(track.nodeId, track.property, contextKeyframeIndex_);
                }
                contextKeyframeIndex_ = -1;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    // Keyframe drag update
    if (draggingKeyframe_ &&
        dragTrackNodeId_ == track.nodeId &&
        dragTrackProperty_ == track.property) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float deltaX = mousePos.x - (pMin.x + dragStartTime_ * pixelsPerSecond_);
            float newTime = dragStartTime_ + deltaX / pixelsPerSecond_;
            newTime = std::clamp(newTime, 0.0f, duration);
            track.keyframes[dragKeyframeIndex_].time = newTime;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (onKeyframeChanged_ &&
                track.keyframes[dragKeyframeIndex_].time != dragStartTime_) {
                onKeyframeChanged_(dragTrackNodeId_, dragTrackProperty_,
                                   dragKeyframeIndex_,
                                   track.keyframes[dragKeyframeIndex_].time);
            }
            draggingKeyframe_ = false;
            dragKeyframeIndex_ = -1;
        }
    }

    // Red time cursor
    float cursorX = pMin.x + currentTime_ * pixelsPerSecond_;
    drawList->AddLine(
        ImVec2(cursorX, pMin.y),
        ImVec2(cursorX, pMin.y + regionHeight),
        IM_COL32(255, 60, 60, 220), 2.0f);

    // ── Double-click hit-test for keyframe value editing ──
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float hitRadius = 10.0f;
        for (int i = 0; i < static_cast<int>(track.keyframes.size()); ++i) {
            float kfX = pMin.x + track.keyframes[i].time * pixelsPerSecond_;
            float kfY = pMin.y + regionHeight * 0.5f;
            if (std::fabs(mousePos.x - kfX) <= hitRadius &&
                std::fabs(mousePos.y - kfY) <= hitRadius) {
                editNodeId_ = track.nodeId;
                editProperty_ = track.property;
                editKeyframeIndex_ = i;
                editKeyframeTime_ = track.keyframes[i].time;
                if (auto* v = std::get_if<Vec2>(&track.keyframes[i].value)) {
                    editKeyframeVec2_ = *v;
                } else if (auto* v = std::get_if<float>(&track.keyframes[i].value)) {
                    editKeyframeVec2_ = Vec2{*v, 0.0f};
                } else {
                    editKeyframeVec2_ = Vec2{};
                }
                ImGui::OpenPopup("##KeyframeEditPopup");
                break;
            }
        }
    }

    // ── Keyframe value edit popup ──
    if (ImGui::BeginPopup("##KeyframeEditPopup")) {
        if (editNodeId_ == track.nodeId && editProperty_ == track.property &&
            editKeyframeIndex_ >= 0 &&
            editKeyframeIndex_ < static_cast<int>(track.keyframes.size())) {
            ImGui::Text("Edit Keyframe — %s.%s", track.nodeId.c_str(), track.property.c_str());
            ImGui::Separator();
            ImGui::InputFloat("Time", &editKeyframeTime_, 0.05f, 0.5f, "%.3f s");
            bool isVec2 = std::get_if<Vec2>(&track.keyframes[editKeyframeIndex_].value) != nullptr;
            if (isVec2) {
                ImGui::InputFloat("X", &editKeyframeVec2_.x, 0.1f, 1.0f, "%.3f");
                ImGui::InputFloat("Y", &editKeyframeVec2_.y, 0.1f, 1.0f, "%.3f");
            } else {
                float fval = editKeyframeVec2_.x;
                ImGui::InputFloat("Value", &fval, 0.1f, 1.0f, "%.3f");
                editKeyframeVec2_.x = fval;
            }
            ImGui::Separator();
            if (ImGui::Button("OK")) {
                auto& kf = track.keyframes[editKeyframeIndex_];
                kf.time = std::clamp(editKeyframeTime_, 0.0f, duration);
                if (isVec2) {
                    kf.value = editKeyframeVec2_;
                } else {
                    kf.value = editKeyframeVec2_.x;
                }
                if (onKeyframeValueChanged_) {
                    onKeyframeValueChanged_(editNodeId_, editProperty_,
                                            editKeyframeIndex_, editKeyframeVec2_);
                }
                editKeyframeIndex_ = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                editKeyframeIndex_ = -1;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
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

bool TimelinePanel::hasTrackGroup(const Animation* anim, const std::string& nodeId,
                                   const std::string& group) const {
    if (!anim) return false;
    auto subs = subPropertiesForGroup(group);
    if (subs.empty()) {
        return hasTrackFor(anim, nodeId, group);
    }
    for (const auto& sub : subs) {
        std::string prop = group + "." + sub;
        if (!hasTrackFor(anim, nodeId, prop)) return false;
    }
    return true;
}

std::vector<std::string> TimelinePanel::subPropertiesForGroup(const std::string& group) {
    // Position, scale, anchor are single Vec2 tracks — no sub-properties
    if (group == "color")
        return {"r", "g", "b"};
    return {};
}

std::vector<std::string> TimelinePanel::getPropertyNames(const std::string& /*nodeId*/) const {
    return {"position", "scale", "rotation", "opacity", "anchor", "color", "visible"};
}

} // namespace anim
