#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>

struct GLMeshBuffers {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    size_t   indexCount;
};

struct GLMesh {
    GLuint vao, vbo;
    size_t vertexCount;
};

struct GLTexture {
    GLuint id;
    int    width, height;
};

enum class BlendMode { None, Alpha, Additive };

struct MaterialInstance {
    GLuint    program;       
    BlendMode blend;
    GLint     uModel, uView, uProj;
    //any other uniform locations
};

struct RenderObject {
    GLMeshBuffers   mesh;
    MaterialInstance material;
    glm::mat4        modelMatrix;
};

struct DrawContext {
    std::vector<RenderObject> opaque;
    std::vector<RenderObject> transparent;
};
