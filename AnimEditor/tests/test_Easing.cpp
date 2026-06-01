#include <gtest/gtest.h>
#include "core/Easing.h"
#include <cmath>

TEST(EasingTest, LinearEndpoints) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 0.5f), 0.5f);
}

TEST(EasingTest, EaseOutEndpoints) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::EaseOut, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::EaseOut, 1.0f), 1.0f);
}

TEST(EasingTest, EaseOutBackOvershoots) {
    float mid = anim::Easing::apply(anim::EasingType::EaseOutBack, 0.5f);
    EXPECT_GT(mid, 0.5f);
}

TEST(EasingTest, ClampedInput) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, -0.5f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 1.5f), 1.0f);
}

TEST(EasingTest, AllTypesProduceValidOutput) {
    anim::EasingType types[] = {
        anim::EasingType::Linear, anim::EasingType::EaseIn,
        anim::EasingType::EaseOut, anim::EasingType::EaseInOut,
        anim::EasingType::EaseOutBack, anim::EasingType::EaseOutBounce,
        anim::EasingType::EaseOutElastic
    };
    for (auto t : types) {
        float v = anim::Easing::apply(t, 0.5f);
        EXPECT_TRUE(std::isfinite(v));
    }
}
