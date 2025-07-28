#include "gl_initializers.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

static std::string readFile(const std::string& relativePath) {
    std::string full = std::string(SHADERS_DIR) + "/" + relativePath;
    std::ifstream in(full, std::ios::in | std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open shader: " + full);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    // check for errors
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::string typeName = (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
        std::cerr << typeName << " shader compile error:\n"
            << log.data() << "\n";
        glDeleteShader(shader);
        throw std::runtime_error(typeName + " shader compilation failed");
    }
    return shader;
}


GLuint  glinit::compileProgram(const char* vsPath, const char* fsPath) {
    std::string vertSrc = readFile(vsPath);
    std::string fragSrc = readFile(fsPath);

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Program link error:\n" << log.data() << "\n";
        glDeleteProgram(prog);
        throw std::runtime_error("Shader program linking failed");
    }

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return prog;
}

void glinit::enableDepthTest(bool e) {
    if (e) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
}
void glinit::setBlendMode(BlendMode m) {
    switch (m) {
    case BlendMode::None:     glDisable(GL_BLEND); break;
    case BlendMode::Alpha:    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    case BlendMode::Additive: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
    }
}
