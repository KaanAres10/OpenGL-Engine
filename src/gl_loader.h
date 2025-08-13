#pragma once
#include "gl_types.h"
#include <string>

namespace glloader {
	GLMeshBuffers loadMesh(const std::string& path);
	GLTexture     loadTexture(const std::string& path, bool gammaCorrection = false);
	GLTexture loadTextureMirror(const std::string& path, bool gammaCorrection = false);
	GLTexture loadCubemap(const std::vector<std::string>& faces, bool gammaCorrection = false);
	GLTexture loadDepthCubemap(GLuint width, GLuint height);
	GLMesh loadQuadWithTexture_Normal();
	GLMesh loadQuadWithTextureNDC();
	GLMesh loadQuadWithColorNDC();
	GLMesh loadCubeWithTexture();
	GLMesh loadPlaneWithTexture();
	GLMesh loadPlaneWithTexture_Normal();
	GLMesh loadPlaneWithTexture_Normal_Tangent();
	GLMesh loadCubeOnlyPosition();
	GLMesh loadPointsNDC();
	GLMesh loadCubeWithoutTexture();
	GLMesh loadCubeWithNormal();
	GLMesh loadCubeWithTexture_Normal();
	GLMeshBuffers loadSphere(unsigned X_SEGMENTS, unsigned Y_SEGMENTS);
	std::vector<glm::vec3> makeSSAOKernel(int K, unsigned seed);
	GLTexture createSSAONoiseTexture(int side, unsigned seed);
}

