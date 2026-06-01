#pragma once
#include "core/AnimData.h"
#include <optional>
#include <functional>

namespace anim {

class SceneGraph {
public:
    const std::vector<NodePtr>& getRootNodes() const { return roots_; }

    std::optional<NodePtr> addNode(const std::string& id, NodeType type,
                                    const std::string& name,
                                    const std::optional<std::string>& parentId);
    bool removeNode(const std::string& id);
    std::optional<NodePtr> findById(const std::string& id) const;
    bool renameNode(const std::string& id, const std::string& newName);
    bool reorderNode(const std::string& id, int newIndex);
    bool reparentNode(const std::string& id, const std::optional<std::string>& newParentId);

    void setOnChanged(std::function<void()> cb) { onChanged_ = std::move(cb); }
    void clear();

private:
    std::vector<NodePtr> roots_;
    std::function<void()> onChanged_;

    bool removeNodeFrom(const std::string& id, std::vector<NodePtr>& nodes);
};

} // namespace anim
