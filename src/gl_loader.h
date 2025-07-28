#pragma once
#include "gl_types.h"
#include <string>

namespace glloader {
	GLMeshBuffers loadMesh(const std::string& path);
	GLTexture     loadTexture(const std::string& path);
	GLMesh loadCube();
}

