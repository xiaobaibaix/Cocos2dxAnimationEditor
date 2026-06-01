#include "AnimRuntime/AnimPlayer.h"
#include "AnimRuntime/Easing.h"
#include <cmath>
#include <algorithm>

namespace anim {

AnimPlayer* AnimPlayer::create() {
    return new AnimPlayer();
}

void AnimPlayer::destroy() {
    delete this;
}

void AnimPlayer::load(const AnimProject& data) {
    data_ = data;
    propertyCache_.clear();
    currentAnim_.clear();
    currentTime_ = 0.0f;
    duration_ = 0.0f;
    playing_ = false;
}

void AnimPlayer::play(const std::string& animName) {
    currentAnim_ = animName;
    currentTime_ = 0.0f;
    playing_ = true;
    propertyCache_.clear();

    // Find the animation duration
    for (const auto& anim : data_.animations) {
        if (anim.name == animName) {
            duration_ = anim.duration;
            break;
        }
    }
}

void AnimPlayer::pause() {
    playing_ = false;
}

void AnimPlayer::stop() {
    playing_ = false;
    currentTime_ = 0.0f;
    propertyCache_.clear();
}

void AnimPlayer::seek(float time) {
    currentTime_ = time;
    // Recalculate all property values at the new time
    propertyCache_.clear();
    for (const auto& anim : data_.animations) {
        if (anim.name != currentAnim_) continue;
        for (const auto& track : anim.tracks) {
            const std::string key = track.nodeId + ":" + track.property;
            propertyCache_[key] = evaluateTrack(track, currentTime_);
        }
        break;
    }
}

void AnimPlayer::update(float dt) {
    if (!playing_ || currentAnim_.empty()) return;

    // Find the current animation
    const Animation* currentAnim = nullptr;
    for (const auto& anim : data_.animations) {
        if (anim.name == currentAnim_) {
            currentAnim = &anim;
            break;
        }
    }
    if (!currentAnim) {
        playing_ = false;
        return;
    }

    const float prevTime = currentTime_;
    currentTime_ += dt;

    // Check for animation completion
    if (currentTime_ >= duration_) {
        if (currentAnim->loop) {
            currentTime_ = std::fmod(currentTime_, duration_);
        } else {
            currentTime_ = duration_;
            playing_ = false;
        }
    }

    // Evaluate all tracks
    propertyCache_.clear();
    for (const auto& track : currentAnim->tracks) {
        const std::string key = track.nodeId + ":" + track.property;
        propertyCache_[key] = evaluateTrack(track, currentTime_);
    }

    // Check for events in the traversed time range
    if (eventCb_) {
        for (const auto& event : data_.events) {
            if (event.time > prevTime && event.time <= currentTime_) {
                eventCb_(event.name);
            }
            // Handle loop wrap-around: if looped, also check from 0 to currentTime_
            if (currentAnim->loop && prevTime > currentTime_) {
                if (event.time <= currentTime_) {
                    eventCb_(event.name);
                }
            }
        }
    }

    // Fire completion callback
    if (!playing_ && completionCb_) {
        completionCb_();
    }
}

float AnimPlayer::getNodePropertyValue(const std::string& nodeId, const std::string& property) const {
    const std::string key = nodeId + ":" + property;
    auto it = propertyCache_.find(key);
    if (it != propertyCache_.end()) {
        return it->second;
    }
    return 0.0f;
}

float AnimPlayer::evaluateTrack(const Track& track, float time) const {
    const auto& kfs = track.keyframes;
    if (kfs.empty()) {
        return 0.0f;
    }
    if (kfs.size() == 1) {
        return std::get<float>(kfs[0].value);
    }
    if (time <= kfs.front().time) {
        return std::get<float>(kfs.front().value);
    }
    if (time >= kfs.back().time) {
        return std::get<float>(kfs.back().value);
    }

    // Find the two keyframes surrounding the requested time
    size_t i = 0;
    for (size_t j = 0; j + 1 < kfs.size(); ++j) {
        if (time >= kfs[j].time && time <= kfs[j + 1].time) {
            i = j;
            break;
        }
    }

    const auto& kf0 = kfs[i];
    const auto& kf1 = kfs[i + 1];
    const float dt = kf1.time - kf0.time;
    if (std::fabs(dt) < 1e-6f) {
        return std::get<float>(kf0.value);
    }

    const float t = (time - kf0.time) / dt;
    const float easedT = Easing::apply(kf0.easing, t);
    const float v0 = std::get<float>(kf0.value);
    const float v1 = std::get<float>(kf1.value);

    return v0 + (v1 - v0) * easedT;
}

} // namespace anim
