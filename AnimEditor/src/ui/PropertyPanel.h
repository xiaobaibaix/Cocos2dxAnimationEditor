#pragma once
#include "core/AnimData.h"
#include <functional>

namespace anim {

class PropertyPanel {
public:
    using OnPropertyChanged = std::function<void(const std::string& nodeId)>;

    void setNode(const NodePtr& node) { currentNode_ = node; }
    void setOnPropertyChanged(OnPropertyChanged cb) { onPropertyChanged_ = std::move(cb); }
    void render();

private:
    NodePtr currentNode_;
    OnPropertyChanged onPropertyChanged_;
};

} // namespace anim
