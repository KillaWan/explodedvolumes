#pragma once

#include "data.h"
#include "post_processor.h"
#include "planes/exploded_view.h"

struct GLFWwindow;

namespace MC
{
    extern Camera camera;
    extern bool firstMouse;
    extern float lastX, lastY;
    extern bool showIntersections;
    extern bool showErrorPopup;
    extern std::string errorPopupMessage;
    extern std::string errorPopupTitle;
    extern float g_isoLevelPercent; 
    extern float g_tempIsoLevelPercent;

    extern float g_explosionDistancePercent;
    extern float g_explosionDistance;

    unsigned int compileShader(unsigned int type, const char *src);
    unsigned int createShaderProgram();
    unsigned int createLineShaderProgram(); // Added for symmetry axis

    // ImGui
    void setupImGuiStyle();

    // viewport
    void calculateViewport(GLFWwindow *window, int &viewportX, int &viewportY, int &viewportWidth, int &viewportHeight);
    void updateViewport(GLFWwindow *window);

    // explosion strategy GUI
    void renderExplosionAxisGUI(std::string &currentStrategy);

    // OpenGL render
    void setupMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void updateMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     std::string &currentExplosionStrategy, PostProcess &postProcessor,
                     unsigned int axisVAO, unsigned int lineShaderProgram);

    // exploded view render
    void renderExplodedView(
        GLFWwindow *window,
        const ExplodedView &explodedView,
        unsigned int shaderProgram,
        const Mesh &mesh,
        float &isoLevel,
        float &tempIsoLevel,
        const VolumeData &volumeData,
        unsigned int axisVAO,
        unsigned int lineShaderProgram);

    // ui
    void setupImGui(GLFWwindow *window);
    void cleanupImGui();

    // callback
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

    // error window
    void showError(const std::string &title, const std::string &message);
    void renderErrorPopup();

    // initialize GLFW and GLAD
    GLFWwindow *initOpenGL();

    // main randering loop
    void runRenderingLoop(GLFWwindow *window, const VolumeData &volumeData, Mesh &mesh);

}
