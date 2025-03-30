#include "headers/data.h"
#include "headers/marching_cubes.h"
#include "headers/visual.h"
#include "headers/symmetry_axis.h" // Add this include
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
    float isoLevel = 10.0f; 
    float tempIsoLevel = isoLevel;
    generateMesh(volumeData, isoLevel, mesh);

    // Compute symmetry/explosion axis
    Vec3 explosionAxis = computeExplosionAxis(mesh.vertices);
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

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        // 渲染当前帧
        renderFrame(window, shaderProgram, VAO, mesh, MC::camera, isoLevel, tempIsoLevel, volumeData);

        // Draw the symmetry axis line
        glLineWidth(5.0f); // Set line width
        glUseProgram(lineShaderProgram);

        // Get viewport size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        // Build transform matrices (same as in renderFrame)
        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / mesh.max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // Center model
        glm::vec3 glmCenter(mesh.center.x, mesh.center.y, mesh.center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // Pass uniforms to line shader
        int lineModelLoc = glGetUniformLocation(lineShaderProgram, "model");
        int lineViewLoc = glGetUniformLocation(lineShaderProgram, "view");
        int lineProjLoc = glGetUniformLocation(lineShaderProgram, "projection");

        glUniformMatrix4fv(lineModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(lineViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lineProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw explosion axis line
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);

        // 如果ISO值已更改，重新生成网格
        static float lastIsoLevel = isoLevel;
        if (isoLevel != lastIsoLevel)
        {
            generateMesh(volumeData, isoLevel, mesh);
            updateMesh(mesh, VAO, VBO, EBO);

            // Recompute the symmetry axis when mesh changes
            explosionAxis = computeExplosionAxis(mesh.vertices);
            std::cout << "Updated explosion axis: ("
                      << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";

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

            lastIsoLevel = isoLevel;
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
    glDeleteProgram(shaderProgram);
    glDeleteProgram(lineShaderProgram);

    // 清理ImGui
    cleanupImGui();

    // 终止GLFW
    glfwTerminate();
    return 0;
}