#include "ui/PropertyPanel.h"
#include "imgui.h"

namespace anim {

void PropertyPanel::render() {
    ImGui::Begin("Properties");

    if (!currentNode_) {
        ImGui::TextDisabled("Select a node to inspect its properties.");
        ImGui::End();
        return;
    }

    bool changed = false;

    // Node identity
    char nameBuf[128];
    snprintf(nameBuf, sizeof(nameBuf), "%s", currentNode_->name.c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        currentNode_->name = nameBuf;
        changed = true;
    }

    const char* typeNames[] = {"Node", "Sprite", "Label", "Button"};
    int typeIndex = static_cast<int>(currentNode_->type);
    ImGui::Combo("Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames));
    currentNode_->type = static_cast<NodeType>(typeIndex);

    ImGui::Separator();

    // Transform section
    if (ImGui::CollapsingHeader("Transform")) {
        if (ImGui::DragFloat2("Position", &currentNode_->properties.position.x)) {
            changed = true;
        }
        if (ImGui::DragFloat2("Scale", &currentNode_->properties.scale.x, 0.01f)) {
            changed = true;
        }
        if (ImGui::DragFloat("Rotation", &currentNode_->properties.rotation, 1.0f, -360.0f, 360.0f)) {
            changed = true;
        }
        if (ImGui::DragFloat2("Anchor", &currentNode_->properties.anchor.x, 0.01f, 0.0f, 1.0f)) {
            changed = true;
        }
    }

    // Appearance section
    if (ImGui::CollapsingHeader("Appearance")) {
        int opacity = currentNode_->properties.opacity;
        if (ImGui::SliderInt("Opacity", &opacity, 0, 255)) {
            currentNode_->properties.opacity = static_cast<uint8_t>(opacity);
            changed = true;
        }

        float color[3] = {
            currentNode_->properties.color.r / 255.0f,
            currentNode_->properties.color.g / 255.0f,
            currentNode_->properties.color.b / 255.0f
        };
        if (ImGui::ColorEdit3("Color", color)) {
            currentNode_->properties.color.r = static_cast<uint8_t>(color[0] * 255.0f);
            currentNode_->properties.color.g = static_cast<uint8_t>(color[1] * 255.0f);
            currentNode_->properties.color.b = static_cast<uint8_t>(color[2] * 255.0f);
            changed = true;
        }

        if (ImGui::Checkbox("Visible", &currentNode_->properties.visible)) {
            changed = true;
        }
    }

    // Sprite-specific properties
    if (currentNode_->type == NodeType::Sprite) {
        if (ImGui::CollapsingHeader("Sprite")) {
            char texBuf[256];
            snprintf(texBuf, sizeof(texBuf), "%s", currentNode_->properties.texture.c_str());
            if (ImGui::InputText("Texture", texBuf, sizeof(texBuf))) {
                currentNode_->properties.texture = texBuf;
                changed = true;
            }
        }
    }

    // Label-specific properties
    if (currentNode_->type == NodeType::Label) {
        if (ImGui::CollapsingHeader("Label")) {
            char textBuf[256];
            snprintf(textBuf, sizeof(textBuf), "%s", currentNode_->properties.text.c_str());
            if (ImGui::InputText("Text", textBuf, sizeof(textBuf))) {
                currentNode_->properties.text = textBuf;
                changed = true;
            }
            if (ImGui::DragFloat("Font Size", &currentNode_->properties.fontSize, 1.0f, 1.0f, 200.0f)) {
                changed = true;
            }
        }
    }

    if (changed && onPropertyChanged_) {
        onPropertyChanged_(currentNode_->id);
    }

    ImGui::End();
}

} // namespace anim
