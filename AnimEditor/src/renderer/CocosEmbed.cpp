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

    // TODO: Render Cocos2d-x scene into this FBO
    // cocos2d::Director::getInstance()->mainLoop();

    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

} // namespace anim
