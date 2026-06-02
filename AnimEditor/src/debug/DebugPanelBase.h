#pragma once

namespace anim {

class DebugPanelBase {
public:
    virtual ~DebugPanelBase() = default;
    virtual const char* name() const = 0;
    virtual void render() = 0;
    bool open = true;
};

} // namespace anim
