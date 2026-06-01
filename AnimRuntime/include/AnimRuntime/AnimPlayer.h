#pragma once
#include "AnimRuntime/AnimData.h"
#include <string>
#include <functional>
#include <map>

namespace anim {

class AnimPlayer {
public:
    static AnimPlayer* create();
    void destroy();

    void load(const AnimProject& data);
    void play(const std::string& animName);
    void pause();
    void stop();
    void seek(float time);
    void update(float dt);

    bool isPlaying() const { return playing_; }
    float getCurrentTime() const { return currentTime_; }
    float getDuration() const { return duration_; }

    void setEventCallback(std::function<void(const std::string&)> cb) { eventCb_ = std::move(cb); }
    void setCompletionCallback(std::function<void()> cb) { completionCb_ = std::move(cb); }

    float getNodePropertyValue(const std::string& nodeId, const std::string& property) const;

private:
    AnimProject data_;
    std::string currentAnim_;
    float currentTime_ = 0.0f;
    float duration_ = 0.0f;
    bool playing_ = false;
    std::function<void(const std::string&)> eventCb_;
    std::function<void()> completionCb_;
    std::map<std::string, float> propertyCache_;

    float evaluateTrack(const Track& track, float time) const;
};

} // namespace anim
