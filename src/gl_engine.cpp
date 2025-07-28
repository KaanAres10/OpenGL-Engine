// gl_engine.cpp
#include "gl_engine.h"
#include "gl_loader.h"
#include "gl_initializers.h"
#include <iostream>
#include <filesystem>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

bool GLEngine::init(int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO)) return false;
    window = SDL_CreateWindow("GL Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Load GL functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    {
        // Set the current working directory
        char* basePath = SDL_GetBasePath();
        if (!basePath) {
            std::cerr << "SDL_GetBasePath failed\n";
            return -1;
        }
        std::filesystem::path exeDir(basePath);
        SDL_free(basePath);
        auto projectRoot = exeDir.parent_path().parent_path();
        std::filesystem::current_path(projectRoot);
    }

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

    pipelines["textured"] = GLPipeline{
        Shader("triangle.vert", "triangle.frag"), 
        BlendMode::Alpha,  
        true,              
        GL_NONE,         
        GL_FILL          
    };

    pipelines["textured"].shader.use();
    pipelines["textured"].shader.setInt("texture1", 0);
    pipelines["textured"].shader.setInt("texture2", 1);


    cubeMesh = glloader::loadCube();
    wallTex = glloader::loadTexture("assets/textures/wall.jpg");
    faceTex = glloader::loadTexture("assets/textures/awesomeface.png");


    cubePositions = {
        {  0.0f,  0.0f,   0.0f },
        {  2.0f,  5.0f, -15.0f },
        { -1.5f, -2.2f,  -2.5f },
        { -3.8f, -2.0f, -12.3f },
        {  2.4f, -0.4f,  -3.5f },
        { -1.7f,  3.0f,  -7.5f },
        {  1.3f, -2.0f,  -2.5f },
        {  1.5f,  2.0f,  -2.5f },
        {  1.5f,  0.2f,  -1.5f },
        { -1.3f,  1.0f,  -1.5f }
    };

    camera.position = { 0,0,3 };
    viewportW = w; viewportH = h;
    return true;
}

void GLEngine::run() {
    bool running = true;
    Uint32 last = SDL_GetTicks();
    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - last) * 0.001f; last = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            processEvent(e);
        }
        camera.update(dt);
        update(dt);
        draw();
        SDL_GL_SwapWindow(window);
    }
}

void GLEngine::processEvent(SDL_Event& e) {
    camera.processSDLEvent(e);
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
        viewportW = e.window.data1;
        viewportH = e.window.data2;
        glViewport(0, 0, viewportW, viewportH);
    }
}

void GLEngine::update(float dt) {
    // animate cubePositions, etc.
}

void GLEngine::draw() {
    glClearColor(.2f, .2f, .2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pipelines["textured"].apply();

    // bind textures
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTex.id);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, faceTex.id);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(45.f),
        float(viewportW) / viewportH, .1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(pipelines["textured"].shader.ID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pipelines["textured"].shader.ID, "projection"), 1, GL_FALSE, &proj[0][0]);

    glBindVertexArray(cubeMesh.vao);
    for (auto& pos : cubePositions) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        glUniformMatrix4fv(glGetUniformLocation(pipelines["textured"].shader.ID, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    }
}

void GLEngine::cleanup() {
    glDeleteProgram(pipelines["textured"].shader.ID);
    glDeleteVertexArrays(1, &cubeMesh.vao);
    glDeleteBuffers(1, &cubeMesh.vbo);
    glDeleteTextures(1, &wallTex.id);
    glDeleteTextures(1, &faceTex.id);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
