#include "core/SceneGraph.h"
#include <algorithm>

namespace anim {

namespace {

std::optional<NodePtr> findByIdIn(const std::string& id,
                                   const std::vector<NodePtr>& nodes) {
    for (const auto& node : nodes) {
        if (node->id == id) return node;
        auto found = findByIdIn(id, node->children);
        if (found) return found;
    }
    return std::nullopt;
}

std::optional<NodePtr> findParentOf(const std::string& id,
                                     const std::vector<NodePtr>& nodes) {
    for (const auto& node : nodes) {
        for (const auto& child : node->children) {
            if (child->id == id) return node;
        }
        auto found = findParentOf(id, node->children);
        if (found) return found;
    }
    return std::nullopt;
}

} // anonymous namespace

std::optional<NodePtr> SceneGraph::addNode(const std::string& id, NodeType type,
                                            const std::string& name,
                                            const std::optional<std::string>& parentId) {
    if (findById(id)) return std::nullopt;

    auto node = std::make_shared<Node>();
    node->id = id;
    node->type = type;
    node->name = name;

    if (parentId) {
        auto parent = findById(*parentId);
        if (!parent) return std::nullopt;
        (*parent)->children.push_back(node);
    } else {
        roots_.push_back(node);
    }

    if (onChanged_) onChanged_();
    return node;
}

bool SceneGraph::removeNode(const std::string& id) {
    if (!findById(id)) return false;

    if (removeNodeFrom(id, roots_)) {
        if (onChanged_) onChanged_();
        return true;
    }
    return false;
}

bool SceneGraph::removeNodeFrom(const std::string& id, std::vector<NodePtr>& nodes) {
    auto it = std::find_if(nodes.begin(), nodes.end(),
                            [&id](const NodePtr& n) { return n->id == id; });
    if (it != nodes.end()) {
        nodes.erase(it);
        return true;
    }
    for (auto& node : nodes) {
        if (removeNodeFrom(id, node->children)) return true;
    }
    return false;
}

std::optional<NodePtr> SceneGraph::findById(const std::string& id) const {
    return findByIdIn(id, roots_);
}

bool SceneGraph::renameNode(const std::string& id, const std::string& newName) {
    auto node = findById(id);
    if (!node) return false;
    (*node)->name = newName;
    if (onChanged_) onChanged_();
    return true;
}

bool SceneGraph::reorderNode(const std::string& id, int newIndex) {
    auto node = findById(id);
    if (!node) return false;

    auto parent = findParentOf(id, roots_);
    std::vector<NodePtr>* siblings = nullptr;

    if (parent) {
        siblings = &(*parent)->children;
    } else {
        siblings = &roots_;
    }

    auto it = std::find_if(siblings->begin(), siblings->end(),
                            [&id](const NodePtr& n) { return n->id == id; });
    if (it == siblings->end()) return false;

    auto nodePtr = *it;
    siblings->erase(it);

    int clampedIndex = std::max(0, std::min(newIndex, static_cast<int>(siblings->size())));
    siblings->insert(siblings->begin() + clampedIndex, nodePtr);

    if (onChanged_) onChanged_();
    return true;
}

bool SceneGraph::reparentNode(const std::string& id,
                               const std::optional<std::string>& newParentId) {
    auto node = findById(id);
    if (!node) return false;

    if (newParentId) {
        auto newParent = findById(*newParentId);
        if (!newParent) return false;
        if (*newParent == *node) return false;
    }

    auto nodePtr = *node;
    if (!removeNodeFrom(id, roots_)) return false;

    if (newParentId) {
        auto newParent = findById(*newParentId);
        (*newParent)->children.push_back(nodePtr);
    } else {
        roots_.push_back(nodePtr);
    }

    if (onChanged_) onChanged_();
    return true;
}

void SceneGraph::clear() {
    roots_.clear();
    if (onChanged_) onChanged_();
}

} // namespace anim
