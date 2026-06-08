// main.cpp - Entry point. Creates the OpenGL 3.3 window and runs the game.
// "Arcane Ascendant" - a third-person mage action game written from scratch
// in C++ with OpenGL (no game engine). Runs on integrated GPUs (no NVIDIA needed).
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "Game.h"
#include <iostream>

static Game* g_game = nullptr;

static void cursorCB(GLFWwindow* w, double x, double y){ if (g_game) g_game->onMouse(x,y); }
static void scrollCB(GLFWwindow* w, double dx, double dy){ if (g_game) g_game->onScroll(dy); }
static void keyCB(GLFWwindow* w, int key, int sc, int action, int mods){ if (g_game) g_game->onKey(key,action); }
static void mbCB(GLFWwindow* w, int b, int action, int mods){ if (g_game) g_game->onMouseButton(b,action); }
static void fbCB(GLFWwindow* w, int width, int height){ if (g_game) g_game->onResize(width,height); }

int main(){
    if (!glfwInit()){ std::cerr<<"Failed to init GLFW\n"; return -1; }

    // Request OpenGL 3.3 Core - widely supported on Intel HD / AMD integrated GPUs.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 2); // light MSAA; cheap on weak GPUs

    int W=1280, H=720;
    GLFWwindow* window = glfwCreateWindow(W,H,"Arcane Ascendant - Mage Action RPG", nullptr, nullptr);
    if (!window){
        std::cerr<<"Failed to create window. Trying without MSAA...\n";
        glfwWindowHint(GLFW_SAMPLES,0);
        window = glfwCreateWindow(W,H,"Arcane Ascendant", nullptr, nullptr);
        if (!window){ glfwTerminate(); return -1; }
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync on => smooth, no tearing, low GPU load

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)){
        std::cerr<<"Failed to load OpenGL functions\n"; glfwTerminate(); return -1;
    }

    std::cout<<"OpenGL: "<<glGetString(GL_VERSION)<<"\n";
    std::cout<<"Renderer: "<<glGetString(GL_RENDERER)<<"\n";

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Game game;
    g_game = &game;
    if (!game.init(window)){ std::cerr<<"Game init failed\n"; glfwTerminate(); return -1; }

    glfwSetCursorPosCallback(window, cursorCB);
    glfwSetScrollCallback(window, scrollCB);
    glfwSetKeyCallback(window, keyCB);
    glfwSetMouseButtonCallback(window, mbCB);
    glfwSetFramebufferSizeCallback(window, fbCB);

    game.run();

    game.shutdown();
    g_game = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
