#include "gl_engine.h"

// Choose External GPU
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


int main() {
    GLEngine app;
    if (!app.init(800, 600)) return -1;
    app.run();
    app.cleanup();
    return 0;
}

