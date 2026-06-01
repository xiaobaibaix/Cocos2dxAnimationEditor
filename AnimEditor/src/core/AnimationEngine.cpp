#include "core/AnimationEngine.h"
#include "core/Easing.h"
#include <algorithm>
#include <cmath>

namespace anim {

float AnimationEngine::evaluate(const Track& track, float time) const {
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

    // Find the two keyframes that surround the requested time.
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

std::map<std::string, KeyframeValue> AnimationEngine::evaluateAtTime(
    const AnimProject& project, const std::string& animName, float time) const {
    std::map<std::string, KeyframeValue> result;

    for (const auto& anim : project.animations) {
        if (anim.name != animName) {
            continue;
        }
        for (const auto& track : anim.tracks) {
            const std::string key = track.nodeId + ":" + track.property;
            result[key] = evaluate(track, time);
        }
        break;
    }

    return result;
}

} // namespace anim
