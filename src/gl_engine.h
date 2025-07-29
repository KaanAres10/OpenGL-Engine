#pragma once
#include "gl_pipelines.h"
#include <SDL.h>
#include "camera.h"
#include "gl_types.h"
#include <vector>
#include <shader.h>
#include <unordered_map>

class GLEngine {
public:
	bool init(int w, int h);
	void run();
	void cleanup();

private:
	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	Camera        camera;
	std::unordered_map<std::string, GLPipeline> pipelines;
	GLMesh        cubeMesh;
	GLMesh        objectMesh;
	GLMesh        lightMesh;
	glm::vec3 lightPos{ 1.2f, 1.0f, 2.0f };
	GLTexture     wallTex, faceTex, containerTex, containerSpecularTex;
	std::vector<glm::vec3> cubePositions;
	std::vector<glm::vec3> pointLightPositions;
	int viewportW, viewportH;

	void processEvent(SDL_Event&);
	void update(float dt);
	void draw();
};
