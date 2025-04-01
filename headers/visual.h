#pragma once

#include "data.h"
#include "post_processor.h"

// 前向声明
struct GLFWwindow;

namespace MC
{
    extern Camera camera;
    extern bool firstMouse;
    extern float lastX, lastY;
    extern bool showIntersections; // 是否显示切割平面交线
    extern bool showErrorPopup;
    extern std::string errorPopupMessage;
    extern std::string errorPopupTitle;
    

    // 着色器相关函数
    unsigned int compileShader(unsigned int type, const char *src);
    unsigned int createShaderProgram();
    unsigned int createLineShaderProgram(); // Added for symmetry axis

    //explosion strategy GUI
    void renderExplosionAxisGUI(std::string& currentStrategy);

    // OpenGL渲染相关函数
    void setupMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void updateMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     std::string &currentExplosionStrategy, PostProcess &postProcessor, unsigned int intersectionVAO = 0, int numIntersectionSegments = 0);

    // UI相关函数
    void setupImGui(GLFWwindow *window);
    void cleanupImGui();

    // 回调函数
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

    // 设置弹出错误窗口的函数
    void showError(const std::string& title, const std::string& message);

    // 渲染错误弹出窗口的函数
    void renderErrorPopup();

    // 初始化GLFW和GLAD
    GLFWwindow *initOpenGL();

} // namespace MC