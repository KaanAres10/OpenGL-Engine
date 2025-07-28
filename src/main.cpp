#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Shader.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include "camera.h"


// Choose External GPU
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// Screen dimensions
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

constexpr int BASE_WIDTH = WIDTH;
constexpr int BASE_HEIGHT = HEIGHT;
constexpr float BASE_ASPECT = float(BASE_WIDTH) / float(BASE_HEIGHT);


GLuint VAO = 0;

void updateViewport(int windowW, int windowH) {
    float winAspect = float(windowW) / float(windowH);
    int vpW, vpH, vpX, vpY;

    if (winAspect > BASE_ASPECT) {
        vpH = windowH;
        vpW = int(vpH * BASE_ASPECT + 0.5f);
        vpX = (windowW - vpW) / 2;
        vpY = 0;
    }
    else {
        vpW = windowW;
        vpH = int(vpW / BASE_ASPECT + 0.5f);
        vpX = 0;
        vpY = (windowH - vpH) / 2;
    }

    glViewport(vpX, vpY, vpW, vpH);
}


void initGLResources() {
    // define your geometry
    float vertices[] = {
     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
      0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

     -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    // create VAO & VBO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);


    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    stbi_set_flip_vertically_on_load(true);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // wrap/filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets\\textures\\wall.jpg",
        &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "ERROR: Failed to load texture \n"
            << "Reason: " << stbi_failure_reason() << std::endl;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    GLuint texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    // wrap/filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("assets\\textures\\awesomeface.png",
        &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "ERROR: Failed to load texture \n"
            << "Reason: " << stbi_failure_reason() << std::endl;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    
}

void renderScene(Shader &shader) {
    // bind your shader & VAO, then draw:

    glm::vec3 cubePositions[] = {
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f,  2.0f, -2.5f),
    glm::vec3(1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    shader.use();
    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < 10; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        shader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }    glBindVertexArray(0);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create the window 
    SDL_Window* window = SDL_CreateWindow(
        "OpenGL-Engine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    // Create the GL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Load GL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    Camera camera;
    camera.position = glm::vec3(0.0f, 0.0f, 3.0f);   
    camera.yaw = glm::radians(-90.0f);          
    camera.pitch = 0.0f;
    camera.movementSpeed = 5.0f;
    camera.mouseSensitivity = 0.002f;

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


    glEnable(GL_DEPTH_TEST);


    Shader triangleShader("triangle.vert",
        "triangle.frag");

    try {
        initGLResources();
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing GL resources: " << e.what() << "\n";
        // clean up SDL/GL context before exit…
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
    }

    triangleShader.use();
    triangleShader.setInt("texture1", 0);
    triangleShader.setInt("texture2", 1);


    int winW, winH;
    SDL_GL_GetDrawableSize(window, &winW, &winH);
    updateViewport(winW, winH);

    // Main loop
    bool running = true;
    SDL_Event event;

    int width = WIDTH, height = HEIGHT;
    glViewport(0, 0, width, height);

    Uint32 lastTicks = SDL_GetTicks();
    while (running) {
        Uint32 nowTicks = SDL_GetTicks();
        float deltaTime = (nowTicks - lastTicks) * 0.001f;
        lastTicks = nowTicks;

        while (SDL_PollEvent(&event)) {
            camera.processSDLEvent(event);

            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                SDL_GL_GetDrawableSize(window, &winW, &winH);
                updateViewport(winW, winH);
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.update(deltaTime);

        renderScene(triangleShader);


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            float(WIDTH) / float(HEIGHT),
            0.1f, 100.0f
        );

        float time = SDL_GetTicks() / 1000.0f;

        model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

        triangleShader.setMat4("model", model);
        triangleShader.setMat4("view", view);
        triangleShader.setMat4("projection", projection);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteProgram(triangleShader.ID);
    glDeleteVertexArrays(1, &VAO);

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
