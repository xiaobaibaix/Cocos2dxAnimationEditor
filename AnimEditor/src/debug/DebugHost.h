#pragma once
#include "debug/DebugPanelBase.h"
#include <memory>
#include <vector>

namespace anim {

class DebugHost {
public:
    void registerPanel(std::unique_ptr<DebugPanelBase> panel);
    void renderMenu();
    void renderPanels();

private:
    std::vector<std::unique_ptr<DebugPanelBase>> panels_;
};

} // namespace anim
