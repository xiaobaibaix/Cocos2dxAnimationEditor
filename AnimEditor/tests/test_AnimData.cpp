#include <gtest/gtest.h>
#include "core/AnimData.h"

TEST(AnimDataTest, NodeDefaultValues) {
    anim::Node node;
    node.id = "n1";
    node.type = anim::NodeType::Sprite;
    node.name = "TestNode";
    EXPECT_EQ(node.id, "n1");
    EXPECT_EQ(node.type, anim::NodeType::Sprite);
    EXPECT_EQ(node.name, "TestNode");
    EXPECT_EQ(node.properties.position.x, 0.0f);
    EXPECT_EQ(node.properties.position.y, 0.0f);
    EXPECT_EQ(node.properties.opacity, 255);
    EXPECT_TRUE(node.properties.visible);
    EXPECT_TRUE(node.children.empty());
}

TEST(AnimDataTest, NodeTreeNesting) {
    anim::Node root;
    root.id = "root";
    root.type = anim::NodeType::Node;
    root.name = "Root";
    anim::Node child;
    child.id = "child1";
    child.type = anim::NodeType::Sprite;
    child.name = "Bg";
    root.children.push_back(std::make_shared<anim::Node>(std::move(child)));
    ASSERT_EQ(root.children.size(), 1u);
    EXPECT_EQ(root.children[0]->id, "child1");
}

TEST(AnimDataTest, KeyframeCreation) {
    anim::Keyframe kf;
    kf.time = 0.5f;
    kf.value = 200.0f;
    kf.easing = anim::EasingType::EaseOut;
    EXPECT_FLOAT_EQ(kf.time, 0.5f);
    EXPECT_FLOAT_EQ(std::get<float>(kf.value), 200.0f);
    EXPECT_EQ(kf.easing, anim::EasingType::EaseOut);
}

TEST(AnimDataTest, AnimationTrackStructure) {
    anim::Track track;
    track.nodeId = "n1";
    track.property = "opacity";
    track.keyframes.push_back({0.0f, 0.0f, anim::EasingType::Linear});
    track.keyframes.push_back({0.4f, 255.0f, anim::EasingType::EaseOut});
    EXPECT_EQ(track.nodeId, "n1");
    EXPECT_EQ(track.property, "opacity");
    ASSERT_EQ(track.keyframes.size(), 2u);
    EXPECT_FLOAT_EQ(track.keyframes[0].time, 0.0f);
    EXPECT_FLOAT_EQ(track.keyframes[1].time, 0.4f);
}

TEST(AnimDataTest, AnimProjectMeta) {
    anim::AnimProject project;
    project.meta.version = "1.0";
    project.meta.canvasWidth = 1280;
    project.meta.canvasHeight = 720;
    project.meta.frameRate = 60;
    EXPECT_EQ(project.meta.version, "1.0");
    EXPECT_EQ(project.meta.canvasWidth, 1280);
}
