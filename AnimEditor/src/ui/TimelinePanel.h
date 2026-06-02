#pragma once
#include "core/AnimData.h"
#include <functional>
#include <string>

namespace anim {

class TimelinePanel {
public:
    using OnTimeChanged = std::function<void(float time)>;
    using OnKeyframeAdded = std::function<void(const std::string& nodeId, const std::string& property)>;
    using OnKeyframeRemoved = std::function<void(const std::string& nodeId, const std::string& property, int index)>;

    void setProject(AnimProject* project) { project_ = project; }
    void setCurrentAnimation(const std::string& name) { currentAnim_ = name; }
    void setCurrentTime(float t) { currentTime_ = t; }
    void setSelectedNode(const std::string& id) { selectedNodeId_ = id; }

    void setOnTimeChanged(OnTimeChanged cb) { onTimeChanged_ = std::move(cb); }
    void setOnKeyframeAdded(OnKeyframeAdded cb) { onKeyframeAdded_ = std::move(cb); }
    void setOnKeyframeRemoved(OnKeyframeRemoved cb) { onKeyframeRemoved_ = std::move(cb); }

    float getCurrentTime() const { return currentTime_; }
    const std::string& getCurrentAnimationName() const { return currentAnim_; }
    bool isPlaying() const { return isPlaying_; }
    void render();
    void renderClipSelector(AnimProject* project);

private:
    AnimProject* project_ = nullptr;
    std::string currentAnim_;
    std::string selectedNodeId_;
    float currentTime_ = 0.0f;
    bool isPlaying_ = false;
    float pixelsPerSecond_ = 200.0f;

    OnTimeChanged onTimeChanged_;
    OnKeyframeAdded onKeyframeAdded_;
    OnKeyframeRemoved onKeyframeRemoved_;

    char clipNameBuf_[256] = {};
    bool renameMode_ = false;

    void renderTimeRuler(float duration);
    void renderTransportControls(Animation* anim);
    void renderTrackRow(const Track& track, float duration);
    void renderPotentialTrack(const std::string& nodeId, const std::string& property,
                              float duration);
    bool hasTrackFor(const Animation* anim, const std::string& nodeId,
                     const std::string& property) const;
    std::vector<std::string> getPropertyNames(const std::string& nodeId) const;
};

} // namespace anim
