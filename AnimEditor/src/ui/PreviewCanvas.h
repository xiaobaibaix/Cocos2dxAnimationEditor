#pragma once
#include "renderer/CocosEmbed.h"
#include <functional>

namespace anim {

class PreviewCanvas {
public:
    using OnNodeDragged = std::function<void(float dx, float dy)>;

    bool init(int width, int height);
    void shutdown();
    void render();

    CocosEmbed& getCocosEmbed() { return embed_; }

    void setOnNodeDragged(OnNodeDragged cb) { onNodeDragged_ = std::move(cb); }
    bool isHovered() const { return hovered_; }

private:
    CocosEmbed embed_;
    OnNodeDragged onNodeDragged_;
    bool hovered_ = false;
    bool dragging_ = false;
};

} // namespace anim
