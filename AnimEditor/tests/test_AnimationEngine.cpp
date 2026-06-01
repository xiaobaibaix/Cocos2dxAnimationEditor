#include <gtest/gtest.h>
#include "core/AnimationEngine.h"

TEST(AnimationEngineTest, LinearInterpolation) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.nodeId = "n1";
    track.property = "opacity";
    track.keyframes = {
        {0.0f, 0.0f, anim::EasingType::Linear},
        {1.0f, 255.0f, anim::EasingType::Linear}
    };
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.5f), 127.5f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 1.0f), 255.0f);
}

TEST(AnimationEngineTest, SingleKeyframe) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.keyframes = {{0.5f, 100.0f, anim::EasingType::Linear}};
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.0f), 100.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 1.0f), 100.0f);
}

TEST(AnimationEngineTest, ThreeKeyframes) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.keyframes = {
        {0.0f, 0.0f, anim::EasingType::Linear},
        {0.5f, 100.0f, anim::EasingType::Linear},
        {1.0f, 200.0f, anim::EasingType::Linear}
    };
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.25f), 50.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.75f), 150.0f);
}

TEST(AnimationEngineTest, NoKeyframes) {
    anim::AnimationEngine engine;
    anim::Track track;
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.5f), 0.0f);
}

TEST(AnimationEngineTest, EvaluateAllTracksForTime) {
    anim::AnimProject project;
    anim::Animation anim;
    anim.name = "test";
    anim.duration = 1.0f;

    anim::Track t1;
    t1.nodeId = "n1"; t1.property = "opacity";
    t1.keyframes = {{0.0f, 0.0f, anim::EasingType::Linear}, {1.0f, 255.0f, anim::EasingType::Linear}};
    anim.tracks.push_back(t1);

    anim::Track t2;
    t2.nodeId = "n1"; t2.property = "position.x";
    t2.keyframes = {{0.0f, 100.0f, anim::EasingType::Linear}, {1.0f, 200.0f, anim::EasingType::Linear}};
    anim.tracks.push_back(t2);

    project.animations.push_back(anim);

    anim::AnimationEngine engine;
    auto result = engine.evaluateAtTime(project, "test", 0.5f);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_FLOAT_EQ(std::get<float>(result["n1:opacity"]), 127.5f);
    EXPECT_FLOAT_EQ(std::get<float>(result["n1:position.x"]), 150.0f);
}
