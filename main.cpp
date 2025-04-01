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
        static float lastExplosionDistance = explodedView.explosionDistance;
        if (explodedView.explosionDistance != lastExplosionDistance)
        {
            updateExplodedViewDisplacements(explodedView, explosionAxis, explodedView.explosionDistance);
            lastExplosionDistance = explodedView.explosionDistance;
        }

        // 如果ISO值已更改，重新生成网格
        static float lastIsoLevel = isoLevel;
        if (isoLevel != lastIsoLevel)
        {
            generateMesh(volumeData, isoLevel, mesh);
            updateMesh(mesh, VAO, VBO, EBO);

            // Update the axis line vertices
            float halfLength = mesh.max_dimension * 2.0f;
            axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
            axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
            axisStart.z = mesh.center.z - explosionAxis.z * halfLength;

            axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
            axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
            axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

            // Update the axis line VBO
            Vertex axisLine[2] = {axisStart, axisEnd};
            glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

            // 重新生成切割平面
            cuttingPlanes = generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3);

            // 重新计算切割平面与网格的交线
            planeIntersection = computePlaneIntersections(mesh, cuttingPlanes);

            // 更新交线VAO/VBO
            updateIntersectionVAO(planeIntersection, intersectionVAO, intersectionVBO);

            // 重新计算爆炸视图
            cleanupExplodedView(explodedView);
            explodedView = computeExplodedView(mesh, cuttingPlanes, explosionAxis);

            // 为爆炸视图中的每个片段设置VAO/VBO/EBO
            for (auto &segment : explodedView.segments)
            {
                setupSegmentMesh(segment);
            }

            lastIsoLevel = isoLevel;
        }

        static std::string lastStrategy = currentExplosionStrategy;

        // 从ImGui获取是否需要手动重新计算的标志
        bool shouldRecalculate = recalculateExplosionAxis;
        if (shouldRecalculate || lastStrategy != currentExplosionStrategy)
        {
            // 检查策略是否变化
            if (lastStrategy != currentExplosionStrategy)
            {
                std::cout << "Strategy changed from " << lastStrategy
                          << " to " << currentExplosionStrategy << std::endl;
                lastStrategy = currentExplosionStrategy;
            }

            // 重新计算爆炸轴
            explosionAxis = computeExplosionAxis(mesh.vertices);
            std::cout << "Updated explosion axis: ("
                      << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";
            std::cout << "Using strategy: " << currentExplosionStrategy << std::endl;

            // 更新轴线顶点
            float halfLength = mesh.max_dimension * 2.0f;
            axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
            axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
            axisStart.z = mesh.center.z - explosionAxis.z * halfLength;

            axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
            axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
            axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

            // 更新轴线VBO
            Vertex axisLine[2] = {axisStart, axisEnd};
            glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

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