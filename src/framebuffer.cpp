#include "Framebuffer.h"

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec)
{
    Invalidate();
}

Framebuffer::~Framebuffer()
{
    if (m_RendererID)
        glDeleteFramebuffers(1, &m_RendererID);
    if (!m_TextureIDs.empty())
        glDeleteTextures((GLsizei)m_TextureIDs.size(), m_TextureIDs.data());
    if (!m_RenderbufferIDs.empty())
        glDeleteRenderbuffers((GLsizei)m_RenderbufferIDs.size(), m_RenderbufferIDs.data());
}

void Framebuffer::Invalidate()
{
    // Cleanup existing
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        if (!m_TextureIDs.empty()) glDeleteTextures((GLsizei)m_TextureIDs.size(), m_TextureIDs.data());
        if (!m_RenderbufferIDs.empty()) glDeleteRenderbuffers((GLsizei)m_RenderbufferIDs.size(), m_RenderbufferIDs.data());
        m_TextureIDs.clear();
        m_RenderbufferIDs.clear();
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    size_t count = m_Specification.Attachments.size();
    m_TextureIDs.resize(count, 0);
    m_RenderbufferIDs.resize(count, 0);

    // Setup each attachment
    for (size_t i = 0; i < count; ++i) {
        const auto& att = m_Specification.Attachments[i];
        if (att.Type == FramebufferAttachmentType::Texture2D) {
            glGenTextures(1, &m_TextureIDs[i]);
            glBindTexture(GL_TEXTURE_2D, m_TextureIDs[i]);

            // Determine base format and type from internal format
            GLenum internalFormat = att.InternalFormat;
            GLenum format = GL_RGBA;
            GLenum type = GL_UNSIGNED_BYTE;
            switch (internalFormat) {
            case GL_RGB8:   format = GL_RGB;   break;
            case GL_RGBA8:  format = GL_RGBA;  break;
            case GL_RGB16F: format = GL_RGB;   type = GL_FLOAT; break;
            case GL_RGBA16F:format = GL_RGBA;  type = GL_FLOAT; break;
            case GL_DEPTH24_STENCIL8:
                format = GL_DEPTH_STENCIL;
                type = GL_UNSIGNED_INT_24_8;
                break;
            }

            glTexImage2D(GL_TEXTURE_2D,
                0,
                internalFormat,
                m_Specification.Width,
                m_Specification.Height,
                0,
                format,
                type,
                nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER,
                att.AttachmentPoint,
                GL_TEXTURE_2D,
                m_TextureIDs[i],
                0);
        }
        else {
            glGenRenderbuffers(1, &m_RenderbufferIDs[i]);
            glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferIDs[i]);
            glRenderbufferStorage(GL_RENDERBUFFER,
                att.InternalFormat,
                m_Specification.Width,
                m_Specification.Height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                att.AttachmentPoint,
                GL_RENDERBUFFER,
                m_RenderbufferIDs[i]);
        }
    }

    // Set draw buffers for color attachments
    std::vector<GLenum> drawBuffers;
    for (const auto& att : m_Specification.Attachments) {
        if (att.AttachmentPoint >= GL_COLOR_ATTACHMENT0 && att.AttachmentPoint <= GL_COLOR_ATTACHMENT31) {
            drawBuffers.push_back(att.AttachmentPoint);
        }
    }
    if (!drawBuffers.empty()) {
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
    }
    else {
        glDrawBuffer(GL_NONE);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is incomplete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

uint32_t Framebuffer::GetTextureID(uint32_t index) const
{
    if (index < m_TextureIDs.size())
        return m_TextureIDs[index];
    return 0;
}