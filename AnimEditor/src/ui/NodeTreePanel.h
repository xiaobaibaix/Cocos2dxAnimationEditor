#pragma once
#include "core/AnimData.h"
#include "core/SceneGraph.h"
#include <functional>
#include <string>

namespace anim {

class NodeTreePanel {
public:
    using OnNodeSelected = std::function<void(const std::string& nodeId)>;

    void setSceneGraph(SceneGraph* sg) { sceneGraph_ = sg; }
    void setOnNodeSelected(OnNodeSelected cb) { onNodeSelected_ = std::move(cb); }
    void setSelectedNode(const std::string& id) { selectedId_ = id; }
    const std::string& getSelectedNode() const { return selectedId_; }
    void render();

private:
    SceneGraph* sceneGraph_ = nullptr;
    std::string selectedId_;
    OnNodeSelected onNodeSelected_;
    int nodeCounter_ = 0;

    void renderNode(const NodePtr& node);
};

} // namespace anim
