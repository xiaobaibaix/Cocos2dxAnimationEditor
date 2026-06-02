#include "ui/NodeTreePanel.h"
#include "imgui.h"

namespace anim {

void NodeTreePanel::render() {
    ImGui::Begin("Nodes");

    if (ImGui::Button("+ Add Node")) {
        if (sceneGraph_) {
            ++nodeCounter_;
            std::string id = "node_" + std::to_string(nodeCounter_);
            std::string name = "Node " + std::to_string(nodeCounter_);

            std::optional<std::string> parentId;
            if (!selectedId_.empty()) {
                parentId = selectedId_;
            }

            auto node = sceneGraph_->addNode(id, NodeType::Node, name, parentId);
            if (node) {
                selectedId_ = id;
                if (onNodeSelected_) {
                    onNodeSelected_(id);
                }
                if (onNodeChanged_) {
                    onNodeChanged_();
                }
            }
        }
    }

    ImGui::Separator();

    if (sceneGraph_) {
        for (const auto& root : sceneGraph_->getRootNodes()) {
            renderNode(root);
        }
    }

    ImGui::End();
}

void NodeTreePanel::renderNode(const NodePtr& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node->id == selectedId_) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (node->children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const char* typeIcon = "Nod";
    switch (node->type) {
        case NodeType::Sprite: typeIcon = "Img"; break;
        case NodeType::Label:  typeIcon = "Txt"; break;
        case NodeType::Button: typeIcon = "Btn"; break;
        default: break;
    }

    bool hasAnim = onQueryNodeAnimated_ && onQueryNodeAnimated_(node->id);

    bool opened = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<intptr_t>(std::hash<std::string>{}(node->id))),
        flags, "[%s] %s", typeIcon, node->name.c_str());

    if (hasAnim) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 50, 255));
        ImGui::TextUnformatted("\xe2\x97\x86");
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked()) {
        selectedId_ = node->id;
        if (onNodeSelected_) {
            onNodeSelected_(node->id);
        }
    }

    if (ImGui::BeginPopupContextItem("NodeContextMenu")) {
        if (ImGui::MenuItem("Add Child")) {
            if (sceneGraph_) {
                ++nodeCounter_;
                std::string childId = "node_" + std::to_string(nodeCounter_);
                std::string childName = "Node " + std::to_string(nodeCounter_);
                sceneGraph_->addNode(childId, NodeType::Node, childName, node->id);
                if (onNodeChanged_) {
                    onNodeChanged_();
                }
            }
        }
        if (ImGui::MenuItem("Delete")) {
            if (sceneGraph_) {
                sceneGraph_->removeNode(node->id);
                if (selectedId_ == node->id) {
                    selectedId_.clear();
                    if (onNodeSelected_) {
                        onNodeSelected_("");
                    }
                }
                if (onNodeChanged_) {
                    onNodeChanged_();
                }
            }
        }
        ImGui::EndPopup();
    }

    if (opened) {
        for (const auto& child : node->children) {
            renderNode(child);
        }
        ImGui::TreePop();
    }
}

} // namespace anim
