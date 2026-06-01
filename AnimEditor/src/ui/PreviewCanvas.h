#pragma once
#include "renderer/CocosEmbed.h"

namespace anim {

class PreviewCanvas {
public:
    bool init(int width, int height);
    void shutdown();
    void render();

    CocosEmbed& getCocosEmbed() { return embed_; }

private:
    CocosEmbed embed_;
};

} // namespace anim
