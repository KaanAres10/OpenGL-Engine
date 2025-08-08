#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <system_error>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    GLuint ID = 0;

    Shader() = default;

    Shader(const char* vertexPath, const char* fragmentPath)
        : Shader(vertexPath, nullptr, fragmentPath) {
    }

    Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
        : vPath(vertexPath ? vertexPath : ""),
        gPath(geometryPath ? geometryPath : ""),
        fPath(fragmentPath ? fragmentPath : "")
    {
        std::string log;
        GLuint prog = 0;
        if (!build(prog, log)) {
            std::cerr << "[Shader] initial build failed:\n" << log << "\n";
        }
        else {
            ID = prog;
            refreshMtimes();
        }
    }

    void use() const { glUseProgram(ID); }

    // Poll this once per frame, returns true if a new program was swapped in.
    bool reloadIfChanged() {
        if (!watchEnabled) return false;
        if (debounceFrames) { --debounceFrames; return false; }
        if (!hasChanged())  return false;

        std::string log;
        GLuint prog = 0;
        if (!build(prog, log)) {
            lastErrorLog = log;
            std::cerr << "[Shader Reload Failed]\n" << lastErrorLog << "\n";
            // keep old program alive, wait a little before trying again
            debounceFrames = 30;
            return false;
        }
        // swap
        GLuint old = ID;
        ID = prog;
        if (old) glDeleteProgram(old);
        refreshMtimes();
        debounceFrames = 10;
        lastErrorLog.clear();
        return true;
    }

    bool forceReload() {
        std::string log;
        GLuint prog = 0;
        if (!build(prog, log)) {
            lastErrorLog = log;
            return false;
        }
        GLuint old = ID;
        ID = prog;
        if (old) glDeleteProgram(old);
        refreshMtimes();
        lastErrorLog.clear();
        return true;
    }

    void setWatchEnabled(bool on) { watchEnabled = on; }
    const std::string& getLastError() const { return lastErrorLog; }

    void setBool(const std::string& name, bool  v) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)v); }
    void setInt(const std::string& name, int   v) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), v); }
    void setFloat(const std::string& name, float v) const { glUniform1f(glGetUniformLocation(ID, name.c_str()), v); }
    void setVec2(const std::string& name, glm::vec2 v) const { glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v)); }
    void setVec3(const std::string& name, glm::vec3 v) const { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v)); }
    void setMat4(const std::string& name, glm::mat4 m) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    std::string vPath, gPath, fPath;

    std::unordered_map<std::string, std::filesystem::file_time_type> mtimes;

    bool        watchEnabled = true;
    unsigned    debounceFrames = 0;
    std::string lastErrorLog;

    static std::string readFile(const std::string& relativePath) {
        std::string full = std::string(SHADERS_DIR) + "/" + relativePath;
        std::ifstream in(full, std::ios::in | std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open shader: " + full);
        std::ostringstream ss; ss << in.rdbuf();
        return ss.str();
    }

    static GLuint compile(GLenum stage, const char* src, const char* debugName, std::string& log) {
        GLuint sh = glCreateShader(stage);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = GL_FALSE;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[2048]; GLsizei len = 0;
            glGetShaderInfoLog(sh, (GLsizei)sizeof(buf), &len, buf);
            log += "[Compile] " + std::string(debugName ? debugName : "unknown") + ":\n";
            log.append(buf, buf + len);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    }

    static bool link(GLuint prog, std::string& log) {
        glLinkProgram(prog);
        GLint ok = GL_FALSE;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[2048]; GLsizei len = 0;
            glGetProgramInfoLog(prog, (GLsizei)sizeof(buf), &len, buf);
            log += "[Link]\n"; log.append(buf, buf + len);
            return false;
        }
        return true;
    }

    bool build(GLuint& outProgram, std::string& outLog) {
        outLog.clear();
        const std::string vsrc = vPath.empty() ? "" : readFile(vPath);
        const std::string fsrc = fPath.empty() ? "" : readFile(fPath);
        const std::string gsrc = gPath.empty() ? "" : readFile(gPath);

        if (vsrc.empty() ) {
            outLog = "Missing vertex.";
            return false;
        }
        if (fsrc.empty()) {
            outLog = "Missing fragment source.";
            return false;
        }

        GLuint vs = compile(GL_VERTEX_SHADER, vsrc.c_str(), vPath.c_str(), outLog);
        if (!vs) return false;

        GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc.c_str(), fPath.c_str(), outLog);
        if (!fs) { glDeleteShader(vs); return false; }

        GLuint gs = 0;
        if (!gsrc.empty()) {
            gs = compile(GL_GEOMETRY_SHADER, gsrc.c_str(), gPath.c_str(), outLog);
            if (!gs) { glDeleteShader(vs); glDeleteShader(fs); return false; }
        }

        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        if (gs) glAttachShader(prog, gs);

        bool ok = link(prog, outLog);
        glDeleteShader(vs); glDeleteShader(fs); if (gs) glDeleteShader(gs);

        if (!ok) { glDeleteProgram(prog); return false; }
        outProgram = prog;
        return true;
    }

    bool fileChanged(const std::string& rel) const {
        if (rel.empty()) return false;
        std::error_code ec;
        auto now = std::filesystem::last_write_time(std::string(SHADERS_DIR) + "/" + rel, ec);
        if (ec) return false;
        auto it = mtimes.find(rel);
        return it == mtimes.end() || it->second != now;
    }

    bool hasChanged() const {
        return fileChanged(vPath) || fileChanged(fPath) || fileChanged(gPath);
    }

    void refreshMtimes() {
        auto put = [&](const std::string& rel) {
            if (rel.empty()) return;
            std::error_code ec;
            mtimes[rel] = std::filesystem::last_write_time(std::string(SHADERS_DIR) + "/" + rel, ec);
            };
        put(vPath); put(fPath); put(gPath);
    }
};

#endif 
