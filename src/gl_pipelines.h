#pragma once
#include "shader.h"          
#include "gl_initializers.h" 

struct GLPipeline {
    Shader    shader;      
    BlendMode blend = BlendMode::None;
    bool      depthTest = true;
    GLenum    cullFace = GL_BACK;
    GLenum    polygonMode = GL_FILL;

    GLPipeline() = default;

    GLPipeline(Shader&& s,
        BlendMode bm = BlendMode::None,
        bool      dt = true,
        GLenum    cf = GL_BACK,
        GLenum    pm = GL_FILL)
        : shader(std::move(s))
        , blend(bm)
        , depthTest(dt)
        , cullFace(cf)
        , polygonMode(pm)
    {
    }

    void apply() const {
        shader.use();
        glinit::enableDepthTest(depthTest);
        glinit::setBlendMode(blend);
        if (cullFace != GL_NONE) {
            glEnable(GL_CULL_FACE);
            glCullFace(cullFace);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    }

    void setViewProj(const glm::mat4& view,
        const glm::mat4& proj) const
    {
        shader.setMat4("view", view);
        shader.setMat4("projection", proj);
    }

    void setModel(const glm::mat4& model) const {
        shader.setMat4("model", model);
    }
};
