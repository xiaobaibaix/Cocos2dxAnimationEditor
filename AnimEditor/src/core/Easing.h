#pragma once
#include "core/AnimData.h"

namespace anim {

class Easing {
public:
    static float apply(EasingType type, float t);

private:
    static float easeIn(float t);
    static float easeOut(float t);
    static float easeInOut(float t);
    static float easeOutBack(float t);
    static float easeOutBounce(float t);
    static float easeOutElastic(float t);
};

} // namespace anim
