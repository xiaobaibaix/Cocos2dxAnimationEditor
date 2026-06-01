#include <gtest/gtest.h>
#include "core/Serializer.h"
#include <cstdio>
#include <fstream>

using namespace anim;

// --- Helpers ---

static AnimProject makeEmptyProject() {
    return AnimProject{};
}

static NodePtr makeNode(const std::string& id, NodeType type, const std::string& name) {
    auto n = std::make_shared<Node>();
    n->id   = id;
    n->type = type;
    n->name = name;
    return n;
}

// --- Tests ---

TEST(Serializer, SerializeEmptyProject) {
    auto project = makeEmptyProject();
    std::string json = Serializer::serialize(project);

    EXPECT_NE(json.find("\"version\""), std::string::npos);
    EXPECT_NE(json.find("\"canvas\""),  std::string::npos);
    EXPECT_NE(json.find("\"width\""),   std::string::npos);
    EXPECT_NE(json.find("\"height\""),  std::string::npos);
}

TEST(Serializer, RoundTripEmptyProject) {
    auto original = makeEmptyProject();
    std::string json = Serializer::serialize(original);

    auto loaded = Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->meta.version,       original.meta.version);
    EXPECT_EQ(loaded->meta.canvasWidth,   original.meta.canvasWidth);
    EXPECT_EQ(loaded->meta.canvasHeight,  original.meta.canvasHeight);
    EXPECT_EQ(loaded->meta.frameRate,     original.meta.frameRate);
    EXPECT_TRUE(loaded->nodeTree.empty());
    EXPECT_TRUE(loaded->animations.empty());
    EXPECT_TRUE(loaded->events.empty());
}

TEST(Serializer, SerializeNodeTree) {
    AnimProject original;
    original.meta.version = "1.0";
    original.meta.canvasWidth  = 1920;
    original.meta.canvasHeight = 1080;

    auto root  = makeNode("root", NodeType::Node, "Root");
    auto child = makeNode("child1", NodeType::Sprite, "Background");
    child->properties.position = {100.0f, 200.0f};
    child->properties.scale    = {2.0f, 2.0f};
    child->properties.rotation = 45.0f;
    child->properties.opacity  = 200;
    child->properties.color    = {128, 64, 32};
    child->properties.texture  = "bg.png";
    child->properties.visible  = false;
    child->properties.zIndex   = 5;

    auto label = makeNode("label1", NodeType::Label, "Title");
    label->properties.text     = "Hello";
    label->properties.font     = "Arial";
    label->properties.fontSize = 36.0f;

    root->children.push_back(child);
    root->children.push_back(label);
    original.nodeTree.push_back(root);

    std::string json = Serializer::serialize(original);
    auto loaded = Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());

    ASSERT_EQ(loaded->nodeTree.size(), 1u);
    const auto& rt = loaded->nodeTree[0];
    EXPECT_EQ(rt->id, "root");
    EXPECT_EQ(rt->type, NodeType::Node);
    EXPECT_EQ(rt->name, "Root");
    ASSERT_EQ(rt->children.size(), 2u);

    const auto& c = rt->children[0];
    EXPECT_EQ(c->id, "child1");
    EXPECT_EQ(c->type, NodeType::Sprite);
    EXPECT_FLOAT_EQ(c->properties.position.x, 100.0f);
    EXPECT_FLOAT_EQ(c->properties.position.y, 200.0f);
    EXPECT_FLOAT_EQ(c->properties.scale.x, 2.0f);
    EXPECT_FLOAT_EQ(c->properties.scale.y, 2.0f);
    EXPECT_FLOAT_EQ(c->properties.rotation, 45.0f);
    EXPECT_EQ(c->properties.opacity, 200);
    EXPECT_EQ(c->properties.color.r, 128);
    EXPECT_EQ(c->properties.color.g, 64);
    EXPECT_EQ(c->properties.color.b, 32);
    EXPECT_EQ(c->properties.texture, "bg.png");
    EXPECT_FALSE(c->properties.visible);
    EXPECT_EQ(c->properties.zIndex, 5);

    const auto& l = rt->children[1];
    EXPECT_EQ(l->id, "label1");
    EXPECT_EQ(l->type, NodeType::Label);
    EXPECT_EQ(l->properties.text, "Hello");
    EXPECT_EQ(l->properties.font, "Arial");
    EXPECT_FLOAT_EQ(l->properties.fontSize, 36.0f);
}

TEST(Serializer, SerializeAnimationTracks) {
    AnimProject original;

    Animation anim;
    anim.name     = "show";
    anim.duration = 0.6f;
    anim.loop     = false;

    Track track;
    track.nodeId   = "n_bg";
    track.property = "opacity";

    Keyframe kf1;
    kf1.time   = 0.0f;
    kf1.value  = 0.0f;
    kf1.easing = EasingType::Linear;

    Keyframe kf2;
    kf2.time   = 0.6f;
    kf2.value  = 1.0f;
    kf2.easing = EasingType::EaseOutBack;

    track.keyframes = {kf1, kf2};
    anim.tracks.push_back(track);
    original.animations.push_back(anim);

    std::string json = Serializer::serialize(original);
    auto loaded = Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());

    ASSERT_EQ(loaded->animations.size(), 1u);
    const auto& a = loaded->animations[0];
    EXPECT_EQ(a.name, "show");
    EXPECT_FLOAT_EQ(a.duration, 0.6f);
    EXPECT_FALSE(a.loop);

    ASSERT_EQ(a.tracks.size(), 1u);
    EXPECT_EQ(a.tracks[0].nodeId, "n_bg");
    EXPECT_EQ(a.tracks[0].property, "opacity");
    ASSERT_EQ(a.tracks[0].keyframes.size(), 2u);

    EXPECT_FLOAT_EQ(a.tracks[0].keyframes[0].time, 0.0f);
    EXPECT_FLOAT_EQ(std::get<float>(a.tracks[0].keyframes[0].value), 0.0f);
    EXPECT_EQ(a.tracks[0].keyframes[0].easing, EasingType::Linear);

    EXPECT_FLOAT_EQ(a.tracks[0].keyframes[1].time, 0.6f);
    EXPECT_FLOAT_EQ(std::get<float>(a.tracks[0].keyframes[1].value), 1.0f);
    EXPECT_EQ(a.tracks[0].keyframes[1].easing, EasingType::EaseOutBack);
}

TEST(Serializer, SerializeEvents) {
    AnimProject original;

    AnimEvent ev1;
    ev1.time   = 0.0f;
    ev1.nodeId = "n_bg";
    ev1.name   = "playSound:whoosh";

    AnimEvent ev2;
    ev2.time   = 1.5f;
    ev2.nodeId = "n_title";
    ev2.name   = "trigger:fade";

    original.events = {ev1, ev2};

    std::string json = Serializer::serialize(original);
    auto loaded = Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());

    ASSERT_EQ(loaded->events.size(), 2u);
    EXPECT_FLOAT_EQ(loaded->events[0].time, 0.0f);
    EXPECT_EQ(loaded->events[0].nodeId, "n_bg");
    EXPECT_EQ(loaded->events[0].name, "playSound:whoosh");

    EXPECT_FLOAT_EQ(loaded->events[1].time, 1.5f);
    EXPECT_EQ(loaded->events[1].nodeId, "n_title");
    EXPECT_EQ(loaded->events[1].name, "trigger:fade");
}

TEST(Serializer, DeserializeInvalidJSON) {
    auto result = Serializer::deserialize("{invalid}");
    EXPECT_FALSE(result.has_value());

    auto result2 = Serializer::deserialize("");
    EXPECT_FALSE(result2.has_value());

    auto result3 = Serializer::deserialize("not json at all");
    EXPECT_FALSE(result3.has_value());
}

TEST(Serializer, LoadSaveFileRoundTrip) {
    const std::string testPath = "/tmp/anim_serializer_test_roundtrip.json";

    AnimProject original;
    original.meta.version      = "1.0";
    original.meta.canvasWidth  = 800;
    original.meta.canvasHeight = 600;
    original.meta.frameRate    = 30;

    auto node = makeNode("n1", NodeType::Button, "Btn");
    node->properties.color = {255, 0, 128};
    original.nodeTree.push_back(node);

    Animation anim;
    anim.name     = "click";
    anim.duration = 0.3f;
    anim.loop     = true;

    Track t;
    t.nodeId   = "n1";
    t.property = "scale";
    Keyframe kf;
    kf.time   = 0.15f;
    kf.value  = 1.2f;
    kf.easing = EasingType::EaseOutElastic;
    t.keyframes.push_back(kf);
    anim.tracks.push_back(t);
    original.animations.push_back(anim);

    AnimEvent ev;
    ev.time   = 0.1f;
    ev.nodeId = "n1";
    ev.name   = "playSound:click";
    original.events.push_back(ev);

    // Save
    ASSERT_TRUE(Serializer::saveToFile(original, testPath));

    // Load
    auto loaded = Serializer::loadFromFile(testPath);
    ASSERT_TRUE(loaded.has_value());

    // Verify meta
    EXPECT_EQ(loaded->meta.version,      "1.0");
    EXPECT_EQ(loaded->meta.canvasWidth,  800);
    EXPECT_EQ(loaded->meta.canvasHeight, 600);
    EXPECT_EQ(loaded->meta.frameRate,    30);

    // Verify nodeTree
    ASSERT_EQ(loaded->nodeTree.size(), 1u);
    EXPECT_EQ(loaded->nodeTree[0]->id,   "n1");
    EXPECT_EQ(loaded->nodeTree[0]->type, NodeType::Button);
    EXPECT_EQ(loaded->nodeTree[0]->name, "Btn");
    EXPECT_EQ(loaded->nodeTree[0]->properties.color.r, 255);
    EXPECT_EQ(loaded->nodeTree[0]->properties.color.g, 0);
    EXPECT_EQ(loaded->nodeTree[0]->properties.color.b, 128);

    // Verify animation
    ASSERT_EQ(loaded->animations.size(), 1u);
    EXPECT_EQ(loaded->animations[0].name, "click");
    EXPECT_FLOAT_EQ(loaded->animations[0].duration, 0.3f);
    EXPECT_TRUE(loaded->animations[0].loop);
    ASSERT_EQ(loaded->animations[0].tracks.size(), 1u);
    ASSERT_EQ(loaded->animations[0].tracks[0].keyframes.size(), 1u);
    EXPECT_FLOAT_EQ(loaded->animations[0].tracks[0].keyframes[0].time, 0.15f);
    EXPECT_FLOAT_EQ(std::get<float>(loaded->animations[0].tracks[0].keyframes[0].value), 1.2f);
    EXPECT_EQ(loaded->animations[0].tracks[0].keyframes[0].easing, EasingType::EaseOutElastic);

    // Verify event
    ASSERT_EQ(loaded->events.size(), 1u);
    EXPECT_FLOAT_EQ(loaded->events[0].time, 0.1f);
    EXPECT_EQ(loaded->events[0].nodeId, "n1");
    EXPECT_EQ(loaded->events[0].name, "playSound:click");

    // Cleanup
    std::remove(testPath.c_str());
}
