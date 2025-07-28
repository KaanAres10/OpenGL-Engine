#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Shader.h"
#include <SDL2/SDL.h>


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

static std::string getExecutableDir() {
    char* basePath = SDL_GetBasePath();
    if (!basePath) throw std::runtime_error("SDL_GetBasePath failed");
    std::string dir(basePath);
    SDL_free(basePath);
    // strip off the executable name, keep the trailing slash
    auto pos = dir.find_last_of("\\/");
    if (pos != std::string::npos)
        dir.resize(pos + 1);
    return dir;
}

static std::string readFile(const std::string& relativePath) {
    std::string fullPath = getExecutableDir() + "shaders/" + relativePath;
    std::ifstream in(fullPath, std::ios::in | std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open file: " + fullPath);
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

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
        // positions         // colors
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
    };

    float texCoords[] = {
    0.0f, 0.0f,  // lower-left corner  
    1.0f, 0.0f,  // lower-right corner
    0.5f, 1.0f   // top-center corner
    };


    // create VAO & VBO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);


    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderScene(Shader &shader) {
    // bind your shader & VAO, then draw:
    shader.use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
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


    int winW, winH;
    SDL_GL_GetDrawableSize(window, &winW, &winH);
    updateViewport(winW, winH);

    // Main loop
    bool running = true;
    SDL_Event event;

    int width = WIDTH, height = HEIGHT;
    glViewport(0, 0, width, height);
    while (running) {
        while (SDL_PollEvent(&event)) {
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
        glClear(GL_COLOR_BUFFER_BIT);

        renderScene(triangleShader);

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
