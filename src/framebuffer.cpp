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
            if (m_Specification.Samples > 1) {
                // Multisample texture
                glGenTextures(1, &m_TextureIDs[i]);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_TextureIDs[i]);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                    m_Specification.Samples,
                    att.InternalFormat,
                    m_Specification.Width,
                    m_Specification.Height,
                    GL_TRUE);
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                    att.AttachmentPoint,
                    GL_TEXTURE_2D_MULTISAMPLE,
                    m_TextureIDs[i], 0);
            }
            else {
                if (att.AttachmentPoint == GL_DEPTH_ATTACHMENT) {
                    glGenTextures(1, &m_TextureIDs[i]);
                    glBindTexture(GL_TEXTURE_2D, m_TextureIDs[i]);
                    glTexImage2D(GL_TEXTURE_2D, 0, att.InternalFormat,
                        m_Specification.Width, m_Specification.Height,
                        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    static const GLfloat border[] = { 1,1,1,1 };
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

                    glFramebufferTexture2D(GL_FRAMEBUFFER,
                        GL_DEPTH_ATTACHMENT,
                        GL_TEXTURE_2D,
                        m_TextureIDs[i], 0);
                }
                else {
                    // Regular texture
                    glGenTextures(1, &m_TextureIDs[i]);
                    glBindTexture(GL_TEXTURE_2D, m_TextureIDs[i]);
                    GLenum internalFormat = att.InternalFormat;
                    GLenum format = (internalFormat == GL_RGB8 ? GL_RGB :
                        internalFormat == GL_RGBA8 ? GL_RGBA : GL_RGBA);
                    GLenum type = (internalFormat == GL_RGB16F || internalFormat == GL_RGBA16F ? GL_FLOAT : GL_UNSIGNED_BYTE);
                    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                        m_Specification.Width, m_Specification.Height,
                        0, format, type, nullptr);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glFramebufferTexture2D(GL_FRAMEBUFFER,
                        att.AttachmentPoint,
                        GL_TEXTURE_2D,
                        m_TextureIDs[i], 0);
                }
               
            }
        }
        else {
            glGenRenderbuffers(1, &m_RenderbufferIDs[i]);
            glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferIDs[i]);
            if (m_Specification.Samples > 1) {
                // Multisample renderbuffer
                glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                    m_Specification.Samples,
                    att.InternalFormat,
                    m_Specification.Width,
                    m_Specification.Height);
            }
            else {
                // Regular renderbuffer
                glRenderbufferStorage(GL_RENDERBUFFER,
                    att.InternalFormat,
                    m_Specification.Width,
                    m_Specification.Height);
            }
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
        glReadBuffer(GL_NONE);
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

uint32_t Framebuffer::GetRendererID() const
{
    return m_RendererID;
}

uint32_t Framebuffer::GetTextureID(uint32_t index) const
{
    if (index < m_TextureIDs.size())
        return m_TextureIDs[index];
    return 0;
}