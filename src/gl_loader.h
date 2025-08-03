#pragma once
#include "gl_types.h"
#include <string>

namespace glloader {
	GLMeshBuffers loadMesh(const std::string& path);
	GLTexture     loadTexture(const std::string& path, bool gammaCorrection = false);
	GLTexture loadTextureMirror(const std::string& path, bool gammaCorrection = false);
	GLTexture loadCubemap(const std::vector<std::string>& faces, bool gammaCorrection = false);
	GLMesh loadQuadWithTexture_Normal();
	GLMesh loadQuadWithTextureNDC();
	GLMesh loadQuadWithColorNDC();
	GLMesh loadCubeWithTexture();
	GLMesh loadPlaneWithTexture();
	GLMesh loadPlaneWithTexture_Normal();
	GLMesh loadCubeOnlyPosition();
	GLMesh loadPointsNDC();
	GLMesh loadCubeWithoutTexture();
	GLMesh loadCubeWithNormal();
	GLMesh loadCubeWithTexture_Normal();
}

