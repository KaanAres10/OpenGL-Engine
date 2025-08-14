#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_loader.h"
#include <stdexcept>
#include <random>
#include <iostream>
#include <framebuffer.h>
#include <shader.h>

GLMeshBuffers glloader::loadMesh(const std::string& path)
{
    // TODO 
    return GLMeshBuffers();
}

GLTexture glloader::loadTexture(const std::string& path, bool gammaCorrection) {
    GLTexture tex{};

    stbi_set_flip_vertically_on_load(true);

    // For HDR
    if (stbi_is_hdr(path.c_str())) {
        int nrChannels = 0;
        float* data = stbi_loadf(path.c_str(), &tex.width, &tex.height, &nrChannels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load HDR texture: " + path);
        }

        GLenum dataFormat, internalFormat;
        if (nrChannels == 1) { dataFormat = GL_RED;  internalFormat = GL_R16F; }
        else if (nrChannels == 3) { dataFormat = GL_RGB;  internalFormat = GL_RGB16F; }
        else if (nrChannels == 4) { dataFormat = GL_RGBA; internalFormat = GL_RGBA16F; }
        else {
            stbi_image_free(data);
            throw std::runtime_error("Unexpected HDR channel count (" +
                std::to_string(nrChannels) + ") in: " + path);
        }

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
            tex.width, tex.height, 0,
            dataFormat, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        return tex;
    }

    // For LDR
    int nrChannels = 0;
    unsigned char* data = stbi_load(path.c_str(),
        &tex.width,
        &tex.height,
        &nrChannels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    // generate & bind
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    // standard parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum internalFormat, dataFormat;
    if (nrChannels == 1) {
        internalFormat = dataFormat = GL_RED;
    }
    else if (nrChannels == 3) {
        internalFormat = gammaCorrection ? GL_SRGB8 : GL_RGB8;
        dataFormat = GL_RGB;
    }
    else if (nrChannels == 4) {
        internalFormat = gammaCorrection ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        dataFormat = GL_RGBA;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else {
        stbi_image_free(data);
        throw std::runtime_error("Unexpected channel count (" +
            std::to_string(nrChannels) +
            ") in: " + path);
    }

    // upload & mipmaps
    glTexImage2D(GL_TEXTURE_2D,
        0,
        internalFormat,
        tex.width,
        tex.height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return tex;
}

GLTexture glloader::loadTextureMirror(const std::string& path, bool gammaCorrection) {
    GLTexture tex{};

    stbi_set_flip_vertically_on_load(true);

    int nrChannels = 0;
    unsigned char* data = stbi_load(path.c_str(),
        &tex.width,
        &tex.height,
        &nrChannels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    // generate & bind
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    // standard parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum internalFormat, dataFormat;
    if (nrChannels == 1) {
        internalFormat = dataFormat = GL_RED;
    }
    else if (nrChannels == 3) {
        internalFormat = gammaCorrection ? GL_SRGB8 : GL_RGB8;
        dataFormat = GL_RGB;
    }
    else if (nrChannels == 4) {
        internalFormat = gammaCorrection ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        dataFormat = GL_RGBA;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else {
        stbi_image_free(data);
        throw std::runtime_error("Unexpected channel count (" +
            std::to_string(nrChannels) +
            ") in: " + path);
    }

    // upload & mipmaps
    glTexImage2D(GL_TEXTURE_2D,
        0,
        internalFormat,
        tex.width,
        tex.height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return tex;
}

GLTexture glloader::loadCubemap(const std::vector<std::string>& faces, bool gammaCorrection) {
    GLTexture tex{};
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex.id);

    stbi_set_flip_vertically_on_load(false);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(
            faces[i].c_str(),
            &width, &height, &nrChannels, 0
        );
        if (!data) {
            throw std::runtime_error("Failed to load cubemap face: " + faces[i]);
        }

        GLenum internalFormat, dataFormat;
        if (nrChannels == 1) {
            internalFormat = dataFormat = GL_R8;
        }
        else if (nrChannels == 3) {
            internalFormat = gammaCorrection
                ? GL_SRGB8
                : GL_RGB8;
            dataFormat = GL_RGB;
        }
        else if (nrChannels == 4) {
            internalFormat = gammaCorrection
                ? GL_SRGB8_ALPHA8
                : GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else {
            stbi_image_free(data);
            throw std::runtime_error(
                "Unexpected channel count (" +
                std::to_string(nrChannels) +
                ") in cubemap face: " + faces[i]
            );
        }

        // Allocate the face
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, internalFormat,
            width, height,
            0, dataFormat,
            GL_UNSIGNED_BYTE, data
        );
        stbi_image_free(data);
    }

    stbi_set_flip_vertically_on_load(true);


    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return tex;
}

GLTexture glloader::loadDepthCubemap(GLuint width, GLuint height) {
    GLTexture tex{};
    tex.width = width;
    tex.height = height;
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex.id);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_DEPTH_COMPONENT24,
            width,
            height,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}

GLMesh glloader::loadQuadWithTexture_Normal() {
    GLMesh m{};
    float quadVertices[] = {
        // positions        // normals         // texCoords
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         0.0f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         1.0f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,

         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         1.0f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         1.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadQuadWithTextureNDC() {
    GLMesh m{};
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadQuadWithColorNDC() {
    GLMesh m{};
    float quadVertices[] = {
        // positions     // colors
        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
        -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
         0.05f,  0.05f,  0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    std::vector<glm::vec2> translations;
    translations.reserve(100);
    int index = 0;
    float offset = 0.1f;
    for (int y = -10; y < 10; y += 2)
    {
        for (int x = -10; x < 10; x += 2)
        {
            translations.emplace_back(
                (float)x / 10.0f + offset,
                (float)y / 10.0f + offset
            );
        }
    }

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
        translations.size() * sizeof(glm::vec2),
        translations.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec2), (void*)0
    );
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadCube()
{
    GLMesh m{};
    const float vertices[] = {
        // back face
        -1.0f,-1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 0.0f,0.0f,
         1.0f, 1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 1.0f,1.0f,
         1.0f,-1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 1.0f,0.0f,
         1.0f, 1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 1.0f,1.0f,
        -1.0f,-1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 0.0f,0.0f,
        -1.0f, 1.0f,-1.0f,  0.0f, 0.0f,-1.0f, 0.0f,1.0f,
        // front face
        -1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f,0.0f,
         1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,0.0f,
         1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,1.0f,
         1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,1.0f,
        -1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f,1.0f,
        -1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f,0.0f,
        // left face
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f,0.0f,
        -1.0f, 1.0f,-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,1.0f,
        -1.0f,-1.0f,-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,1.0f,
        -1.0f,-1.0f,-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,1.0f,
        -1.0f,-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f,0.0f,
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f,0.0f,
        // right face
         1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,0.0f,
         1.0f,-1.0f,-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,1.0f,
         1.0f, 1.0f,-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,1.0f,
         1.0f,-1.0f,-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,1.0f,
         1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,0.0f,
         1.0f,-1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 0.0f,0.0f,
         // bottom face
         -1.0f,-1.0f,-1.0f,  0.0f,-1.0f, 0.0f, 0.0f,1.0f,
          1.0f,-1.0f,-1.0f,  0.0f,-1.0f, 0.0f, 1.0f,1.0f,
          1.0f,-1.0f, 1.0f,  0.0f,-1.0f, 0.0f, 1.0f,0.0f,
          1.0f,-1.0f, 1.0f,  0.0f,-1.0f, 0.0f, 1.0f,0.0f,
         -1.0f,-1.0f, 1.0f,  0.0f,-1.0f, 0.0f, 0.0f,0.0f,
         -1.0f,-1.0f,-1.0f,  0.0f,-1.0f, 0.0f, 0.0f,1.0f,
         // top face
         -1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f, 0.0f,1.0f,
          1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,0.0f,
          1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f, 1.0f,1.0f,
          1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,0.0f,
         -1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f, 0.0f,1.0f,
         -1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,0.0f
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMesh glloader::loadCubeWithTexture() {
    GLMesh m{};
    float vertices[] = {
     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
      0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

     -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMesh glloader::loadPlaneWithTexture() {
    GLMesh m{};
    float planeVertices[] = {
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadPlaneWithTexture_Normal() {
    GLMesh m{};
    float planeVertices[] = {
        // positions         // normals       // texcoords
         5.0f, -0.5f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

         5.0f, -0.5f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadPlaneWithTexture_Normal_Tangent() {
    GLMesh m{};

    // positions
    glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
    glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
    glm::vec3 pos3(1.0f, -1.0f, 0.0f);
    glm::vec3 pos4(1.0f, 1.0f, 0.0f);

    // texture coordinates
    glm::vec2 uv1(0.0f, 1.0f);
    glm::vec2 uv2(0.0f, 0.0f);
    glm::vec2 uv3(1.0f, 0.0f);
    glm::vec2 uv4(1.0f, 1.0f);

    // normal vector
    glm::vec3 nm(0.0f, 0.0f, 1.0f);

    glm::vec3 tangent1, bitangent1;
    glm::vec3 tangent2, bitangent2;

    // --- Triangle 1 ---
    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent1 = glm::normalize(glm::vec3(
        f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
        f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
        f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
    ));

    bitangent1 = glm::normalize(glm::vec3(
        f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
        f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
        f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
    ));

    // --- Triangle 2 ---
    edge1 = pos3 - pos1;
    edge2 = pos4 - pos1;
    deltaUV1 = uv3 - uv1;
    deltaUV2 = uv4 - uv1;

    f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent2 = glm::normalize(glm::vec3(
        f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
        f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
        f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
    ));

    bitangent2 = glm::normalize(glm::vec3(
        f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
        f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
        f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
    ));

    float vertices[] = {
        // pos              // normal     // uv        // tangent         // bitangent
        pos1.x,pos1.y,pos1.z, nm.x,nm.y,nm.z, uv1.x,uv1.y, tangent1.x,tangent1.y,tangent1.z, bitangent1.x,bitangent1.y,bitangent1.z,
        pos2.x,pos2.y,pos2.z, nm.x,nm.y,nm.z, uv2.x,uv2.y, tangent1.x,tangent1.y,tangent1.z, bitangent1.x,bitangent1.y,bitangent1.z,
        pos3.x,pos3.y,pos3.z, nm.x,nm.y,nm.z, uv3.x,uv3.y, tangent1.x,tangent1.y,tangent1.z, bitangent1.x,bitangent1.y,bitangent1.z,

        pos1.x,pos1.y,pos1.z, nm.x,nm.y,nm.z, uv1.x,uv1.y, tangent2.x,tangent2.y,tangent2.z, bitangent2.x,bitangent2.y,bitangent2.z,
        pos3.x,pos3.y,pos3.z, nm.x,nm.y,nm.z, uv3.x,uv3.y, tangent2.x,tangent2.y,tangent2.z, bitangent2.x,bitangent2.y,bitangent2.z,
        pos4.x,pos4.y,pos4.z, nm.x,nm.y,nm.z, uv4.x,uv4.y, tangent2.x,tangent2.y,tangent2.z, bitangent2.x,bitangent2.y,bitangent2.z
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    constexpr GLsizei STRIDE = 14 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);                  // position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float))); // normal
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(6 * sizeof(float))); // uv
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(8 * sizeof(float))); // tangent
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(11 * sizeof(float))); // bitangent
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    m.vertexCount = 6;
    return m;
}

GLMesh glloader::loadCubeOnlyPosition() {
    GLMesh m{};
    float vertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMesh glloader::loadPointsNDC() {
    GLMesh m{};
    float vertices[] = {
    -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // top-left
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // top-right
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // bottom-right
    -0.5f, -0.5f, 1.0f, 1.0f, 0.0f  // bottom-left
    };

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m.vertexCount = 4;
    return m;
}

GLMesh glloader::loadCubeWithoutTexture() {
    GLMesh m{};
    float vertices[] = {
     -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

     -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMesh glloader::loadCubeWithNormal() {
    GLMesh m{};
    float vertices[] = {
     -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

     -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMesh glloader::loadCubeWithTexture_Normal() {
    GLMesh m{};
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,

         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
          0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f
    };
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    m.vertexCount = 36;
    return m;
}

GLMeshBuffers glloader::loadSphere(unsigned X_SEGMENTS, unsigned Y_SEGMENTS)
{
    GLMeshBuffers m{};
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;

    positions.reserve((X_SEGMENTS + 1) * (Y_SEGMENTS + 1));
    normals.reserve(positions.capacity());
    uvs.reserve(positions.capacity());
    indices.reserve(Y_SEGMENTS * (X_SEGMENTS + 1) * 2);

    const float PI = 3.14159265359f;

    for (unsigned y = 0; y <= Y_SEGMENTS; ++y) {
        float v = float(y) / float(Y_SEGMENTS);
        float phi = v * PI;
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (unsigned x = 0; x <= X_SEGMENTS; ++x) {
            float u = float(x) / float(X_SEGMENTS);
            float theta = u * 2.0f * PI;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            float xPos = cosTheta * sinPhi;
            float yPos = cosPhi;
            float zPos = sinTheta * sinPhi;

            positions.emplace_back(xPos, yPos, zPos);
            normals.emplace_back(xPos, yPos, zPos);     // unit sphere
            uvs.emplace_back(u, v);
        }
    }

    bool oddRow = false;
    for (unsigned y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (unsigned x = 0; x <= X_SEGMENTS; ++x) {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else {
            for (int x = int(X_SEGMENTS); x >= 0; --x) {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + unsigned(x));
                indices.push_back(y * (X_SEGMENTS + 1) + unsigned(x));
            }
        }
        oddRow = !oddRow;
    }
    m.indexCount = indices.size();

    std::vector<float> interleaved;
    interleaved.reserve(positions.size() * 8);
    for (size_t i = 0; i < positions.size(); ++i) {
        interleaved.push_back(positions[i].x);
        interleaved.push_back(positions[i].y);
        interleaved.push_back(positions[i].z);
        interleaved.push_back(normals[i].x);
        interleaved.push_back(normals[i].y);
        interleaved.push_back(normals[i].z);
        interleaved.push_back(uvs[i].x);
        interleaved.push_back(uvs[i].y);
    }

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        interleaved.size() * sizeof(float),
        interleaved.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    return m;
}


GLTexture glloader::equirectangularToCubemap(GLuint hdr2D, int size)
{
    GLTexture cube{};
    cube.width = size;
    cube.height = size;

    glGenTextures(1, &cube.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube.id);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    FramebufferSpecification capSpec{};
    capSpec.Width = size;
    capSpec.Height = size;
    capSpec.Samples = 1;
    capSpec.Attachments = {
        { FramebufferAttachmentType::TextureCubeMap, GL_RGB16F,        GL_COLOR_ATTACHMENT0 },
        { FramebufferAttachmentType::Renderbuffer,   GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT }
    };

    std::unique_ptr<Framebuffer> captureFBO = std::make_unique<Framebuffer>(capSpec);

    captureFBO->Bind();

    Shader eqToCube("equirect_to_cubemap.vert",
        "equirect_to_cubemap.frag");
    eqToCube.use();
    eqToCube.setInt("equirectangularMap", 0);

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };
    eqToCube.setMat4("projection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr2D);


    GLMesh cubeMesh = glloader::loadCubeOnlyPosition();

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glViewport(0, 0, size, size);

    glBindVertexArray(cubeMesh.vao);
    for (int face = 0; face < 6; ++face) {
        eqToCube.setMat4("view", views[face]);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            cube.id, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)cubeMesh.vertexCount);
    }
    glBindVertexArray(0);

    captureFBO->Unbind();
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube.id);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return cube;
}

GLTexture glloader::convolveIrradiance(GLuint envCubemap /* RGB16F cubemap */, int size)
{
    GLTexture irr{};
    irr.width = size;
    irr.height = size;

    glGenTextures(1, &irr.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irr.id);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    FramebufferSpecification capSpec{};
    capSpec.Width = size;
    capSpec.Height = size;
    capSpec.Samples = 1;
    capSpec.Attachments = {
        { FramebufferAttachmentType::TextureCubeMap, GL_RGB16F,          GL_COLOR_ATTACHMENT0 },
        { FramebufferAttachmentType::Renderbuffer,   GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT }
    };
    std::unique_ptr<Framebuffer> captureFBO = std::make_unique<Framebuffer>(capSpec);

    captureFBO->Bind();

    Shader irradiance("irradiance_convolution.vert",  
        "irradiance_convolution.frag");
    irradiance.use();
    irradiance.setInt("environmentMap", 0);

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };
    irradiance.setMat4("projection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    GLMesh cubeMesh = glloader::loadCube();

    GLint oldViewport[4]; glGetIntegerv(GL_VIEWPORT, oldViewport);
    glViewport(0, 0, size, size);

    glBindVertexArray(cubeMesh.vao);
    for (int face = 0; face < 6; ++face) {
        irradiance.setMat4("view", views[face]);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            irr.id, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)cubeMesh.vertexCount);
    }
    glBindVertexArray(0);

    captureFBO->Unbind();
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    glBindTexture(GL_TEXTURE_CUBE_MAP, irr.id);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return irr;
}

std::vector<glm::vec3> glloader::makeSSAOKernel(int K, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U01(0.0f, 1.0f);

    std::vector<glm::vec3> kernel;
    kernel.reserve(K);

    for (int i = 0; i < K; ++i) {
        glm::vec3 s(
            U01(rng) * 2.0f - 1.0f, // x in [-1,1]
            U01(rng) * 2.0f - 1.0f, // y in [-1,1]
            U01(rng)               // z in [0,1]
        );
        s = glm::normalize(s);

        float r = U01(rng);                
        float t = float(i) / float(K);     
        float scale = 0.1f + 0.9f * (t * t); 
        s *= (r * r) * scale;

        kernel.push_back(s);
    }
    return kernel;
}

GLTexture glloader::createSSAONoiseTexture(int side, unsigned seed) {
    GLTexture tex{};
    tex.width = side;
    tex.height = side;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(-1.0f, 1.0f);

    std::vector<glm::vec3> noise(side * side);
    for (auto& v : noise) v = glm::vec3(U(rng), U(rng), 0.0f); 

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, side, side,
        0, GL_RGB, GL_FLOAT, noise.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return tex;
}

