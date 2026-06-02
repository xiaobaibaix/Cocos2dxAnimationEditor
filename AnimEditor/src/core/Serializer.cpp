#include "Serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace anim {

using json = nlohmann::json;

// --- Enum <-> string helpers ---

static const char* nodeTypeToString(NodeType t) {
    switch (t) {
        case NodeType::Node:   return "Node";
        case NodeType::Sprite: return "Sprite";
        case NodeType::Label:  return "Label";
        case NodeType::Button: return "Button";
    }
    return "Node";
}

static std::optional<NodeType> stringToNodeType(const std::string& s) {
    if (s == "Node")   return NodeType::Node;
    if (s == "Sprite") return NodeType::Sprite;
    if (s == "Label")  return NodeType::Label;
    if (s == "Button") return NodeType::Button;
    return std::nullopt;
}

static const char* easingTypeToString(EasingType t) {
    switch (t) {
        case EasingType::Linear:          return "linear";
        case EasingType::EaseIn:          return "easeIn";
        case EasingType::EaseOut:         return "easeOut";
        case EasingType::EaseInOut:       return "easeInOut";
        case EasingType::EaseOutBack:     return "easeOutBack";
        case EasingType::EaseOutBounce:   return "easeOutBounce";
        case EasingType::EaseOutElastic:  return "easeOutElastic";
    }
    return "linear";
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

static json vec2ToJson(const Vec2& v) {
    return {{"x", v.x}, {"y", v.y}};
}

static Vec2 jsonToVec2(const json& j) {
    Vec2 v;
    v.x = j.value("x", 0.0f);
    v.y = j.value("y", 0.0f);
    return v;
}

static json colorToJson(const Color& c) {
    return {{"r", c.r}, {"g", c.g}, {"b", c.b}};
}

static Color jsonToColor(const json& j) {
    Color c;
    c.r = j.value("r", static_cast<uint8_t>(255));
    c.g = j.value("g", static_cast<uint8_t>(255));
    c.b = j.value("b", static_cast<uint8_t>(255));
    return c;
}

// --- NodeProperties ---

static json nodePropertiesToJson(const NodeProperties& p) {
    return {
        {"position",  vec2ToJson(p.position)},
        {"scale",     vec2ToJson(p.scale)},
        {"rotation",  p.rotation},
        {"anchor",    vec2ToJson(p.anchor)},
        {"opacity",   p.opacity},
        {"color",     colorToJson(p.color)},
        {"visible",   p.visible},
        {"texture",   p.texture},
        {"text",      p.text},
        {"font",      p.font},
        {"fontSize",  p.fontSize},
        {"zIndex",    p.zIndex}
    };
}

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

// --- KeyframeValue (variant<float, int, bool, string>) ---

static json keyframeValueToJson(const KeyframeValue& kv) {
    return std::visit([](const auto& v) -> json {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, float>) {
            return v;
        } else if constexpr (std::is_same_v<T, int>) {
            return v;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, Vec2>) {
            return vec2ToJson(v);
        }
    }, kv);
}

static std::optional<KeyframeValue> jsonToKeyframeValue(const json& j) {
    if (j.is_boolean()) {
        return j.get<bool>();
    } else if (j.is_number_integer()) {
        return j.get<int>();
    } else if (j.is_number_float()) {
        return j.get<float>();
    } else if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_object() && j.contains("x") && j.contains("y")) {
        return jsonToVec2(j);
    }
    return std::nullopt;
}

// --- Keyframe ---

static json keyframeToJson(const Keyframe& kf) {
    return {
        {"time",   kf.time},
        {"value",  keyframeValueToJson(kf.value)},
        {"easing", easingTypeToString(kf.easing)}
    };
}

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

static json trackToJson(const Track& t) {
    json kfs = json::array();
    for (const auto& kf : t.keyframes) {
        kfs.push_back(keyframeToJson(kf));
    }
    return {
        {"nodeId",    t.nodeId},
        {"property",  t.property},
        {"keyframes", kfs}
    };
}

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

static json nodeToJson(const Node& n) {
    json children = json::array();
    for (const auto& child : n.children) {
        children.push_back(nodeToJson(*child));
    }
    return {
        {"id",         n.id},
        {"type",       nodeTypeToString(n.type)},
        {"name",       n.name},
        {"properties", nodePropertiesToJson(n.properties)},
        {"children",   children}
    };
}

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

static json animationToJson(const Animation& a) {
    json tracks = json::array();
    for (const auto& t : a.tracks) {
        tracks.push_back(trackToJson(t));
    }
    return {
        {"name",     a.name},
        {"duration", a.duration},
        {"loop",     a.loop},
        {"tracks",   tracks}
    };
}

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

static json eventToJson(const AnimEvent& e) {
    return {
        {"time",   e.time},
        {"nodeId", e.nodeId},
        {"name",   e.name}
    };
}

static std::optional<AnimEvent> jsonToEvent(const json& j) {
    AnimEvent e;
    e.time   = j.value("time",   0.0f);
    e.nodeId = j.value("nodeId", std::string{});
    e.name   = j.value("name",   std::string{});
    return e;
}

// --- Public API ---

std::string Serializer::serialize(const AnimProject& project) {
    json doc;

    // meta
    doc["meta"] = {
        {"version",   project.meta.version},
        {"canvas",    {{"width", project.meta.canvasWidth}, {"height", project.meta.canvasHeight}}},
        {"frameRate", project.meta.frameRate}
    };

    // nodeTree
    json nodeTree = json::array();
    for (const auto& n : project.nodeTree) {
        nodeTree.push_back(nodeToJson(*n));
    }
    doc["nodeTree"] = nodeTree;

    // animations
    json animations = json::array();
    for (const auto& a : project.animations) {
        animations.push_back(animationToJson(a));
    }
    doc["animations"] = animations;

    // events
    json events = json::array();
    for (const auto& e : project.events) {
        events.push_back(eventToJson(e));
    }
    doc["events"] = events;

    return doc.dump(2);
}

std::optional<AnimProject> Serializer::deserialize(const std::string& jsonStr) {
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

bool Serializer::saveToFile(const AnimProject& project, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << serialize(project);
    return file.good();
}

std::optional<AnimProject> Serializer::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    std::ostringstream ss;
    ss << file.rdbuf();
    return deserialize(ss.str());
}

} // namespace anim
