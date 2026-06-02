#pragma once
#include <cstdint>

namespace anim {

class CocosEmbed {
public:
    bool init(int width, int height);
    void shutdown();
    void resize(int width, int height);
    void renderFrame();

    uint32_t getTextureId() const { return textureId_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    bool isInitialized() const { return initialized_; }

private:
    uint32_t fbo_ = 0;
    uint32_t textureId_ = 0;
    uint32_t depthBuffer_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;

    void createFBO(int width, int height);
    void destroyFBO();
    void drawTestPattern();
};

} // namespace anim
