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
#include <map>


bool GLEngine::init(int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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
    glDepthFunc(GL_LESS);

    glEnable(GL_STENCIL_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pipelines["light"] = GLPipeline{
        Shader("light.vert", "light.frag"),
        BlendMode::Alpha,
        true,
        GL_BACK,
        GL_FILL
    };


    pipelines["object"] = GLPipeline{
        Shader("object.vert", "object.frag"),
        BlendMode::Alpha,
        true,
        GL_BACK,
        GL_FILL
    };

    pipelines["model"] = GLPipeline{
        Shader("model.vert", "model.frag"),
        BlendMode::Alpha,
        true,
        GL_BACK,
        GL_FILL
    };

    pipelines["depth"] = GLPipeline{
       Shader("depth.vert", "depth.frag"),
       BlendMode::Alpha,
       true,
       GL_BACK,
       GL_FILL
    };

    pipelines["stencil"] = GLPipeline{
    Shader("stencil.vert", "stencil.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };


    pipelines["blending"] = GLPipeline{
    Shader("blending.vert", "blending.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };

    pipelines["framebuffer"] = GLPipeline{
    Shader("framebuffer.vert", "framebuffer.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };

    pipelines["cubemap"] = GLPipeline{
    Shader("cubemap.vert", "cubemap.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };

    pipelines["environment_map"] = GLPipeline{
    Shader("environment_map.vert", "environment_map.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };


    lightMesh = glloader::loadCubeWithoutTexture();

    objectMesh = glloader::loadCubeWithTexture_Normal();

    cubeMesh = glloader::loadCubeWithTexture_Normal();

    planeMesh = glloader::loadPlaneWithTexture_Normal();

    screenQuadMesh = glloader::loadQuadWithTextureNDC();

    quadMesh = glloader::loadQuadWithTexture_Normal();

    skyBoxMesh = glloader::loadCubeOnlyPosition();

    environmentCubeMesh = glloader::loadCubeWithNormal();

    containerTex = glloader::loadTexture("assets/textures/container.png");
    containerSpecularTex = glloader::loadTexture("assets/textures/container_specular.png");
    floorTex = glloader::loadTextureMirror("assets/textures/floor.jpeg");
    grassTex = glloader::loadTexture("assets/textures/grass.png");
    windowTex = glloader::loadTexture("assets/textures/blending_transparent_window.png");
    cubeMapTex = glloader::loadCubemap({
        "assets/textures/sky_2/px.png",
        "assets/textures/sky_2/nx.png", 
        "assets/textures/sky_2/py.png", 
        "assets/textures/sky_2/ny.png", 
        "assets/textures/sky_2/pz.png", 
        "assets/textures/sky_2/nz.png"  
        });

    sceneModel.loadModel("assets/Sponza/Sponza.gltf");

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

    pointLightPositions = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    vegetation.push_back(glm::vec3(-1.5f, 0.0f, -0.48f));
    vegetation.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
    vegetation.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
    vegetation.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
    vegetation.push_back(glm::vec3(0.5f, 0.0f, -0.6f));


    camera.position = { 0,0,5 };
    camera.pitch = { 0.040 };
    camera.yaw = { 0.2 };
    camera.front = { 0.0f, 0.0f, -1.0f };
    camera.movementSpeed = 100.0f;

    viewportW = w; viewportH = h;

    frameBufferSpec.Width = w;
    frameBufferSpec.Height = h;

    frameBufferSpec.Attachments = {
        {FramebufferAttachmentType::Texture2D, GL_RGB8, GL_COLOR_ATTACHMENT0},
        {FramebufferAttachmentType::Renderbuffer, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT}
    };

    sceneFrameBuffer = std::make_unique<Framebuffer>(frameBufferSpec);

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
        sceneFrameBuffer->Resize(viewportW, viewportH);
    }
}

void GLEngine::update(float dt) {

}

void GLEngine::draw() {
    sceneFrameBuffer->Bind();
    glViewport(0, 0, viewportW, viewportH);

    glEnable(GL_DEPTH_TEST);
    glClearColor(.2f, .2f, .2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


    model = glm::mat4(1.0f);
    view = camera.getViewMatrix();
    proj = glm::perspective(glm::radians(75.f),
        float(viewportW) / viewportH, 0.1f, 10000.f);


    pipelines["blending"].apply();
    pipelines["blending"].setModel(model);
    pipelines["blending"].setView(view);
    pipelines["blending"].setProj(proj);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, containerTex.id);
    glBindVertexArray(cubeMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    glBindVertexArray(0);

    model = glm::translate(model, glm::vec3(0.5, 0.0, 2.0));
    pipelines["blending"].shader.use();
    pipelines["blending"].setModel(model);
    pipelines["blending"].setView(view);
    pipelines["blending"].setProj(proj);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, containerTex.id);
    glBindVertexArray(cubeMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    glBindVertexArray(0);

    model = glm::mat4(1.0f);
    pipelines["blending"].shader.use();
    pipelines["blending"].setModel(model);
    pipelines["blending"].setView(view);
    pipelines["blending"].setProj(proj);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTex.id);
    glBindVertexArray(planeMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, planeMesh.vertexCount);
    glBindVertexArray(0);


    std::map<float, glm::vec3> sorted;
    for (GLuint i = 0; i < vegetation.size(); i++)
    {
        float distance = glm::length(camera.position - vegetation[i]);
        sorted[distance] = vegetation[i];
    }

    drawScene();

    pipelines["blending"].shader.use();
    glBindVertexArray(quadMesh.vao);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, windowTex.id);
    for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); it++) {
        model = glm::mat4(1.0f);
        model = glm::translate(model, it->second);
        pipelines["blending"].setModel(model);
        glDrawArrays(GL_TRIANGLES, 0, quadMesh.vertexCount);
    }

    glDisable(GL_CULL_FACE);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 50.0f, 0.0f));
    model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
    pipelines["environment_map"].shader.use();
    pipelines["environment_map"].setModel(model);
    pipelines["environment_map"].setView(view);
    pipelines["environment_map"].setProj(proj);
    pipelines["environment_map"].shader.setFloat("skybox", 0);
    pipelines["environment_map"].shader.setVec3("cameraPos", camera.position);
    glBindVertexArray(environmentCubeMesh.vao);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex.id);
    glDrawArrays(GL_TRIANGLES, 0, environmentCubeMesh.vertexCount);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);

    view = glm::mat4(glm::mat3(view));
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    pipelines["cubemap"].shader.use();
    pipelines["cubemap"].setView(view);
    pipelines["cubemap"].setProj(proj);
    glBindVertexArray(skyBoxMesh.vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex.id);
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    view = camera.getViewMatrix();


    sceneFrameBuffer->Unbind();

    glViewport(0, 0, viewportW, viewportH);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    pipelines["framebuffer"].shader.use();
    glBindVertexArray(screenQuadMesh.vao);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneFrameBuffer->GetTextureID(0));
    glDrawArrays(GL_TRIANGLES, 0, screenQuadMesh.vertexCount);
    glBindVertexArray(0);
}

void GLEngine::drawScene()
{
    model = glm::mat4(1.0f);

    pipelines["model"].apply();
    pipelines["model"].shader.setMat4("model", model);
    pipelines["model"].shader.setMat4("view", view);
    pipelines["model"].shader.setMat4("projection", proj);

    sceneModel.draw(pipelines["model"].shader);
}

void GLEngine::drawLight()
{
    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), lightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    pipelines["light"].apply();
    pipelines["light"].shader.setMat4("model", model);
    pipelines["light"].shader.setMat4("view", view);
    pipelines["light"].shader.setMat4("projection", proj);
    glBindVertexArray(lightMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, lightMesh.vertexCount);
    glBindVertexArray(0);
}

void GLEngine::drawCubes()
{
    model = glm::mat4(1.0f);
    pipelines["object"].apply();
    pipelines["object"].shader.setInt("material.diffuse", 0);
    pipelines["object"].shader.setInt("material.specular", 1);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, containerTex.id);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, containerSpecularTex.id);
    pipelines["object"].shader.setVec3("material.ambient", glm::vec3(1.0f, 0.5, 0.31f));
    pipelines["object"].shader.setFloat("material.shininess", 32.0f);
    pipelines["object"].shader.setMat4("model", model);
    pipelines["object"].shader.setMat4("view", view);
    pipelines["object"].shader.setMat4("projection", proj);
    pipelines["object"].shader.setVec3("viewPos", camera.position);

    pipelines["object"].shader.setVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    pipelines["object"].shader.setVec3("directionalLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
    pipelines["object"].shader.setVec3("directionalLight.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
    pipelines["object"].shader.setVec3("directionalLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

    for (GLuint i = 0; i < pointLightPositions.size(); i++) {
        std::string idx = std::to_string(i);

        pipelines["object"].shader.setVec3(
            ("pointLights[" + idx + "].position").c_str(),
            pointLightPositions[i]
        );
        pipelines["object"].shader.setVec3(
            ("pointLights[" + idx + "].ambient").c_str(),
            glm::vec3(0.05f)
        );
        pipelines["object"].shader.setVec3(
            ("pointLights[" + idx + "].diffuse").c_str(),
            glm::vec3(0.8f)
        );
        pipelines["object"].shader.setVec3(
            ("pointLights[" + idx + "].specular").c_str(),
            glm::vec3(1.0f)
        );
        pipelines["object"].shader.setFloat(
            ("pointLights[" + idx + "].constant").c_str(),
            1.0f
        );
        pipelines["object"].shader.setFloat(
            ("pointLights[" + idx + "].linear").c_str(),
            0.09f
        );
        pipelines["object"].shader.setFloat(
            ("pointLights[" + idx + "].quadratic").c_str(),
            0.032f
        );
    }

    pipelines["object"].shader.setVec3("spotLight.position", camera.position);
    pipelines["object"].shader.setVec3("spotLight.direction", camera.front);
    pipelines["object"].shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    pipelines["object"].shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
    pipelines["object"].shader.setVec3("spotLight.ambient", glm::vec3(0.0f));
    pipelines["object"].shader.setVec3("spotLight.diffuse", glm::vec3(1.0f));
    pipelines["object"].shader.setVec3("spotLight.specular", glm::vec3(1.0f));
    pipelines["object"].shader.setFloat("spotLight.constant", 1.0f);
    pipelines["object"].shader.setFloat("spotLight.linear", 0.09f);
    pipelines["object"].shader.setFloat("spotLight.quadratic", 0.032f);

    glBindVertexArray(objectMesh.vao);
    for (GLuint i = 0; i < cubePositions.size(); i++) {
        model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        pipelines["object"].shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, objectMesh.vertexCount);
    }
    glBindVertexArray(0);
}


void GLEngine::cleanup() {
    glDeleteProgram(pipelines["light"].shader.ID);
    glDeleteProgram(pipelines["object"].shader.ID);
    glDeleteProgram(pipelines["model"].shader.ID);
    glDeleteVertexArrays(1, &lightMesh.vao);
    glDeleteVertexArrays(1, &objectMesh.vao);
    glDeleteBuffers(1, &lightMesh.vbo);
    glDeleteBuffers(1, &objectMesh.vbo);
    glDeleteTextures(1, &wallTex.id);
    glDeleteTextures(1, &faceTex.id);
    glDeleteTextures(1, &containerTex.id);
    glDeleteTextures(1, &containerSpecularTex.id);

    sceneFrameBuffer.reset();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
