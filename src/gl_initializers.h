#pragma once
#include <string>
#include "gl_types.h"

namespace glinit {
    GLuint    compileProgram(const char* vsPath, const char* fsPath);
    
    // state helpers
    void enableDepthTest(bool enable = true);
    void setBlendMode(BlendMode mode);
}
