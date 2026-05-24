#include "headers/explosionaxis/explosion_axis_strategy.h"
#include "headers/planes/selecting_planes.h"
#include "headers/planes/cutting_planes.h"
#include "headers/planes/exploded_view.h"
#include "headers/marching_cubes.h"
#include "headers/visual.h"
#include "post_processor.h"
#include "headers/data.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>
#include <algorithm> // for std::max

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLAD & GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for matrix transformations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// isoLevel change
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
    MC::ExplodedView &explodedView)
{
    // regenerate Mesh
    MC::generateMesh(volumeData, newIsoLevel, mesh);
    MC::updateMesh(mesh, VAO, VBO, EBO);

    // update axis vertex
    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;
    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    // update axis VBO
    MC::Vertex axisLine[2] = {axisStart, axisEnd};
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

    // regenerate cutting planes
    cuttingPlanes = MC::generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3);

    // recalculate exploded view
    MC::cleanupExplodedView(explodedView);
    explodedView = computeExplodedView(mesh, cuttingPlanes, explosionAxis, MC::g_explosionDistance);

    // set up VAO/VBO/EBO for each clip in the exploded view
    for (auto &segment : explodedView.segments)
    {
        MC::setupSegmentMesh(segment);
    }
}

// explosion axis change
void recalculateExplosionAxisAndView(
    MC::Mesh &mesh,
    MC::Vec3 &explosionAxis,
    std::string &currentExplosionStrategy,
    std::string &lastStrategy,
    MC::Vertex &axisStart,
    MC::Vertex &axisEnd,
    unsigned int axisVBO,
    std::vector<MC::CuttingPlane> &cuttingPlanes,
    MC::ExplodedView &explodedView,
    float &lastExplosionDistance)
{
    if (lastStrategy != currentExplosionStrategy)
    {
        std::cout << "Strategy changed from " << lastStrategy
                  << " to " << currentExplosionStrategy << std::endl;
        lastStrategy = currentExplosionStrategy;
    }

    float currentExplosionDistance = explodedView.explosionDistance;
    bool wasEnabled = explodedView.enabled;
    
    MC::cleanupExplodedView(explodedView);
    explosionAxis = MC::computeExplosionAxis(mesh.vertices);
    std::cout << "Updated explosion axis: ("
              << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";
    
    float length = std::sqrt(
        explosionAxis.x * explosionAxis.x +
        explosionAxis.y * explosionAxis.y +
        explosionAxis.z * explosionAxis.z);
    if (length > 0.0001f) {
        explosionAxis.x /= length;
        explosionAxis.y /= length;
        explosionAxis.z /= length;
    }

    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;
    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    MC::Vertex axisLine[2] = {axisStart, axisEnd};
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);

    cuttingPlanes = MC::generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3); // 3个切割平面
    explodedView = computeExplodedView(mesh, cuttingPlanes, explosionAxis, MC::g_explosionDistance);
    explodedView.enabled = wasEnabled;
    explodedView.explosionDistance = currentExplosionDistance;
    MC::updateExplodedViewDisplacements(explodedView, explosionAxis, currentExplosionDistance);
    for (auto &segment : explodedView.segments)
    {
        MC::setupSegmentMesh(segment);
    }

    lastExplosionDistance = currentExplosionDistance;
}


int main()
{
    using namespace MC;

    // initialize OpenGL
    GLFWwindow *window = initOpenGL();
    if (!window){ return -1;}

    // initialize ImGui
    setupImGui(window);
    bool imguiInitialized = true;

    auto cleanupAndExit = [&](int exitCode) {
        if (imguiInitialized)
        {
            cleanupImGui();
            imguiInitialized = false;
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return exitCode;
    };

    // symbol for explosion axis recalculate
    bool recalculateExplosionAxis = false;
    ImGui::GetIO().UserData = &recalculateExplosionAxis;

    // create post-process for rendering
    PostProcess postProcessor;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (!postProcessor.init(width, height))
    {
        std::cerr << "Failed to initialize post processor\n";
        postProcessor.cleanup();
        return cleanupAndExit(-1);
    }

    int lastWidth = width, lastHeight = height;

    // open NIfTI file
    std::string filePath = openNiftiFileDialog();
    if (filePath.empty())
    {
        std::cerr << "No file selected.\n";
        postProcessor.cleanup();
        return cleanupAndExit(-1);
    }

    // load volume data
    VolumeData volumeData;
    if (!loadNiiFile(filePath, volumeData))
    {
        std::cerr << "Failed to load file\n";
        postProcessor.cleanup();
        return cleanupAndExit(-1);
    }

    // initialize mesh and marching cubes
    Mesh mesh;
    float isoLevel = volumeData.minValue + (MC::g_isoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
    float tempIsoLevel = isoLevel;
    generateMesh(volumeData, isoLevel, mesh);

    // compute symmetry/explosion axis
    std::string currentExplosionStrategy = MC::ExplosionAxisConfig::convertToUIName(MC::getCurrentExplosionStrategyName());
    Vec3 explosionAxis = MC::computeExplosionAxis(mesh.vertices);
    std::cout << "Computed explosion axis: (" << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";
    Vertex axisStart, axisEnd;
    float halfLength = mesh.max_dimension * 2.0f;
    axisStart.x = mesh.center.x - explosionAxis.x * halfLength;
    axisStart.y = mesh.center.y - explosionAxis.y * halfLength;
    axisStart.z = mesh.center.z - explosionAxis.z * halfLength;
    axisEnd.x = mesh.center.x + explosionAxis.x * halfLength;
    axisEnd.y = mesh.center.y + explosionAxis.y * halfLength;
    axisEnd.z = mesh.center.z + explosionAxis.z * halfLength;

    // create VAO/VBO for the symmetry axis line
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

    // create shader program
    unsigned int shaderProgram = createShaderProgram();
    unsigned int lineShaderProgram = createLineShaderProgram();

    // create VAO/VBO/EBO for the mesh
    unsigned int VAO, VBO, EBO;
    setupMesh(mesh, VAO, VBO, EBO);

    // cutting plane & exploded view
    std::vector<CuttingPlane> cuttingPlanes = generateAdaptiveCuttingPlanes(mesh, explosionAxis, 3);
    ExplodedView explodedView = computeExplodedView(mesh, cuttingPlanes, explosionAxis);
    for (auto &segment : explodedView.segments)
    {
        setupSegmentMesh(segment);
    }

    // record current status
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

        // update exploded view
        explodedView.enabled = showIntersections;
        if (g_explosionDistance != explodedView.explosionDistance)
        {
            explodedView.explosionDistance = g_explosionDistance;
            updateExplodedViewDisplacements(explodedView, explosionAxis, explodedView.explosionDistance);
            lastExplosionDistance = explodedView.explosionDistance;
        }

        // render frame
        if (!explodedView.enabled)
        {
            renderFrame(window, shaderProgram, VAO, mesh, MC::camera, isoLevel,
                        tempIsoLevel, volumeData, currentExplosionStrategy, postProcessor,
                        axisVAO, lineShaderProgram);
        }
        else
        {
            renderExplodedView(window, explodedView, shaderProgram, mesh, isoLevel, tempIsoLevel, volumeData,
                               axisVAO, lineShaderProgram, postProcessor);
        }

        // explosion distance update after being controlled by ui
        if (explodedView.explosionDistance != lastExplosionDistance)
        {
            updateExplodedViewDisplacements(explodedView, explosionAxis, explodedView.explosionDistance);
            lastExplosionDistance = explodedView.explosionDistance;
        }
        tempIsoLevel = volumeData.minValue + (MC::g_tempIsoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);

        // regenerate mesh if iso value has been changed
        if (isoLevel != lastIsoLevel)
        {
            isoLevel = volumeData.minValue + (MC::g_isoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
            handleIsoLevelChange(
                volumeData, isoLevel, mesh, VAO, VBO, EBO,
                explosionAxis, axisStart, axisEnd, axisVBO,
                cuttingPlanes,
                explodedView);
            
            lastIsoLevel = isoLevel;
        }

        // if recalculation is required
        bool shouldRecalculate = recalculateExplosionAxis;
        if (shouldRecalculate)
        {
            recalculateExplosionAxisAndView(
                mesh, explosionAxis, currentExplosionStrategy, lastStrategy,
                axisStart, axisEnd, axisVBO,
                cuttingPlanes,
                explodedView, lastExplosionDistance);
            recalculateExplosionAxis = false;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // clean
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(lineShaderProgram);
    postProcessor.cleanup();
    cleanupExplodedView(explodedView);
    cleanupImGui();
    imguiInitialized = false;
    glfwDestroyWindow(window);

    // terminate GLFW
    glfwTerminate();

    return 0;
}
