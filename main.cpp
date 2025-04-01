#include "headers/data.h"
#include "headers/marching_cubes.h"
#include "headers/visual.h"
#include "headers/explosionaxis/explosion_axis_strategy.h"
#include "headers/planes/selecting_planes.h"
#include "headers/planes/cutting_planes.h"
#include "headers/planes/exploded_view.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>
#include <algorithm> // for std::max

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "post_processor.h"

// GLAD & GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for matrix transformations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 处理 isoLevel 改变的函数
void handleIsoLevelChange(
    MC::VolumeData &volumeData, 
    float newIsoLevel, 
    MC::Mesh &mesh, 
    unsigned int VAO, 
    unsigned int VBO, 
    unsigned int EBO,
    MC::Vec3 &explosionAxis,
    MC::Vertex &axisStart,
    MC::Vertex &axisEnd,
    unsigned int axisVBO,
    std::vector<MC::CuttingPlane> &cuttingPlanes,
    MC::PlaneIntersection &planeIntersection,
    unsigned int intersectionVAO,
    unsigned int intersectionVBO,
    MC::ExplodedView &explodedView)
{
    // 重新生成网格
    MC::generateMesh(volumeData, newIsoLevel, mesh);
    MC::updateMesh(mesh, VAO, VBO, EBO);

    // 更新轴线顶点
    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;

    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    // 更新轴线VBO
    MC::Vertex axisLine[2] = {axisStart, axisEnd};
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

    // 重新生成切割平面
    cuttingPlanes = MC::generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3);

    // 重新计算切割平面与网格的交线
    planeIntersection = MC::computePlaneIntersections(mesh, cuttingPlanes);

    // 更新交线VAO/VBO
    MC::updateIntersectionVAO(planeIntersection, intersectionVAO, intersectionVBO);

    // 重新计算爆炸视图
    MC::cleanupExplodedView(explodedView);
    explodedView = MC::computeExplodedView(mesh, cuttingPlanes, explosionAxis);

    // 为爆炸视图中的每个片段设置VAO/VBO/EBO
    for (auto &segment : explodedView.segments)
    {
        MC::setupSegmentMesh(segment);
    }
}

// 处理爆炸轴重新计算的函数
void recalculateExplosionAxisAndView(
    MC::Mesh &mesh,
    MC::Vec3 &explosionAxis,
    std::string &currentExplosionStrategy,
    std::string &lastStrategy,
    MC::Vertex &axisStart,
    MC::Vertex &axisEnd,
    unsigned int axisVBO,
    std::vector<MC::CuttingPlane> &cuttingPlanes,
    MC::PlaneIntersection &planeIntersection,
    unsigned int intersectionVAO,
    unsigned int intersectionVBO,
    MC::ExplodedView &explodedView,
    float &lastExplosionDistance)
{
    // 检查策略是否变化
    if (lastStrategy != currentExplosionStrategy)
    {
        std::cout << "Strategy changed from " << lastStrategy
                  << " to " << currentExplosionStrategy << std::endl;
        lastStrategy = currentExplosionStrategy;
    }

    // 保存当前爆炸距离和状态
    float currentExplosionDistance = explodedView.explosionDistance;
    bool wasEnabled = explodedView.enabled;
    
    // 完全清理之前的资源
    MC::cleanupExplodedView(explodedView);
    
    // 重新计算爆炸轴
    explosionAxis = MC::computeExplosionAxis(mesh.vertices);
    std::cout << "Updated explosion axis: ("
              << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";
    
    // 规范化爆炸轴（确保是单位向量）
    float length = std::sqrt(
        explosionAxis.x * explosionAxis.x +
        explosionAxis.y * explosionAxis.y +
        explosionAxis.z * explosionAxis.z);
    
    if (length > 0.0001f) {
        explosionAxis.x /= length;
        explosionAxis.y /= length;
        explosionAxis.z /= length;
    }

    // 更新轴线可视化
    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;

    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    // 更新轴线VBO
    MC::Vertex axisLine[2] = {axisStart, axisEnd};
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

    // 重新生成切割平面 - 明确指定切割数量和使用新的爆炸轴
    cuttingPlanes = MC::generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3); // 3个切割平面

    // 重新计算切割平面与网格的交线
    planeIntersection = MC::computePlaneIntersections(mesh, cuttingPlanes);
    
    // 更新交线VAO/VBO
    MC::updateIntersectionVAO(planeIntersection, intersectionVAO, intersectionVBO);

    // 重新计算爆炸视图
    explodedView = MC::computeExplodedView(mesh, cuttingPlanes, explosionAxis);
    
    // 恢复之前的爆炸设置
    explodedView.enabled = wasEnabled;
    explodedView.explosionDistance = currentExplosionDistance;
    
    // 更新爆炸视图位移
    MC::updateExplodedViewDisplacements(explodedView, explosionAxis, currentExplosionDistance);

    // 为爆炸视图中的每个片段设置VAO/VBO/EBO
    for (auto &segment : explodedView.segments)
    {
        MC::setupSegmentMesh(segment);
    }

    // 更新记录的爆炸距离
    lastExplosionDistance = currentExplosionDistance;
}


int main()
{
    using namespace MC;

    // initialize OpenGL
    GLFWwindow *window = initOpenGL();
    if (!window)
    {
        return -1;
    }

    // initialize ImGui
    setupImGui(window);

    // symbol for explosion axis recalculate
    bool recalculateExplosionAxis = false;
    ImGui::GetIO().UserData = &recalculateExplosionAxis;

    PostProcess postProcessor;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (!postProcessor.init(width, height))
    {
        std::cerr << "Failed to initialize post processor\n";
        glfwTerminate();
        return -1;
    }

    // 记录初始窗口大小，用于检测调整大小
    int lastWidth = width, lastHeight = height;
    // open NIfTI file
    std::string filePath = openNiftiFileDialog();
    if (filePath.empty())
    {
        std::cerr << "No file selected.\n";
        glfwTerminate();
        return -1;
    }

    // load volume data
    VolumeData volumeData;
    if (!loadNiiFile(filePath, volumeData))
    {
        std::cerr << "Failed to load file\n";
        glfwTerminate();
        return -1;
    }

    // initialize mesh and Marching Cubes
    Mesh mesh;
    float isoLevel = 50.0f;
    float tempIsoLevel = isoLevel;
    generateMesh(volumeData, isoLevel, mesh);

    // Compute symmetry/explosion axis
    std::string currentExplosionStrategy = MC::ExplosionAxisConfig::convertToUIName(
        MC::getCurrentExplosionStrategyName());
    Vec3 explosionAxis = MC::computeExplosionAxis(mesh.vertices);
    std::cout << "Computed explosion axis: ("
              << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";

    // Create axis line for visualization
    Vertex axisStart, axisEnd;
    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;

    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    // Create VAO/VBO for the symmetry axis line
    unsigned int axisVAO, axisVBO;
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    Vertex axisLine[2] = {axisStart, axisEnd};
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Create shader program for the mesh
    unsigned int shaderProgram = createShaderProgram();

    // Create shader program for the symmetry axis line
    unsigned int lineShaderProgram = createLineShaderProgram();

    // create VAO/VBO/EBO for the mesh
    unsigned int VAO, VBO, EBO;
    setupMesh(mesh, VAO, VBO, EBO);

    // 生成切割平面
    std::vector<CuttingPlane> cuttingPlanes = generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3);

    // 计算切割平面与网格的交线
    PlaneIntersection planeIntersection = computePlaneIntersections(mesh, cuttingPlanes);

    // 创建用于显示交线的VAO/VBO
    unsigned int intersectionVAO, intersectionVBO;
    setupIntersectionVAO(planeIntersection, intersectionVAO, intersectionVBO);

    // 计算爆炸视图
    ExplodedView explodedView = computeExplodedView(mesh, cuttingPlanes, explosionAxis);

    // 为爆炸视图中的每个片段设置VAO/VBO/EBO
    for (auto &segment : explodedView.segments)
    {
        setupSegmentMesh(segment);
    }

    // 记录状态变量
    static std::string lastStrategy = currentExplosionStrategy;
    static float lastIsoLevel = isoLevel;
    static float lastExplosionDistance = explodedView.explosionDistance;

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &width, &height);
        if (width != lastWidth || height != lastHeight)
        {
            postProcessor.resize(width, height);
            lastWidth = width;
            lastHeight = height;
        }

        // 更新爆炸视图状态
        explodedView.enabled = showIntersections;

        // 渲染当前帧
        if (!explodedView.enabled)
        {
            // 正常渲染完整模型
            renderFrame(window, shaderProgram, VAO, mesh, MC::camera, isoLevel,
                        tempIsoLevel, volumeData, currentExplosionStrategy, postProcessor, axisVAO,
                        intersectionVAO, planeIntersection.segments.size());
        }
        else
        {
            // 渲染爆炸视图
            renderExplodedView(window, explodedView, shaderProgram, mesh, isoLevel, tempIsoLevel, volumeData);
        }

        // 处理爆炸距离更新 (通过UI控制后)
        if (explodedView.explosionDistance != lastExplosionDistance)
        {
            updateExplodedViewDisplacements(explodedView, explosionAxis, explodedView.explosionDistance);
            lastExplosionDistance = explodedView.explosionDistance;
        }

        // 如果ISO值已更改，重新生成网格
        if (isoLevel != lastIsoLevel)
        {
            handleIsoLevelChange(
                volumeData, isoLevel, mesh, VAO, VBO, EBO,
                explosionAxis, axisStart, axisEnd, axisVBO,
                cuttingPlanes, planeIntersection, intersectionVAO, intersectionVBO,
                explodedView);
            
            lastIsoLevel = isoLevel;
        }

        // 从ImGui获取是否需要手动重新计算的标志
        bool shouldRecalculate = recalculateExplosionAxis;
        if (shouldRecalculate || lastStrategy != currentExplosionStrategy)
        {
            recalculateExplosionAxisAndView(
                mesh, explosionAxis, currentExplosionStrategy, lastStrategy,
                axisStart, axisEnd, axisVBO,
                cuttingPlanes, planeIntersection, intersectionVAO, intersectionVBO,
                explodedView, lastExplosionDistance);
            
            // 重置标志
            recalculateExplosionAxis = false;
        }

        // 交换缓冲区并处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteVertexArrays(1, &intersectionVAO);
    glDeleteBuffers(1, &intersectionVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(lineShaderProgram);

    postProcessor.cleanup();

    // 清理爆炸视图资源
    cleanupExplodedView(explodedView);

    // 清理ImGui
    cleanupImGui();

    // 终止GLFW
    glfwTerminate();
    return 0;
}