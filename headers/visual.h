#pragma once

#include "data.h"
#include "post_processor.h"
#include "planes/exploded_view.h"

// 前向声明
struct GLFWwindow;

namespace MC
{
    extern Camera camera;
    extern bool firstMouse;
    extern float lastX, lastY;
    extern bool showIntersections; // 控制爆炸视图的显示
    extern bool showErrorPopup;
    extern std::string errorPopupMessage;
    extern std::string errorPopupTitle;
    // ISO Level百分比相关全局变量
    extern float g_isoLevelPercent; // ISO Level百分比值（0-100%）
    extern float g_tempIsoLevelPercent;

    // 爆炸距离相关全局变量
    extern float g_explosionDistancePercent; // 爆炸距离百分比（0-100%）
    extern float g_explosionDistance;        // 实际爆炸距离值

    // 着色器相关函数
    unsigned int compileShader(unsigned int type, const char *src);
    unsigned int createShaderProgram();
    unsigned int createLineShaderProgram(); // Added for symmetry axis

    // ImGui样式设置
    void setupImGuiStyle();

    // 视口计算和更新
    void calculateViewport(GLFWwindow *window, int &viewportX, int &viewportY, int &viewportWidth, int &viewportHeight);
    void updateViewport(GLFWwindow *window);

    // explosion strategy GUI
    void renderExplosionAxisGUI(std::string &currentStrategy);

    // OpenGL渲染相关函数
    void setupMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void updateMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO);
    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     std::string &currentExplosionStrategy, PostProcess &postProcessor,
                     unsigned int axisVAO);

    // 爆炸视图渲染函数
    void renderExplodedView(
        GLFWwindow *window,
        const ExplodedView &explodedView,
        unsigned int shaderProgram,
        const Mesh &mesh,
        float &isoLevel,
        float &tempIsoLevel,
        const VolumeData &volumeData,
        unsigned int axisVAO);

    // UI相关函数
    void setupImGui(GLFWwindow *window);
    void cleanupImGui();

    // 回调函数
    void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

    // 设置弹出错误窗口的函数
    void showError(const std::string &title, const std::string &message);

    // 渲染错误弹出窗口的函数
    void renderErrorPopup();

    // 初始化GLFW和GLAD
    GLFWwindow *initOpenGL();

    // 主渲染循环
    void runRenderingLoop(GLFWwindow *window, const VolumeData &volumeData, Mesh &mesh);

} // namespace MC