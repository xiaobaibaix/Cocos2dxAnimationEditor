#include "debug/DebugPanelBase.h"
#include "imgui.h"

namespace anim {

class DemoDebugPanel : public DebugPanelBase {
public:
    const char* name() const override { return "Demo Debug"; }

    void render() override {
        ImGui::Text("Debug framework is working.");
        ImGui::Separator();

        ImGui::Text("Slider test:");
        ImGui::SliderFloat("##demo_slider", &sliderVal_, 0.0f, 1.0f);
        ImGui::Text("Value: %.2f", sliderVal_);

        if (ImGui::Button("Increment")) counter_++;
        ImGui::SameLine();
        ImGui::Text("Counter: %d", counter_);

        ImGui::Checkbox("Show more", &showMore_);
        if (showMore_) {
            ImGui::Text("This panel will be replaced by module-specific");
            ImGui::Text("debug panels on feature branches.");
        }
    }

private:
    float sliderVal_ = 0.5f;
    int counter_ = 0;
    bool showMore_ = false;
};

} // namespace anim
