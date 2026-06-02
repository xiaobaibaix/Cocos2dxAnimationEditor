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
    // Generate a checkerboard + crosshair pattern to verify the preview pipeline.
    // This confirms the FBO→texture→ImGui::Image chain is working.
    std::vector<uint8_t> pixels(static_cast<size_t>(width_) * height_ * 4);

    int sq = 32; // checkerboard square size
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            size_t idx = (static_cast<size_t>(y) * width_ + x) * 4;
            bool bright = ((x / sq) + (y / sq)) % 2 == 0;
            uint8_t c = bright ? 58 : 28;
            pixels[idx]     = c;
            pixels[idx + 1] = c;
            pixels[idx + 2] = c;
            pixels[idx + 3] = 255;
        }
    }

    // Draw a red crosshair at the center
    int cx = width_ / 2;
    int cy = height_ / 2;
    int chLen = 40;
    int chW = 2;
    for (int dy = -chLen; dy <= chLen; ++dy) {
        for (int dx = -chW; dx <= chW; ++dx) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < width_ && py >= 0 && py < height_) {
                size_t idx = (static_cast<size_t>(py) * width_ + px) * 4;
                pixels[idx]     = 220;
                pixels[idx + 1] = 40;
                pixels[idx + 2] = 40;
                pixels[idx + 3] = 255;
            }
        }
    }
    for (int dx = -chLen; dx <= chLen; ++dx) {
        for (int dy = -chW; dy <= chW; ++dy) {
            int px = cx + dx;
            int py = cy + dy;
            if (px >= 0 && px < width_ && py >= 0 && py < height_) {
                size_t idx = (static_cast<size_t>(py) * width_ + px) * 4;
                pixels[idx]     = 220;
                pixels[idx + 1] = 40;
                pixels[idx + 2] = 40;
                pixels[idx + 3] = 255;
            }
        }
    }

    // Draw a white border around the full area
    for (int x = 0; x < width_; ++x) {
        for (int bw = 0; bw < 2; ++bw) {
            size_t idxTop = (static_cast<size_t>(bw) * width_ + x) * 4;
            size_t idxBot = (static_cast<size_t>(height_ - 1 - bw) * width_ + x) * 4;
            for (int c = 0; c < 3; ++c) {
                pixels[idxTop + c] = 180;
                pixels[idxBot + c] = 180;
            }
            pixels[idxTop + 3] = 255;
            pixels[idxBot + 3] = 255;
        }
    }
    for (int y = 0; y < height_; ++y) {
        for (int bw = 0; bw < 2; ++bw) {
            size_t idxL = (static_cast<size_t>(y) * width_ + bw) * 4;
            size_t idxR = (static_cast<size_t>(y) * width_ + width_ - 1 - bw) * 4;
            for (int c = 0; c < 3; ++c) {
                pixels[idxL + c] = 180;
                pixels[idxR + c] = 180;
            }
            pixels[idxL + 3] = 255;
            pixels[idxR + 3] = 255;
        }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

} // namespace anim
