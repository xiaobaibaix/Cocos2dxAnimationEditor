#pragma once
#include "core/AnimData.h"
#include <map>
#include <string>

namespace anim {

class AnimationEngine {
public:
    float evaluate(const Track& track, float time) const;
    std::map<std::string, KeyframeValue> evaluateAtTime(
        const AnimProject& project, const std::string& animName, float time) const;
};

} // namespace anim
