#include "AnimRuntime/AnimLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace anim {

using json = nlohmann::json;

// --- Enum <-> string helpers ---

static std::optional<NodeType> stringToNodeType(const std::string& s) {
    if (s == "Node")   return NodeType::Node;
    if (s == "Sprite") return NodeType::Sprite;
    if (s == "Label")  return NodeType::Label;
    if (s == "Button") return NodeType::Button;
    return std::nullopt;
}

static std::optional<EasingType> stringToEasingType(const std::string& s) {
    if (s == "linear")          return EasingType::Linear;
    if (s == "easeIn")          return EasingType::EaseIn;
    if (s == "easeOut")         return EasingType::EaseOut;
    if (s == "easeInOut")       return EasingType::EaseInOut;
    if (s == "easeOutBack")     return EasingType::EaseOutBack;
    if (s == "easeOutBounce")   return EasingType::EaseOutBounce;
    if (s == "easeOutElastic")  return EasingType::EaseOutElastic;
    return std::nullopt;
}

// --- Vec2 / Color ---

static Vec2 jsonToVec2(const json& j) {
    Vec2 v;
    v.x = j.value("x", 0.0f);
    v.y = j.value("y", 0.0f);
    return v;
}

static Color jsonToColor(const json& j) {
    Color c;
    c.r = j.value("r", static_cast<uint8_t>(255));
    c.g = j.value("g", static_cast<uint8_t>(255));
    c.b = j.value("b", static_cast<uint8_t>(255));
    return c;
}

// --- NodeProperties ---

static NodeProperties jsonToNodeProperties(const json& j) {
    NodeProperties p;
    if (j.contains("position"))  p.position = jsonToVec2(j["position"]);
    if (j.contains("scale"))     p.scale    = jsonToVec2(j["scale"]);
    p.rotation = j.value("rotation", 0.0f);
    if (j.contains("anchor"))    p.anchor   = jsonToVec2(j["anchor"]);
    p.opacity  = j.value("opacity", static_cast<uint8_t>(255));
    if (j.contains("color"))     p.color    = jsonToColor(j["color"]);
    p.visible  = j.value("visible", true);
    p.texture  = j.value("texture", std::string{});
    p.text     = j.value("text",    std::string{});
    p.font     = j.value("font",    std::string{});
    p.fontSize = j.value("fontSize", 24.0f);
    p.zIndex   = j.value("zIndex",  0);
    return p;
}

// --- KeyframeValue ---

static std::optional<KeyframeValue> jsonToKeyframeValue(const json& j) {
    if (j.is_boolean()) {
        return j.get<bool>();
    } else if (j.is_number_integer()) {
        return j.get<int>();
    } else if (j.is_number_float()) {
        return j.get<float>();
    } else if (j.is_string()) {
        return j.get<std::string>();
    }
    return std::nullopt;
}

// --- Keyframe ---

static std::optional<Keyframe> jsonToKeyframe(const json& j) {
    Keyframe kf;
    kf.time = j.value("time", 0.0f);

    if (!j.contains("value")) return std::nullopt;
    auto val = jsonToKeyframeValue(j["value"]);
    if (!val) return std::nullopt;
    kf.value = std::move(*val);

    if (j.contains("easing")) {
        auto easing = stringToEasingType(j["easing"].get<std::string>());
        if (!easing) return std::nullopt;
        kf.easing = *easing;
    }
    return kf;
}

// --- Track ---

static std::optional<Track> jsonToTrack(const json& j) {
    Track t;
    t.nodeId   = j.value("nodeId",   std::string{});
    t.property = j.value("property", std::string{});

    if (j.contains("keyframes") && j["keyframes"].is_array()) {
        for (const auto& kj : j["keyframes"]) {
            auto kf = jsonToKeyframe(kj);
            if (!kf) return std::nullopt;
            t.keyframes.push_back(std::move(*kf));
        }
    }
    return t;
}

// --- Node (recursive) ---

static std::optional<NodePtr> jsonToNode(const json& j) {
    auto node = std::make_shared<Node>();
    node->id   = j.value("id",   std::string{});
    node->name = j.value("name", std::string{});

    if (j.contains("type")) {
        auto t = stringToNodeType(j["type"].get<std::string>());
        if (!t) return std::nullopt;
        node->type = *t;
    }

    if (j.contains("properties")) {
        node->properties = jsonToNodeProperties(j["properties"]);
    }

    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& cj : j["children"]) {
            auto child = jsonToNode(cj);
            if (!child) return std::nullopt;
            node->children.push_back(std::move(*child));
        }
    }
    return node;
}

// --- Animation ---

static std::optional<Animation> jsonToAnimation(const json& j) {
    Animation a;
    a.name     = j.value("name",     std::string{});
    a.duration = j.value("duration", 1.0f);
    a.loop     = j.value("loop",     false);

    if (j.contains("tracks") && j["tracks"].is_array()) {
        for (const auto& tj : j["tracks"]) {
            auto t = jsonToTrack(tj);
            if (!t) return std::nullopt;
            a.tracks.push_back(std::move(*t));
        }
    }
    return a;
}

// --- AnimEvent ---

static std::optional<AnimEvent> jsonToEvent(const json& j) {
    AnimEvent e;
    e.time   = j.value("time",   0.0f);
    e.nodeId = j.value("nodeId", std::string{});
    e.name   = j.value("name",   std::string{});
    return e;
}

// --- Public API ---

std::optional<AnimProject> AnimLoader::loadFromString(const std::string& jsonStr) {
    try {
        auto doc = json::parse(jsonStr);
        AnimProject project;

        // meta
        if (doc.contains("meta")) {
            const auto& m = doc["meta"];
            project.meta.version = m.value("version", std::string{"1.0"});
            if (m.contains("canvas")) {
                project.meta.canvasWidth  = m["canvas"].value("width",  1280);
                project.meta.canvasHeight = m["canvas"].value("height", 720);
            }
            project.meta.frameRate = m.value("frameRate", 60);
        }

        // nodeTree
        if (doc.contains("nodeTree") && doc["nodeTree"].is_array()) {
            for (const auto& nj : doc["nodeTree"]) {
                auto n = jsonToNode(nj);
                if (!n) return std::nullopt;
                project.nodeTree.push_back(std::move(*n));
            }
        }

        // animations
        if (doc.contains("animations") && doc["animations"].is_array()) {
            for (const auto& aj : doc["animations"]) {
                auto a = jsonToAnimation(aj);
                if (!a) return std::nullopt;
                project.animations.push_back(std::move(*a));
            }
        }

        // events
        if (doc.contains("events") && doc["events"].is_array()) {
            for (const auto& ej : doc["events"]) {
                auto e = jsonToEvent(ej);
                if (!e) return std::nullopt;
                project.events.push_back(std::move(*e));
            }
        }

        return project;
    } catch (const json::exception&) {
        return std::nullopt;
    }
}

std::optional<AnimProject> AnimLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    std::ostringstream ss;
    ss << file.rdbuf();
    return loadFromString(ss.str());
}

} // namespace anim
