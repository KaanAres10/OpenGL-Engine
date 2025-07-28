// gl_engine.cpp
#include "gl_engine.h"
#include "gl_loader.h"
#include "gl_initializers.h"
#include <iostream>
#include <filesystem>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

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


    pipelines["light"] = GLPipeline{
        Shader("light.vert", "light.frag"),
        BlendMode::Alpha,
        true,
        GL_NONE,
        GL_FILL
    };


    pipelines["object"] = GLPipeline{
        Shader("object.vert", "object.frag"),
        BlendMode::Alpha,
        true,
        GL_NONE,
        GL_FILL
    };

    lightMesh = glloader::loadCubeWithoutTexture();


    objectMesh = glloader::loadCubeWithTexture_Normal();
    containerTex = glloader::loadTexture("assets/textures/container.png");
    containerSpecularTex = glloader::loadTexture("assets/textures/container_specular.png");

    camera.position = { 0,0,5 };
    camera.pitch = { 0.040 };
    camera.yaw = { 0.2 };

    viewportW = w; viewportH = h;
    return true;
}

void GLEngine::run() {
    bool running = true;
    Uint32 last = SDL_GetTicks();
    while (running) {
        Uint32 now = SDL_GetTicks();
        float totalTime = now * 0.001f;
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

}

void GLEngine::draw() {
    glClearColor(.2f, .2f, .2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(45.f),
        float(viewportW) / viewportH, .1f, 100.f);

    model = glm::translate(glm::mat4(1.0f), lightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    pipelines["light"].apply();
    glBindVertexArray(lightMesh.vao);
    pipelines["light"].shader.setMat4("model", model);
    pipelines["light"].shader.setMat4("view", view);
    pipelines["light"].shader.setMat4("projection", proj);
    glDrawArrays(GL_TRIANGLES, 0, lightMesh.vertexCount);
    glBindVertexArray(0);

    model = glm::mat4(1.0f);
    pipelines["object"].apply();
    pipelines["object"].shader.setInt("material.diffuse", 0);
    pipelines["object"].shader.setInt("material.specular", 1);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, containerTex.id);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, containerSpecularTex.id);

    glBindVertexArray(objectMesh.vao);
    pipelines["object"].shader.setMat4("model", model);
    pipelines["object"].shader.setMat4("view", view);
    pipelines["object"].shader.setMat4("projection", proj);
    pipelines["object"].shader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
    pipelines["object"].shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    pipelines["object"].shader.setVec3("lightPos", lightPos);
    pipelines["object"].shader.setVec3("viewPos", camera.position);
    pipelines["object"].shader.setVec3("material.ambient", glm::vec3(1.0f, 0.5, 0.31f));
    pipelines["object"].shader.setVec3("material.diffuse", glm::vec3(1.0f, 0.5, 0.31f));
    pipelines["object"].shader.setVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
    pipelines["object"].shader.setFloat("material.shininess", 32.0f);
    pipelines["object"].shader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    pipelines["object"].shader.setVec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    pipelines["object"].shader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));


    glDrawArrays(GL_TRIANGLES, 0, objectMesh.vertexCount);
    glBindVertexArray(0);
}

void GLEngine::cleanup() {
    glDeleteProgram(pipelines["light"].shader.ID);
    glDeleteVertexArrays(1, &lightMesh.vao);
    glDeleteBuffers(1, &lightMesh.vbo);
    glDeleteTextures(1, &wallTex.id);
    glDeleteTextures(1, &faceTex.id);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
