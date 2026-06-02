#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cstdint>

namespace anim {

enum class NodeType { Node, Sprite, Label, Button };
enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut, EaseOutBack, EaseOutBounce, EaseOutElastic };

struct Vec2 { float x = 0.0f, y = 0.0f; };
struct Color { uint8_t r = 255, g = 255, b = 255; };

struct NodeProperties {
    Vec2 position;
    Vec2 scale{1.0f, 1.0f};
    float rotation = 0.0f;
    Vec2 anchor{0.5f, 0.5f};
    uint8_t opacity = 255;
    Color color;
    bool visible = true;
    std::string texture;    // Sprite
    std::string text;       // Label
    std::string font;       // Label
    float fontSize = 24.0f; // Label
    int zIndex = 0;
};

struct Node;
using NodePtr = std::shared_ptr<Node>;

struct Node {
    std::string id;
    NodeType type = NodeType::Node;
    std::string name;
    NodeProperties properties;
    std::vector<NodePtr> children;
};

using KeyframeValue = std::variant<float, int, bool, std::string, Vec2>;

struct Keyframe {
    float time = 0.0f;
    KeyframeValue value = 0.0f;
    EasingType easing = EasingType::Linear;
};

struct Track {
    std::string nodeId;
    std::string property;
    std::vector<Keyframe> keyframes;
};

struct AnimEvent {
    float time = 0.0f;
    std::string nodeId;
    std::string name;
};

struct Animation {
    std::string name;
    float duration = 1.0f;
    bool loop = false;
    std::vector<Track> tracks;
};

struct ProjectMeta {
    std::string version = "1.0";
    int canvasWidth = 1280;
    int canvasHeight = 720;
    int frameRate = 60;
};

struct AnimProject {
    ProjectMeta meta;
    std::vector<NodePtr> nodeTree;
    std::vector<Animation> animations;
    std::vector<AnimEvent> events;
};

} // namespace anim
