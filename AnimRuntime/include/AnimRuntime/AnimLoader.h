#pragma once
#include "AnimRuntime/AnimData.h"
#include <string>
#include <optional>

namespace anim {

class AnimLoader {
public:
    static std::optional<AnimProject> loadFromFile(const std::string& path);
    static std::optional<AnimProject> loadFromString(const std::string& json);
};

} // namespace anim
