#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> 
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader
{
public:
	// program ID
	GLuint ID;

    Shader() = default;
    Shader(const char* vertexPath, const char* fragmentPath)
        : Shader(vertexPath, nullptr, fragmentPath) {}

    Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath) {
        std::string vertCode = readFile(vertexPath);
        std::string fragCode = readFile(fragmentPath);
        const char* vSrc = vertCode.c_str();
        const char* fSrc = fragCode.c_str();

        std::string geomCode;
        const char* gSrc = nullptr;
        if (geometryPath) {
            geomCode = readFile(geometryPath);
            gSrc = geomCode.c_str();
        }

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vSrc, nullptr);
        glCompileShader(vs);
        checkCompileErrors(vs, "VERTEX");

        GLuint gs = 0;
        if (gSrc) {
            gs = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(gs, 1, &gSrc, nullptr);
            glCompileShader(gs);
            checkCompileErrors(gs, "GEOMETRY");
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fSrc, nullptr);
        glCompileShader(fs);
        checkCompileErrors(fs, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vs);
        if (gSrc) glAttachShader(ID, gs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vs);
        if (gSrc) glDeleteShader(gs);
        glDeleteShader(fs);
    }

    void use() const { glUseProgram(ID); }

	// uniform functions
    void setBool(const std::string& name, bool  v) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)v);
    }
    void setInt(const std::string& name, int   v) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), v);
    }
    void setFloat(const std::string& name, float v) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), v);
    }
    void setVec2(const std::string& name, glm::vec2 vec) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()),
            1, glm::value_ptr(vec));
    }
    void setVec3(const std::string& name, glm::vec3 vec) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()),
            1, glm::value_ptr(vec));
    }
    void setMat4(const std::string& name, glm::mat4 mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
            1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    static std::string readFile(const std::string& relativePath) {
        std::string full = std::string(SHADERS_DIR) + "/" + relativePath;
        std::ifstream in(full, std::ios::in | std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open shader: " + full);
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    static void checkCompileErrors(GLuint obj, const std::string& type) {
        GLint success;
        GLchar log[1024];
        if (type == "PROGRAM") {
            glGetProgramiv(obj, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(obj, 1024, nullptr, log);
                std::cerr << "ERROR::PROGRAM_LINKING_FAILED\n" << log << "\n";
            }
        }
        else {
            glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(obj, 1024, nullptr, log);
                std::cerr << "ERROR::" << type
                    << "_COMPILATION_FAILED\n" << log << "\n";
            }
        }
    }
};
#endif