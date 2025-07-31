#pragma once

#include <vector>
#include <cstdint>
#include <glad/glad.h>
#include <iostream>

// Specifies texture or renderbuffer for an attachment
enum class FramebufferAttachmentType { Texture2D, Renderbuffer };

// Defines one attachment
struct FramebufferAttachmentSpecification {
    FramebufferAttachmentType Type = FramebufferAttachmentType::Texture2D;
    GLenum                    InternalFormat = GL_RGBA8;
    GLenum                    AttachmentPoint = GL_COLOR_ATTACHMENT0;
};

struct FramebufferSpecification {
    uint32_t Width = 0;
    uint32_t Height = 0;
    bool     SwapChainTarget = false;
    std::vector<FramebufferAttachmentSpecification> Attachments;
};

class Framebuffer {
public:
    explicit Framebuffer(const FramebufferSpecification& spec);
    ~Framebuffer();

    void Invalidate();           // (Re)creates resources based on spec
    void Bind() const;
    void Unbind() const;

    void Resize(uint32_t width, uint32_t height);

    // Returns texture ID for texture attachments only
    uint32_t GetTextureID(uint32_t index = 0) const;

private:
    uint32_t m_RendererID = 0;
    FramebufferSpecification m_Specification;

    // Store generated IDs 
    std::vector<uint32_t> m_TextureIDs;
    std::vector<uint32_t> m_RenderbufferIDs;
};
