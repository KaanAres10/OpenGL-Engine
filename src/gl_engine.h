// gl_engine.h
#pragma once
#include "gl_pipelines.h"
#include <SDL.h>
#include "camera.h"
#include "gl_types.h"
#include <vector>
#include <Shader.h>
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
	GLTexture     wallTex, faceTex;
	std::vector<glm::vec3> cubePositions;

	int viewportW, viewportH;

	void processEvent(SDL_Event&);
	void update(float dt);
	void draw();
};
