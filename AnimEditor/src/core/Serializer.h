#pragma once
#include "AnimData.h"
#include <string>
#include <optional>

namespace anim {

class Serializer {
public:
    static std::string serialize(const AnimProject& project);
    static std::optional<AnimProject> deserialize(const std::string& json);
    static bool saveToFile(const AnimProject& project, const std::string& path);
    static std::optional<AnimProject> loadFromFile(const std::string& path);
};

} // namespace anim
