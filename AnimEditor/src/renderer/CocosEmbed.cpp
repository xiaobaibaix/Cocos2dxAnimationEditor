#include "renderer/CocosEmbed.h"

// Silence OpenGL deprecation warnings on macOS (OpenGL is deprecated since 10.14)
#if defined(__APPLE__)
    #ifndef GL_SILENCE_DEPRECATION
        #define GL_SILENCE_DEPRECATION
    #endif
#endif

// OpenGL headers — macOS provides OpenGL 3.0 Core declarations via <OpenGL/gl3.h>,
// while other platforms use the standard <GL/gl.h> via GLFW.
#if defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

#include <cmath>
#include <vector>

namespace anim {

bool CocosEmbed::init(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    createFBO(width, height);
    initialized_ = true;
    return true;
}

void CocosEmbed::createFBO(int width, int height) {
    width_ = width;
    height_ = height;

    // Create color attachment texture
    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &depthBuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Assemble FBO
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, textureId_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Framebuffer incomplete — will render to a black texture
    }

    // Clear to editor background color
    glViewport(0, 0, width, height);
    glClearColor(0.086f, 0.086f, 0.118f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CocosEmbed::destroyFBO() {
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (textureId_) {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }
    if (depthBuffer_) {
        glDeleteRenderbuffers(1, &depthBuffer_);
        depthBuffer_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

void CocosEmbed::shutdown() {
    if (initialized_) {
        destroyFBO();
        initialized_ = false;
    }
}

void CocosEmbed::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == width_ && height == height_) return;
    destroyFBO();
    createFBO(width, height);
}

void CocosEmbed::renderFrame() {
    if (!initialized_) return;

    // Save current framebuffer binding
    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);

    glClearColor(0.086f, 0.086f, 0.118f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawTestPattern();

    // TODO: Render Cocos2d-x scene into this FBO
    // cocos2d::Director::getInstance()->mainLoop();

    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

void CocosEmbed::drawTestPattern() {
    std::vector<uint8_t> pixels(static_cast<size_t>(width_) * height_ * 4);

    // Fill background
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i]     = 22;
        pixels[i + 1] = 22;
        pixels[i + 2] = 30;
        pixels[i + 3] = 255;
    }

    float halfW = width_ * 0.5f;
    float halfH = height_ * 0.5f;
    // Negate panY to compensate for UV flip in ImGui::Image
    float effPanY = -panY_;

    auto setPixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            size_t idx = (static_cast<size_t>(y) * width_ + x) * 4;
            pixels[idx]     = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
        }
    };

    // Visible world range for grid line iteration
    float wxMin = (0 - halfW - panX_) / zoom_;
    float wxMax = (width_ - halfW - panX_) / zoom_;
    float wyMin = (0 - halfH - effPanY) / zoom_;
    float wyMax = (height_ - halfH - effPanY) / zoom_;

    int gridMinor = 32;
    int gridMajor = 128;
    int gxStart = static_cast<int>(floorf(wxMin / gridMinor)) - 1;
    int gxEnd = static_cast<int>(ceilf(wxMax / gridMinor)) + 1;
    int gyStart = static_cast<int>(floorf(wyMin / gridMinor)) - 1;
    int gyEnd = static_cast<int>(ceilf(wyMax / gridMinor)) + 1;

    // Vertical grid lines
    for (int gx = gxStart; gx <= gxEnd; ++gx) {
        int sx = static_cast<int>(gx * gridMinor * zoom_ + halfW + panX_);
        bool major = gx * gridMinor % gridMajor == 0;
        uint8_t c = major ? 42 : 32;
        for (int y = 0; y < height_; ++y) {
            setPixel(sx, y, c, c, c + 8);
        }
    }

    // Horizontal grid lines
    for (int gy = gyStart; gy <= gyEnd; ++gy) {
        int sy = static_cast<int>(gy * gridMinor * zoom_ + halfH + effPanY);
        bool major = gy * gridMinor % gridMajor == 0;
        uint8_t c = major ? 42 : 32;
        for (int x = 0; x < width_; ++x) {
            setPixel(x, sy, c, c, c + 8);
        }
    }

    // Axis lines through world origin
    int ox = static_cast<int>(halfW + panX_);
    int oy = static_cast<int>(halfH + effPanY);
    for (int y = 0; y < height_; ++y) setPixel(ox, y, 55, 55, 75);
    for (int x = 0; x < width_; ++x) setPixel(x, oy, 55, 55, 75);

    // Crosshair at world origin
    int chLen = static_cast<int>(20.0f * zoom_);
    int chW = std::max(1, static_cast<int>(1.5f * zoom_));
    for (int d = -chLen; d <= chLen; ++d) {
        for (int w = -chW; w <= chW; ++w) {
            setPixel(ox + d, oy + w, 200, 70, 70);
            setPixel(ox + w, oy + d, 200, 70, 70);
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CocosEmbed::setViewTransform(float zoom, float panX, float panY) {
    zoom_ = zoom;
    panX_ = panX;
    panY_ = panY;
}

} // namespace anim
