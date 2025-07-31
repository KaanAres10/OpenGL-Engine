#pragma once
#include "gl_types.h"
#include <string>

namespace glloader {
	GLMeshBuffers loadMesh(const std::string& path);
	GLTexture     loadTexture(const std::string& path);
	GLTexture loadTextureMirror(const std::string& path);
	GLTexture loadCubemap(const std::vector<std::string>& faces);
	GLMesh loadQuadWithTexture_Normal();
	GLMesh loadQuadWithTextureNDC();
	GLMesh loadCubeWithTexture();
	GLMesh loadPlaneWithTexture();
	GLMesh loadPlaneWithTexture_Normal();
	GLMesh loadCubeOnlyPosition();
	GLMesh loadCubeWithoutTexture();
	GLMesh loadCubeWithNormal();
	GLMesh loadCubeWithTexture_Normal();
}

