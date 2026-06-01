#include "core/Easing.h"
#include <cmath>
#include <algorithm>

namespace anim {

float Easing::apply(EasingType type, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (type) {
        case EasingType::Linear: return t;
        case EasingType::EaseIn: return easeIn(t);
        case EasingType::EaseOut: return easeOut(t);
        case EasingType::EaseInOut: return easeInOut(t);
        case EasingType::EaseOutBack: return easeOutBack(t);
        case EasingType::EaseOutBounce: return easeOutBounce(t);
        case EasingType::EaseOutElastic: return easeOutElastic(t);
    }
    return t;
}

float Easing::easeIn(float t) { return t * t; }

float Easing::easeOut(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }

float Easing::easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float Easing::easeOutBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float Easing::easeOutBounce(float t) {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
    if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

float Easing::easeOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    constexpr float c4 = (2.0f * 3.14159265f) / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

} // namespace anim
