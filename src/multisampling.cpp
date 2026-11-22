#include "multisampling.h"

#include <raylib.h>
#include <external/glad.h>

MultisampleTarget::~MultisampleTarget() {
    Release();
}

void MultisampleTarget::Release() {
    if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
    if (color_) glDeleteRenderbuffers(1, &color_);
    if (depth_) glDeleteRenderbuffers(1, &depth_);
    framebuffer_ = color_ = depth_ = 0;
    samples_ = 0;
}

bool MultisampleTarget::Allocate(int requestedSamples) {
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glGenRenderbuffers(1, &color_);
    glBindRenderbuffer(GL_RENDERBUFFER, color_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, requestedSamples, GL_RGBA8, width_, height_);
    int colorSamples = 0;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &colorSamples);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_);

    glGenRenderbuffers(1, &depth_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, requestedSamples, GL_DEPTH_COMPONENT24,
                                     width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);
    const bool usable = colorSamples == requestedSamples &&
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!usable) {
        Release();
        return false;
    }
    samples_ = colorSamples;
    return true;
}

bool MultisampleTarget::Begin(int width, int height, int requestedSamples) {
    if (width != width_ || height != height_ || requestedSamples != requested_) {
        Release();
        width_ = width;
        height_ = height;
        requested_ = requestedSamples;
        if (requestedSamples > 0 && width > 0 && height > 0) {
            int maximum = 0;
            glGetIntegerv(GL_MAX_SAMPLES, &maximum);
            for (auto option = MsaaOptions.rbegin(); option != MsaaOptions.rend(); ++option) {
                if (*option > 0 && *option <= requestedSamples && *option <= maximum &&
                    Allocate(*option)) {
                    break;
                }
            }
        }
    }
    if (!framebuffer_) return false;
    RenderTexture2D target{};
    target.id = framebuffer_;
    target.texture.width = width_;
    target.texture.height = height_;
    BeginTextureMode(target);
    return true;
}

void MultisampleTarget::Resolve() const {
    EndTextureMode();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
