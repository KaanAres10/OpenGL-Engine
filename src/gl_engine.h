#pragma once
#include "gl_pipelines.h"
#include <SDL.h>
#include "camera.h"
#include "gl_types.h"
#include <vector>
#include <shader.h>
#include <unordered_map>
#include <model.h>
#include "framebuffer.h"
#include "uniformbuffer.hpp"

struct alignas(16) Matrices {
	glm::mat4 projection;
	glm::mat4 view;
};


struct InstanceGrid {
	unsigned int           N = 100;         
	unsigned int           gridSize = 20;  
	float                  spacing = 15.0f;   
	float                  halfExt = (gridSize - 1) * 0.5f * spacing;  
	std::vector<glm::mat4> modelMatrices;
};


class GLEngine {
public:
	bool init(int w, int h);
	void run();
	void cleanup();

private:
	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	int viewportW, viewportH;
	Camera        camera;
	std::unordered_map<std::string, GLPipeline> pipelines;

	GLMesh        cubeMesh;
	GLMesh        planeMesh;
	GLMesh        objectMesh;
	GLMesh        lightMesh;
	GLMesh        quadMesh;
	GLMesh        screenQuadMesh;
	GLMesh        skyBoxMesh;
	GLMesh        environmentCubeMesh;
	GLMesh        pointsMesh;
	GLMesh        quadInstancingMesh;
	GLMesh        floorMesh;
	GLMesh        hdrEnvCubeMesh;

	GLMeshBuffers sphereMesh;



	bool enableImgui = true;

	GLTexture     wallTex, faceTex, containerTex,
		containerSpecularTex, floorTex, grassTex, windowTex, cubeMapTex, whiteTex, brickWallTex, brickWallNormalTex, brickWallDisplacementTex, 
		toyBoxTex, toyBoxNormalTex, toyBoxDisTex, blackTex, rustedIronAlbedoTex, rustedIronNormalTex, rustedIronRoughnessTex, rustedIronMetallicTex, rustedIronAOTex;

	GLTexture depthCubemap;

	GLTexture ssaoNoiseTex;

	// For Diffuse IBL
	GLTexture hdrEnvMapTex, hdrEnvCubeMapTex, irradianceCubeMap;

	// For Specular IBL
	GLTexture prefilteredEnvMap;
	GLTexture brdfIntegrationMap;

	GLuint blurredBloomTex;


	std::vector<GLuint> depthCubemaps;
	std::vector<std::unique_ptr<Framebuffer>> shadowCubeFBOs;

	std::vector<glm::vec3> cubePositions;
	std::vector<glm::vec3> pointLightPositions;
	std::vector<glm::vec3> pointLightColors;
	std::vector<glm::mat4> instanceModelMatrices;

	InstanceGrid grid;

	Model sceneModel;
	Model plantModel;
	Model backpackModel;
	Model cerberusModel;

	vector<glm::vec3> vegetation;

	FramebufferSpecification frameBufferSpec;
	std::unique_ptr<Framebuffer> sceneFrameBuffer;
	std::unique_ptr<Framebuffer> resolveFrameBuffer;
	std::unique_ptr<Framebuffer> pingFrameBuffer;
	std::unique_ptr<Framebuffer> pongFrameBuffer;
	std::unique_ptr<Framebuffer> gBuffer;
	std::unique_ptr<Framebuffer> ssaoFrameBuffer;
	std::unique_ptr<Framebuffer> ssaoBlurFrameBuffer;

	std::vector<glm::vec3> ssaoKernel;
	int   ssaoKernelSize = 64;
	float ssaoRadius = 0.75f;
	float ssaoBias = 0.025f;
	bool  ssaoEnabled = true;

	static constexpr GLuint SHADOW_WIDTH = 2048;
	static constexpr GLuint SHADOW_HEIGHT = 2048;
	float nearPlane = 0.1f, farPlane = 1000.0f;
	std::unique_ptr<Framebuffer> shadowFrameBuffer;
	glm::mat4 lightSpaceMatrix;

	glm::vec3 lightPos = glm::vec3(2000.0f, 2261.0f, -23.0f);
	glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	Camera   lightCamera;

	std::unique_ptr<UniformBuffer<Matrices>> uboMatrices;

	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	void update(float dt);
	void draw();

	void basicPBRGrid();

	void texturedPBRGrid();

	void basicPBR_IBL_Grid();

	void texturedPBR_IBL_Grid();

	void drawPBRModel();

	void drawSkyBox();

	void ambientOcclussion();

	void geometryPass();

	void lightPass();

	void blurBloom();

	void postProcess();

	void pointLightShadow();

	void directionalLightShadow();

	void drawCubeMap();

	void drawEnvironmentMap();

	void drawDisplacementToy();

	void drawLightBoxes();

	void drawPointLights();

	void drawDirectionalLight();

	void drawSpotLight();

	void drawScene();
	void drawPlants();
	void drawSceneNormal();
	void drawLight();
	void drawCubes();

	void addPointLight(const glm::vec3& pos, const glm::vec3& col);

	void removePointLight(int idx);


};
