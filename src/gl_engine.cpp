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
#include <random>
#include <cmath>
#include "ImGuizmo.h"
#include <array>
#include <fmt/format.h>
#include <numeric>

const static int NR_POINT_LIGHTS = 16;
constexpr int MAX_PL = NR_POINT_LIGHTS;

static float fpsTimer = 0.0f;
static int frameCount = 0;

static int   selectedLightIdx = -1;                 
static float gizmoSnap[3] = { 0.f, 0.f, 0.f };    
static void ShowImGuizmoTranslation(int viewportW, int viewportH,
    const Camera& cam,
    glm::vec3& position)
{
    ImGuizmo::BeginFrame();                               
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList()); 

    const glm::mat4 view = cam.getViewMatrix();
    const glm::mat4 proj = glm::perspective(glm::radians(75.0f),
        float(viewportW) / float(viewportH),
        0.1f, 10000.0f);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    ImGuizmo::SetRect(0, 0, float(viewportW), float(viewportH));

    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
        ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
        glm::value_ptr(model), nullptr, gizmoSnap);

    if (ImGuizmo::IsUsing())
        position = glm::vec3(model[3]);               
}

void GLEngine::addPointLight(const glm::vec3& pos, const glm::vec3& col) {
    pointLightPositions.push_back(pos);
    pointLightColors.push_back(col);

    GLTexture cube = glloader::loadDepthCubemap(SHADOW_WIDTH, SHADOW_HEIGHT);
    depthCubemaps.push_back(cube.id);

    FramebufferSpecification cubeSpec;
    cubeSpec.Width = SHADOW_WIDTH;
    cubeSpec.Height = SHADOW_HEIGHT;
    cubeSpec.Samples = 1;
    cubeSpec.Attachments = { {
        FramebufferAttachmentType::TextureCubeMap,
        GL_DEPTH_COMPONENT24,
        GL_DEPTH_ATTACHMENT
    } };
    auto fbo = std::make_unique<Framebuffer>(cubeSpec);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo->GetRendererID());
    glFramebufferTexture(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        cube.id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shadowCubeFBOs.push_back(std::move(fbo));
}

void GLEngine::removePointLight(int idx) {
    glDeleteTextures(1, &depthCubemaps[idx]);
    depthCubemaps.erase(depthCubemaps.begin() + idx);
    shadowCubeFBOs.erase(shadowCubeFBOs.begin() + idx);
    pointLightPositions.erase(pointLightPositions.begin() + idx);
    pointLightColors.erase(pointLightColors.begin() + idx);
}
bool GLEngine::init(int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);    
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

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

    if (enableImgui)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForOpenGL(window, glContext);
        ImGui_ImplOpenGL3_Init("#version 450");
    }


    glViewport(0, 0, w, h);


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_STENCIL_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);
    
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
    GL_NONE,
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

    pipelines["geometry"] = GLPipeline{
    Shader("geometry.vert", "geometry.geom", "geometry.frag"),
    BlendMode::Alpha,
    true,
    GL_BACK,
    GL_FILL
    };

    pipelines["instancing"] = GLPipeline{
    Shader("instancing.vert", "instancing.frag"),
    BlendMode::Alpha,
    true,
    GL_NONE,
    GL_FILL
    };

    pipelines["fxaa"] = GLPipeline{
    Shader("fxaa.vert", "fxaa.frag"),
    BlendMode::None,
    false,         
    GL_NONE,
    GL_FILL
    };

    pipelines["blinn_phong"] = GLPipeline{
      Shader("blinn_phong.vert", "blinn_phong.frag"),
      BlendMode::None,
      true,
      GL_NONE,
      GL_FILL
    };

    pipelines["blinn_phong_V2"] = GLPipeline{
    Shader("blinn_phong_V2.vert", "blinn_phong_V2.frag"),
    BlendMode::None,
    true,
    GL_NONE,
    GL_FILL
    };

    pipelines["shadow_map"] = GLPipeline{
    Shader("shadow_map.vert", "shadow_map.frag"),
    BlendMode::None,
    true,
    GL_NONE,
    GL_FILL
    };

    pipelines["depth_debug"] = GLPipeline{
    Shader("depth_debug.vert", "depth_debug.frag"),
    BlendMode::None,
    true,
    GL_NONE,
    GL_FILL
    };

    pipelines["shadow_cube"] = GLPipeline{
    Shader("shadow_cube.vert", "shadow_cube.geom", "shadow_cube.frag"),
    BlendMode::None,
    true,
    GL_NONE,
    GL_FILL
    };

    pipelines["parallax"] = GLPipeline{
    Shader("parallax.vert", "parallax.frag"),
    BlendMode::None,
    true,
    GL_NONE,
    GL_FILL
    };


    lightMesh = glloader::loadCubeWithoutTexture();

    objectMesh = glloader::loadCubeWithTexture_Normal();

    cubeMesh = glloader::loadCubeWithTexture_Normal();

    planeMesh = glloader::loadPlaneWithTexture_Normal_Tangent();

    screenQuadMesh = glloader::loadQuadWithTextureNDC();

    quadMesh = glloader::loadQuadWithTexture_Normal();

    skyBoxMesh = glloader::loadCubeOnlyPosition();

    environmentCubeMesh = glloader::loadCubeWithNormal();

    pointsMesh = glloader::loadPointsNDC();

    quadInstancingMesh = glloader::loadQuadWithColorNDC();

    floorMesh = glloader::loadPlaneWithTexture_Normal();

    containerTex = glloader::loadTexture("assets/textures/container.png", true);
    containerSpecularTex = glloader::loadTexture("assets/textures/container_specular.png");
    grassTex = glloader::loadTexture("assets/textures/grass.png", true);
    windowTex = glloader::loadTexture("assets/textures/blending_transparent_window.png", true);
    cubeMapTex = glloader::loadCubemap({
        "assets/textures/sky_2/px.png",
        "assets/textures/sky_2/nx.png", 
        "assets/textures/sky_2/py.png", 
        "assets/textures/sky_2/ny.png", 
        "assets/textures/sky_2/pz.png", 
        "assets/textures/sky_2/nz.png"
        });
    floorTex = glloader::loadTexture("assets/textures/floor.png", true);
    whiteTex = glloader::loadTexture("assets/textures/white.jpg", true);
    brickWallTex = glloader::loadTexture("assets/textures/bricks/bricks2.jpg", true);
    brickWallNormalTex = glloader::loadTexture("assets/textures//bricks/bricks2_normal.jpg", true);
    brickWallDisplacementTex = glloader::loadTexture("assets/textures//bricks/bricks2_disp.jpg", true);
    toyBoxTex = glloader::loadTexture("assets/textures/wooden_toy/toy_box_diffuse.png", true);
    toyBoxNormalTex = glloader::loadTexture("assets/textures/wooden_toy/toy_box_normal.png", true);
    toyBoxDisTex = glloader::loadTexture("assets/textures/wooden_toy/toy_box_disp.png", true);



    sceneModel.loadModel("assets/Sponza/Sponza.gltf");
    plantModel.loadModel("assets/plant/scene.gltf");

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
        glm::vec3(235,  180.0f,  20.0f)/*,
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)*/
    };

    pointLightColors = {
    {1.0f, 1.0f, 1.0f}/*,
    {0.0f, 0.0f, 1.0f}, 
    {0.0f, 1.0f, 0.0f}, 
    {1.0f, 1.0f, 1.0f}, */
    };

    vegetation.push_back(glm::vec3(-1.5f, 0.0f, -0.48f));
    vegetation.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
    vegetation.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
    vegetation.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
    vegetation.push_back(glm::vec3(0.5f, 0.0f, -0.6f));


    grid.modelMatrices.clear();
    grid.modelMatrices.reserve(grid.N);
    for (GLuint i = 0; i < grid.N; ++i) {
        unsigned int row = i / grid.gridSize;
        unsigned int col = i % grid.gridSize;

        float x = (static_cast<float>(col) * grid.spacing) - grid.halfExt;
        float z = (static_cast<float>(row) * grid.spacing) - grid.halfExt;
        float y = 0.0f;

        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        M = glm::scale(M, glm::vec3(0.2f));

        grid.modelMatrices.emplace_back(M);
    }

    camera.position = { 0,0,5 };
    camera.pitch = { 0.040 };
    camera.yaw = { 0.2 };
    camera.front = { 0.0f, 0.0f, -1.0f };
    camera.movementSpeed = 500.0f;

    viewportW = w; viewportH = h;

    frameBufferSpec.Width = w;
    frameBufferSpec.Height = h;
    frameBufferSpec.Samples = 4;

    frameBufferSpec.Attachments = {
        {FramebufferAttachmentType::Texture2D, GL_RGB8, GL_COLOR_ATTACHMENT0},
        {FramebufferAttachmentType::Renderbuffer, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT}
    };

    sceneFrameBuffer = std::make_unique<Framebuffer>(frameBufferSpec);


    FramebufferSpecification resolveSpec = frameBufferSpec;
    resolveSpec.Samples = 1;  
    resolveSpec.Attachments = {
        { FramebufferAttachmentType::Texture2D, GL_RGB8, GL_COLOR_ATTACHMENT0 },
    };
    resolveFrameBuffer = std::make_unique<Framebuffer>(resolveSpec);

    
    FramebufferSpecification shadowSpec;
    shadowSpec.Width = SHADOW_WIDTH;
    shadowSpec.Height = SHADOW_HEIGHT;
    shadowSpec.Samples = 1;  
    shadowSpec.Attachments = {
        { FramebufferAttachmentType::Texture2D, GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT }
    };
    shadowFrameBuffer = std::make_unique<Framebuffer>(shadowSpec);


    depthCubemap = glloader::loadDepthCubemap(SHADOW_WIDTH, SHADOW_HEIGHT);


    depthCubemaps.reserve(pointLightPositions.size());
    shadowCubeFBOs.reserve(pointLightPositions.size());

    for (int i = 0; i < pointLightPositions.size(); ++i) {
        GLTexture cube = glloader::loadDepthCubemap(SHADOW_WIDTH, SHADOW_HEIGHT);
        depthCubemaps.push_back(cube.id);

        FramebufferSpecification cubeSpec;
        cubeSpec.Width = SHADOW_WIDTH;
        cubeSpec.Height = SHADOW_HEIGHT;
        cubeSpec.Samples = 1;
        cubeSpec.Attachments = {
            { FramebufferAttachmentType::TextureCubeMap, GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT }
        };

       std::unique_ptr<Framebuffer> fbo = std::make_unique<Framebuffer>(cubeSpec);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo->GetRendererID());
        glFramebufferTexture(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            cube.id, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        shadowCubeFBOs.push_back(std::move(fbo));
    }

    uboMatrices = std::make_unique<UniformBuffer<Matrices>>(0, GL_DYNAMIC_DRAW);

    lightCamera.position = glm::vec3(-2.0f, 4.0f, -1.0f);
    lightCamera.yaw = glm::radians(45.0f);  
    lightCamera.pitch = glm::radians(-30.0f);
    lightCamera.movementSpeed = 10.0f;
    lightCamera.mouseSensitivity = 0.005f;

    return true;
}

void GLEngine::run() {
    bool running = true;
    Uint32 last = SDL_GetTicks();

    while (running) {

        Uint32 now = SDL_GetTicks();
        float dt = (now - last) * 0.001f;
        last = now;
        enableImgui = true;
        ImGuiIO& io = ImGui::GetIO();
        SDL_Event e;

        frameCount++;
        fpsTimer += dt;

        for (auto& [name, pipe] : pipelines) {
            pipe.shader.reloadIfChanged();
        }

        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        while (SDL_PollEvent(&e)) {
            if (enableImgui) ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                running = false;
            }
            else if ((e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN ||
                e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEWHEEL)
                && (enableImgui && io.WantCaptureMouse)) {
                // ImGui handles mouse
            }
            else if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP ||
                e.type == SDL_TEXTINPUT) &&  (enableImgui && io.WantCaptureKeyboard)) {
                // ImGui handles keyboard
            }
            else {
                camera.processSDLEvent(e);
                lightCamera.processSDLEvent(e);

                if (e.type == SDL_WINDOWEVENT &&
                    e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    viewportW = e.window.data1;
                    viewportH = e.window.data2;
                    glViewport(0, 0, viewportW, viewportH);
                    sceneFrameBuffer->Resize(viewportW, viewportH);
                    resolveFrameBuffer->Resize(viewportW, viewportH);
                }
            }
        }

        if (enableImgui)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
        }
     

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = glm::perspective(
            glm::radians(75.0f),
            float(viewportW) / float(viewportH),
            0.1f, 10000.0f
        );


        if (enableImgui) {
            ImGui::Begin("Lights");

            {
                std::vector<std::string> labels = { "None" };
                for (size_t i = 0; i < pointLightPositions.size(); ++i)
                    labels.push_back("Light " + std::to_string(i));

                std::vector<const char*> cstrs;
                for (auto& s : labels) cstrs.push_back(s.c_str());

                int comboIdx = selectedLightIdx + 1;
                ImGui::Combo("Selected light", &comboIdx,
                    cstrs.data(), int(cstrs.size()));
                selectedLightIdx = comboIdx - 1;
            }

            if (selectedLightIdx >= 0)
            {
                ImGui::PushID(selectedLightIdx);

                glm::vec3& pos = pointLightPositions[selectedLightIdx];
                glm::vec3& col = pointLightColors[selectedLightIdx];

                ShowImGuizmoTranslation(viewportW, viewportH, camera, pos);

                ImGui::DragFloat3("Position", glm::value_ptr(pos),
                    0.05f, -50.f, 50.f);
                ImGui::ColorEdit3("Color", glm::value_ptr(col));

                ImGui::PopID();
            }

            if (ImGui::Button("Add point light"))
            {
                addPointLight(glm::vec3(0.0f), glm::vec3(1.0f));
                selectedLightIdx = int(pointLightPositions.size()) - 1;
            }

            if (selectedLightIdx >= 0 && ImGui::Button("Remove selected"))
            {
                removePointLight(selectedLightIdx);
                selectedLightIdx = -1;
            }

            ImGui::End();
        }
        

        uboMatrices->updateMember(0, proj);
        uboMatrices->updateMember(sizeof(glm::mat4), view);

        camera.update(dt);
        lightCamera.update(dt);

        //lightPos = lightCamera.position;

        update(dt);
        draw();

      
        if (enableImgui)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
 

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
        resolveFrameBuffer->Resize(viewportW, viewportH);
    }
}

void GLEngine::update(float dt) {

}

void GLEngine::draw() {
    renderPointLightShadow();

    renderDirectionalLightShadow();

    // Draw the Actual Scene
    glViewport(0, 0, viewportW, viewportH);
    sceneFrameBuffer->Bind();
    glEnable(GL_DEPTH_TEST);
    glClearColor(.2f, .2f, .2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    model = glm::mat4(1.0f);
    view = camera.getViewMatrix();
    proj = glm::perspective(glm::radians(75.f),
        float(viewportW) / viewportH, 0.1f, 10000.f);
    uboMatrices->updateMember(0, proj);
    uboMatrices->updateMember(sizeof(glm::mat4), view);



    pipelines["blinn_phong_V2"].shader.use();
    pipelines["blinn_phong_V2"].shader.setVec3("viewPos", camera.position);
    pipelines["blinn_phong_V2"].setModel(model);
    pipelines["blinn_phong_V2"].shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    pipelines["blinn_phong_V2"].shader.setVec3("dirLightDirection", glm::vec3(-0.840f, -0.541f, -0.035f));
    pipelines["blinn_phong_V2"].shader.setVec3("dirLightColor", glm::vec3(1.0f));

    // Shadow Cubemap binding for each point light
    int pointCount = pointLightPositions.size();
    std::array<GLint, MAX_PL> textureUnits;
    for (int i = 0; i < MAX_PL; i++) {
        int texUnit = 5 + i;
        textureUnits[i] = texUnit;
        glActiveTexture(GL_TEXTURE0 + texUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (i < pointCount) ? depthCubemaps[i] : depthCubemaps[0]);
    }
    glUniform1iv(glGetUniformLocation(pipelines["blinn_phong_V2"].shader.ID, "shadowCubes"),
        MAX_PL, textureUnits.data());

    GLfloat fars[MAX_PL];
    for (int i = 0; i < MAX_PL; i++)
        fars[i] = farPlane;
    glUniform1fv(glGetUniformLocation(pipelines["blinn_phong_V2"].shader.ID, "far_planes"),
        MAX_PL, fars);

    pipelines["blinn_phong_V2"].shader.setInt("uPointLightCount", pointCount);
    for (int i = 0; i < pointCount; ++i) {
        pipelines["blinn_phong_V2"].shader.setVec3(
            fmt::format("pointLightPositions[{}]", i),
            pointLightPositions[i]
        );
        pipelines["blinn_phong_V2"].shader.setVec3(
            fmt::format("pointLightColors[{}]", i),
            pointLightColors[i]
        );
    }
   

    drawScene();
     
    drawCubeMap();

    drawDisplacementToy();

    sceneFrameBuffer->Unbind();

    // MSAA to Single Sample Framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER,  sceneFrameBuffer->GetRendererID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,  resolveFrameBuffer->GetRendererID());
    glBlitFramebuffer(
        0, 0, viewportW, viewportH,
        0, 0, viewportW, viewportH,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);


    // Draw Screen Quad 
    glViewport(0, 0, viewportW, viewportH);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    pipelines["framebuffer"].apply();
    pipelines["framebuffer"].shader.setInt("screenTexture", 0);

    glBindVertexArray(screenQuadMesh.vao);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, resolveFrameBuffer->GetTextureID(0));
    glDrawArrays(GL_TRIANGLES, 0, screenQuadMesh.vertexCount);
    glBindVertexArray(0);
}

void GLEngine::renderPointLightShadow()
{
    float aspect = float(SHADOW_WIDTH) / float(SHADOW_HEIGHT);
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
    for (int i = 0; i < (int)pointLightPositions.size(); ++i) {
        shadowCubeFBOs[i]->Bind();
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);

        std::array<glm::mat4, 6> shadowTransforms;
        shadowTransforms[0] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
        shadowTransforms[1] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
        shadowTransforms[2] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        shadowTransforms[3] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
        shadowTransforms[4] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
        shadowTransforms[5] = shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));

        pipelines["shadow_cube"].apply();
        for (int f = 0; f < 6; ++f) {
            pipelines["shadow_cube"].shader.setMat4("shadowMatrices[" + std::to_string(f) + "]", shadowTransforms[f]);
        }
        pipelines["shadow_cube"].shader.setVec3("lightPos", pointLightPositions[i]);
        pipelines["shadow_cube"].shader.setFloat("far_plane", farPlane);

        glm::mat4 model = glm::mat4{ 1.0 };
        pipelines["shadow_cube"].setModel(model);
        sceneModel.draw(pipelines["shadow_cube"].shader);

        shadowCubeFBOs[i]->Unbind();
    }
}

void GLEngine::renderDirectionalLightShadow()
{
    static float l = -1000.0f, r = 1000.0f, b = -1000.0f, t = 1000.0f, n = 0.01f, f = 4000.0f;
    if (enableImgui)
    {
        ImGui::Begin("Shadow Frustum");
        ImGui::DragFloat("Left", &l, 1.0f, -5000.0f, 5000.0f);
        ImGui::DragFloat("Right", &r, 1.0f, -5000.0f, 5000.0f);
        ImGui::DragFloat("Bottom", &b, 1.0f, -5000.0f, 5000.0f);
        ImGui::DragFloat("Top", &t, 1.0f, -5000.0f, 5000.0f);
        ImGui::DragFloat("Near", &n, 0.1f, 0.01f, 1000.0f);
        ImGui::DragFloat("Far", &f, 0.1f, 0.01f, 10000.0f);
        ImGui::End();

        ImGui::Begin("Directional Light View");
        ImGui::DragFloat3("Directional Light Position", glm::value_ptr(lightPos), 0.1f, -1000.0f, 1000.0f);
        ImGui::DragFloat3("Light Target", glm::value_ptr(lightTarget), 0.1f, -1000.0f, 1000.0f);
        ImGui::End();

        ImGui::Begin("Camera");
        ImGui::DragFloat3("Camera Position", glm::value_ptr(camera.position), 0.1f, -1000.0f, 1000.0f);
        ImGui::DragFloat3("Camera Direction", glm::value_ptr(camera.front), 0.1f, -1000.0f, 1000.0f);

        ImGui::End();
    }
   

    glm::mat4 lightProj = glm::ortho(l, r, b, t, n, f);
    glm::mat4 lightView = glm::lookAt(
        lightPos,
        lightTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    lightSpaceMatrix = lightProj * lightView;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    shadowFrameBuffer->Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    model = glm::mat4(1.0f);
    pipelines["shadow_map"].apply();
    pipelines["shadow_map"].shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    pipelines["shadow_map"].setModel(model);
    sceneModel.draw(pipelines["shadow_map"].shader);
    shadowFrameBuffer->Unbind();
}



void GLEngine::drawCubeMap() {
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
}

void GLEngine::drawEnvironmentMap()
{
    glDisable(GL_CULL_FACE);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 250.0f, 0.0f));
    model = glm::scale(model, glm::vec3(50.0f, 50.0f, 50.0f));
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
}

void GLEngine::drawDisplacementToy()
{
    pipelines["parallax"].apply();
    pipelines["parallax"].shader.setVec3("viewPos", camera.position);
    pipelines["parallax"].shader.setVec3("pointLightPositions[0]", pointLightPositions[0]);
    model = glm::translate(model, glm::vec3(0.0, 50.0, 0.0));
    model = glm::scale(model, glm::vec3(50, 50, 50));
    pipelines["parallax"].shader.setInt("diffuseMap", 0);
    pipelines["parallax"].shader.setInt("normalMap", 2);
    pipelines["parallax"].shader.setInt("depthMap", 3);
    pipelines["parallax"].shader.setFloat("height_scale", 0.2);
    pipelines["parallax"].setModel(model);

    glBindVertexArray(planeMesh.vao);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, toyBoxTex.id);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, toyBoxNormalTex.id);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, toyBoxDisTex.id);
    glDrawArrays(GL_TRIANGLES, 0, planeMesh.vertexCount);
    glBindVertexArray(0);
}


void GLEngine::drawPointLights()
{

    pipelines["blinn_phong"].shader.setInt("uPointLightCount",
        static_cast<int>(pointLightPositions.size()));

    for (GLuint i = 0; i < pointLightPositions.size(); i++) {
        std::string idx = std::to_string(i);

        pipelines["blinn_phong"].shader.setVec3(
            ("pointLights[" + idx + "].position").c_str(),
            pointLightPositions[i]
        );
        pipelines["blinn_phong"].shader.setVec3(
            ("pointLights[" + idx + "].ambient").c_str(),
            glm::vec3(0.05f)
        );
        pipelines["blinn_phong"].shader.setVec3(
            ("pointLights[" + idx + "].diffuse").c_str(),
            glm::vec3(0.8f)
        );
        pipelines["blinn_phong"].shader.setVec3(
            ("pointLights[" + idx + "].specular").c_str(),
            glm::vec3(1.0f)
        );
        pipelines["blinn_phong"].shader.setFloat(
            ("pointLights[" + idx + "].constant").c_str(),
            1.0f
        );
        pipelines["blinn_phong"].shader.setFloat(
            ("pointLights[" + idx + "].linear").c_str(),
            0.198f
        );
        pipelines["blinn_phong"].shader.setFloat(
            ("pointLights[" + idx + "].quadratic").c_str(),
            0.155f
        );
    }
}

void GLEngine::drawDirectionalLight()
{
    pipelines["blinn_phong"].shader.setVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    pipelines["blinn_phong"].shader.setVec3("directionalLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
    pipelines["blinn_phong"].shader.setVec3("directionalLight.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
    pipelines["blinn_phong"].shader.setVec3("directionalLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));


}

void GLEngine::drawSpotLight() {
    pipelines["blinn_phong"].shader.setVec3("spotLight.position", camera.position);
    pipelines["blinn_phong"].shader.setVec3("spotLight.direction", camera.front);
    pipelines["blinn_phong"].shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    pipelines["blinn_phong"].shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
    pipelines["blinn_phong"].shader.setVec3("spotLight.ambient", glm::vec3(0.0f));
    pipelines["blinn_phong"].shader.setVec3("spotLight.diffuse", glm::vec3(1.0f));
    pipelines["blinn_phong"].shader.setVec3("spotLight.specular", glm::vec3(1.0f));
    pipelines["blinn_phong"].shader.setFloat("spotLight.constant", 1.0f);
    pipelines["blinn_phong"].shader.setFloat("spotLight.linear", 0.198f);
    pipelines["blinn_phong"].shader.setFloat("spotLight.quadratic", 0.155f);
}

void GLEngine::drawScene()
{
    pipelines["blinn_phong_V2"].apply();

    pipelines["blinn_phong_V2"].shader.setInt("shadowMap", 4);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, shadowFrameBuffer->GetTextureID(0));

    glPointSize(8.0f);
    model = glm::mat4(1.0f);
    pipelines["blinn_phong_V2"].setModel(model);

    sceneModel.draw(pipelines["blinn_phong_V2"].shader);
}

void GLEngine::drawPlants()
{
    pipelines["instancing"].apply();
    plantModel.setInstanceData(grid.modelMatrices);
    plantModel.drawInstanced(pipelines["instancing"].shader,
        static_cast<GLsizei>(grid.N));
}

void GLEngine::drawSceneNormal()
{
    model = glm::mat4(1.0f);
    pipelines["geometry"].shader.use();
    pipelines["geometry"].shader.setMat4("model", model);
    pipelines["geometry"].shader.setMat4("view", view);
    pipelines["geometry"].shader.setMat4("projection", proj);


    sceneModel.draw(pipelines["geometry"].shader);
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
