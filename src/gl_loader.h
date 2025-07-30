#pragma once
#include "gl_types.h"
#include <string>

namespace glloader {
	GLMeshBuffers loadMesh(const std::string& path);
	GLTexture     loadTexture(const std::string& path);
	GLTexture loadTextureMirror(const std::string& path);
	GLMesh loadCubeWithTexture();
	GLMesh loadPlaneWithTexture();
	GLMesh loadPlaneWithTexture_Normal();
	GLMesh loadCubeWithoutTexture();
	GLMesh loadCubeWithNormal();
	GLMesh loadCubeWithTexture_Normal();
}

