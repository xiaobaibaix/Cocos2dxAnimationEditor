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
    using OnKeyframeChanged = std::function<void(const std::string& nodeId, const std::string& property,
                                                  int index, float newTime)>;
    using OnKeyframeValueChanged = std::function<void(const std::string& nodeId, const std::string& property,
                                                       int index, const Vec2& newValue)>;

    void setProject(AnimProject* project) { project_ = project; }
    void setCurrentAnimation(const std::string& name) { currentAnim_ = name; }
    void setCurrentTime(float t) { currentTime_ = t; }
    void setSelectedNode(const std::string& id) { selectedNodeId_ = id; }

    void setOnTimeChanged(OnTimeChanged cb) { onTimeChanged_ = std::move(cb); }
    void setOnKeyframeAdded(OnKeyframeAdded cb) { onKeyframeAdded_ = std::move(cb); }
    void setOnKeyframeRemoved(OnKeyframeRemoved cb) { onKeyframeRemoved_ = std::move(cb); }
    void setOnKeyframeChanged(OnKeyframeChanged cb) { onKeyframeChanged_ = std::move(cb); }
    void setOnKeyframeValueChanged(OnKeyframeValueChanged cb) { onKeyframeValueChanged_ = std::move(cb); }

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
    OnKeyframeChanged onKeyframeChanged_;
    OnKeyframeValueChanged onKeyframeValueChanged_;

    // Drag state
    bool draggingKeyframe_ = false;
    std::string dragTrackNodeId_;
    std::string dragTrackProperty_;
    int dragKeyframeIndex_ = -1;
    float dragStartTime_ = 0.0f;

    // Context menu state
    int contextMenuType_ = 0;
    std::string contextNodeId_;
    std::string contextProperty_;
    int contextKeyframeIndex_ = -1;
    float contextClickTime_ = 0.0f;

    // Keyframe edit popup state
    std::string editNodeId_;
    std::string editProperty_;
    int editKeyframeIndex_ = -1;
    float editKeyframeTime_ = 0.0f;
    Vec2 editKeyframeVec2_;

    char clipNameBuf_[256] = {};
    bool renameMode_ = false;

    void renderTimeRuler(float duration, float contentWidth);
    void renderTransportControls(Animation* anim);
    void renderTrackRow(Track& track, float duration, float contentWidth);
    bool hasTrackFor(const Animation* anim, const std::string& nodeId,
                     const std::string& property) const;
    bool hasTrackGroup(const Animation* anim, const std::string& nodeId,
                       const std::string& group) const;
    static std::vector<std::string> subPropertiesForGroup(const std::string& group);
    std::vector<std::string> getPropertyNames(const std::string& nodeId) const;
};

} // namespace anim
