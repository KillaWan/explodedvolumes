#include "headers/visual.h"

// GLAD & GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//explosion strategy
#include "explosionaxis/explosion_axis_strategy.h"

#include "post_processor.h"

namespace MC
{

    // 全局变量
    Camera camera;
    bool firstMouse = true;
    float lastX, lastY;
    bool showIntersections = false; // 控制爆炸视图的显示
    bool showErrorPopup = false;
    std::string errorPopupMessage = "";
    std::string errorPopupTitle = "Error";
    //---------------------- 着色器源代码 ----------------------

    const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Calculate normal using derivatives
out vec3 FragPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

    const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;

void main()
{
    // Calculate normal using derivative functions
    vec3 normal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    
    // Basic lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 ambient = vec3(0.2, 0.2, 0.2);
    
    FragColor = vec4(ambient + diffuse, 1.0);
}
)glsl";

    // Line shader for symmetry axis visualization
    const char *lineVertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

    const char *lineFragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black color for the axis
}
)glsl";

    //---------------------- 回调函数实现 ----------------------

    // Window size change callback
    void framebuffer_size_callback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    // Mouse callback function
    void mouse_callback(GLFWwindow *window, double xpos, double ypos)
    {
        // 检查ImGui是否需要鼠标
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
            return;
        }

        // 只在按下左键时旋转
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // 反转

            const float sensitivity = 0.1f;
            camera.rotationY += xoffset * sensitivity;
            camera.rotationX += yoffset * sensitivity;

            // 限制俯仰角
            if (camera.rotationX > 89.0f)
                camera.rotationX = 89.0f;
            if (camera.rotationX < -89.0f)
                camera.rotationX = -89.0f;
        }

        lastX = xpos;
        lastY = ypos;
    }

    // Scroll callback for zooming
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
    {
        std::cout << "Scroll event: yoffset = " << yoffset << std::endl;
        camera.zoom -= yoffset * 0.2f; // 增加灵敏度

        // // 限制缩放范围
        // if (camera.zoom < 0.1f)
        //     camera.zoom = 0.1f;
        // if (camera.zoom > 6.0f)
        //     camera.zoom = 6.0f;
    }

    // Key callback for additional controls
    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // 使用 + 和 - 键调整相机距离
        if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera.distance *= 0.9f; // 拉近
        }
        if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera.distance *= 1.1f; // 拉远
        }

        // 不再使用I键切换交线显示，改为完全通过UI控制
        // 保留此处以便以后可以添加其他键盘快捷键
    }

    //---------------------- 着色器函数 ----------------------

    // 编译着色器
    unsigned int compileShader(unsigned int type, const char *src)
    {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        }
        return shader;
    }

    // 创建着色器程序
    unsigned int createShaderProgram()
    {
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        unsigned int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    // Create shader program for line rendering
    unsigned int createLineShaderProgram()
    {
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, lineVertexShaderSource);
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, lineFragmentShaderSource);

        unsigned int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Line shader program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    //---------------------- OpenGL函数 ----------------------

    // 设置网格的VAO/VBO/EBO
    void setupMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex),
                     mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(IndexType),
                     mesh.indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // 更新网格数据
    void updateMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex),
                     mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(IndexType),
                     mesh.indices.data(), GL_STATIC_DRAW);
    }
    
    void renderExplosionAxisGUI(std::string& currentStrategy)
{
    if (ImGui::Begin("Explosion Axis Settings"))
    {
        // 获取当前配置
        MC::ExplosionAxisConfig& config = MC::getExplosionAxisConfig();
        
        // 获取当前使用的内部策略名称
        std::string currentInternalStrategy = MC::getCurrentExplosionStrategyName();
        
        // 将内部策略名称转换为UI友好名称
        if (currentStrategy.empty()) {
            currentStrategy = MC::ExplosionAxisConfig::convertToUIName(currentInternalStrategy);
        }
        
        // 获取可用的UI友好策略名称
        std::vector<std::string> strategies = MC::ExplosionAxisConfig::getStrategyNames();
        
        // 创建策略名称的字符数组，用于ImGui::Combo
        std::vector<const char*> strategyNames;
        for (const auto& strategy : strategies) {
            strategyNames.push_back(strategy.c_str());
        }
        
        // 找到当前策略在列表中的索引
        int currentIndex = 0;
        for (size_t i = 0; i < strategies.size(); ++i) {
            if (strategies[i] == currentStrategy) {
                currentIndex = static_cast<int>(i);
                break;
            }
        }
        
        // 存储选择前的策略
        static std::string previousStrategy = currentStrategy;
        
        // 创建下拉选择框
        if (ImGui::Combo("Explosion Axis Strategy", &currentIndex, strategyNames.data(), static_cast<int>(strategyNames.size())))
        {
            // 用户选择了新的策略，仅更新UI显示，不立即应用
            currentStrategy = strategies[currentIndex];
            
            // 只在UI上显示已选择新策略，但不实际应用
            if (previousStrategy != currentStrategy) {
                ImGui::Text("Selected: %s (Click 'Recalculate' to apply)", currentStrategy.c_str());
            }
        }
        
        // 根据选择的策略显示不同的参数设置
        ImGui::Separator();
        
        // 检查是否需要显示错误弹窗
        static bool errorShown = false;
        bool shouldShowError = false;
        std::string errorMessage;
        
        // 通用检测失败
        if (!config.lastDetectionSuccessful) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), 
                "No symmetry detected with current strategy!");
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                "Using previous axis as fallback.");
            ImGui::Separator();
            
            shouldShowError = true;
            errorMessage = "No symmetry detected with the current strategy!\n\n"
                          "Using previous axis as fallback.\n\n"
                          "Try adjusting the parameters or selecting a different strategy.";
        }
        
        if (currentStrategy == "Rotational Symmetry") {
            ImGui::Text("Rotational Symmetry Parameters:");
            
            // 特定策略的状态提示
            if (!config.rotationalDetectionSuccessful && config.lastDetectionSuccessful) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                    "No rotational symmetry detected in last analysis.");
                
                shouldShowError = true;
                errorMessage = "No rotational symmetry detected in last analysis.\n\n"
                              "Try adjusting the sample count or symmetry order,\n"
                              "or switch to a different strategy.";
            }
            
            // 采样点数量
            if (ImGui::SliderInt("Sample Count##rot", &config.rotationSampleCount, 10, 5000)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            
            // 对称阶数
            if (ImGui::SliderInt("Symmetry Order", &config.rotationSymmetryOrder, 2, 12)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            
            // 自定义旋转轴选项
            if (ImGui::Checkbox("Use Custom Rotation Axis", &config.useCustomRotationAxis)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            
            if (config.useCustomRotationAxis) {
                bool axisChanged = false;
                axisChanged |= ImGui::DragFloat("Axis X##rot", &config.rotationAxis.x, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Y##rot", &config.rotationAxis.y, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Z##rot", &config.rotationAxis.z, 0.01f, -1.0f, 1.0f);
                
                if (axisChanged) {
                    // 修改后仅更新配置但不重新计算
                    MC::applyExplosionAxisConfig(config);
                }
                
                // 归一化轴向量
                float length = std::sqrt(
                    config.rotationAxis.x * config.rotationAxis.x +
                    config.rotationAxis.y * config.rotationAxis.y +
                    config.rotationAxis.z * config.rotationAxis.z
                );
                
                if (length > 0.0001f) {
                    ImGui::Text("Normalized Axis: (%.2f, %.2f, %.2f)", 
                                config.rotationAxis.x / length,
                                config.rotationAxis.y / length,
                                config.rotationAxis.z / length);
                }
            }
        }
        else if (currentStrategy == "Reflective Symmetry") {
            ImGui::Text("Reflective Symmetry Parameters:");
            
            // 特定策略的状态提示
            if (!config.reflectiveDetectionSuccessful && config.lastDetectionSuccessful) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                    "No reflective symmetry detected in last analysis.");
                
                shouldShowError = true;
                errorMessage = "No reflective symmetry detected in last analysis.\n\n"
                              "Try adjusting the sample count or providing a custom mirror normal,\n"
                              "or switch to a different strategy.";
            }
            
            // 采样点数量
            if (ImGui::SliderInt("Sample Count##mirror", &config.mirrorSampleCount, 10, 5000)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            
            // 自定义镜像轴选项
            if (ImGui::Checkbox("Use Custom Mirror Normal", &config.useCustomMirrorNormal)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            
            if (config.useCustomMirrorNormal) {
                bool normalChanged = false;
                normalChanged |= ImGui::DragFloat("Normal X##mirror", &config.mirrorNormal.x, 0.01f, -1.0f, 1.0f);
                normalChanged |= ImGui::DragFloat("Normal Y##mirror", &config.mirrorNormal.y, 0.01f, -1.0f, 1.0f);
                normalChanged |= ImGui::DragFloat("Normal Z##mirror", &config.mirrorNormal.z, 0.01f, -1.0f, 1.0f);
                
                if (normalChanged) {
                    // 修改后仅更新配置但不重新计算
                    MC::applyExplosionAxisConfig(config);
                }
                
                // 归一化法向量
                float length = std::sqrt(
                    config.mirrorNormal.x * config.mirrorNormal.x +
                    config.mirrorNormal.y * config.mirrorNormal.y +
                    config.mirrorNormal.z * config.mirrorNormal.z
                );
                
                if (length > 0.0001f) {
                    ImGui::Text("Normalized Normal: (%.2f, %.2f, %.2f)", 
                                config.mirrorNormal.x / length,
                                config.mirrorNormal.y / length,
                                config.mirrorNormal.z / length);
                }
            }
        }
        else if (currentStrategy == "PCA (Longest Axis)") {
            ImGui::Text("PCA Parameters:");
            if (ImGui::Checkbox("Use Longest Principal Axis", &config.useLongestAxis)) {
                // 修改后仅更新配置但不重新计算
                MC::applyExplosionAxisConfig(config);
            }
            if (!config.useLongestAxis) {
                ImGui::Text("Will use shortest principal axis instead");
            }
        }
        else if (currentStrategy == "Combined") {
            ImGui::Text("Combined Strategy Parameters:");
            ImGui::TextWrapped("Priority order: Rotational Symmetry -> Reflective Symmetry -> PCA");
            
            // 策略检测状态提示
            if (!config.rotationalDetectionSuccessful) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                    "No rotational symmetry detected.");
            }
            
            if (!config.reflectiveDetectionSuccessful) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                    "No reflective symmetry detected.");
            }
            
            // 显示各个子策略的一些基本设置
            if (ImGui::CollapsingHeader("Rotational Symmetry Settings")) {
                bool changed = false;
                changed |= ImGui::SliderInt("Sample Count##combined_rot", &config.rotationSampleCount, 10, 5000);
                changed |= ImGui::SliderInt("Symmetry Order##combined", &config.rotationSymmetryOrder, 2, 12);
                
                if (changed) {
                    // 修改后仅更新配置但不重新计算
                    MC::applyExplosionAxisConfig(config);
                }
            }
            
            if (ImGui::CollapsingHeader("Reflective Symmetry Settings")) {
                if (ImGui::SliderInt("Sample Count##combined_mirror", &config.mirrorSampleCount, 10, 5000)) {
                    // 修改后仅更新配置但不重新计算
                    MC::applyExplosionAxisConfig(config);
                }
            }
            
            if (ImGui::CollapsingHeader("PCA Settings")) {
                if (ImGui::Checkbox("Use Longest Principal Axis##combined_pca", &config.useLongestAxis)) {
                    // 修改后仅更新配置但不重新计算
                    MC::applyExplosionAxisConfig(config);
                }
            }
        }
        
        // 如果应该显示错误弹窗且之前未显示过
        if (shouldShowError && !errorShown) {
            MC::showError("Symmetry Detection Failed", errorMessage);
            errorShown = true;
        } else if (!shouldShowError) {
            // 重置错误显示标志
            errorShown = false;
        }
        
        ImGui::Separator();
        
        // 显示说明信息
        if (previousStrategy != currentStrategy) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), 
                "Strategy change will be applied when you click 'Recalculate'");
        }
        
        // 添加重新计算按钮
        if (ImGui::Button("Recalculate Explosion Axis", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)))
        {
            // 如果策略已更改，立即应用新策略
            if (previousStrategy != currentStrategy) {
                std::string newInternalStrategy = MC::ExplosionAxisConfig::convertToInternalName(currentStrategy);
                config.strategyName = newInternalStrategy;
                MC::setExplosionStrategy(newInternalStrategy);
                MC::applyExplosionAxisConfig(config);
                previousStrategy = currentStrategy;  // 更新先前策略记录
            }
            
            // 设置重新计算标志
            *reinterpret_cast<bool*>(ImGui::GetIO().UserData) = true;
            // 重置错误显示标志，以便在重新计算后能显示新的错误
            errorShown = false;
        }
    }
    ImGui::End();
    
    // 调用渲染弹出窗口的函数
    MC::renderErrorPopup();
}

    // 设置弹出错误窗口的函数
    void showError(const std::string& title, const std::string& message)
    {
        errorPopupTitle = title;
        errorPopupMessage = message;
        showErrorPopup = true;
    }
    
    // 渲染错误弹出窗口的函数
    void renderErrorPopup()
    {
        if (showErrorPopup)
        {
            // 设置弹出窗口大小和位置
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
            
            // 创建模态弹出窗口
            if (ImGui::BeginPopupModal(errorPopupTitle.c_str(), &showErrorPopup, 
                                      ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextWrapped("%s", errorPopupMessage.c_str());
                ImGui::Separator();
                
                // 创建居中的确认按钮
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);
                if (ImGui::Button("OK", ImVec2(120, 0)) || 
                    ImGui::IsKeyPressed(ImGuiKey_Escape) || 
                    ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    showErrorPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
            
            // 如果showErrorPopup为true，打开弹出窗口
            if (showErrorPopup)
            {
                ImGui::OpenPopup(errorPopupTitle.c_str());
            }
        }
    }

    // 渲染一帧
    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     std::string &currentExplosionStrategy, PostProcess &postProcessor,
                    unsigned int axisVAO,
                     unsigned int intersectionVAO, int numIntersectionSegments){
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        // 绑定后处理FBO，渲染到纹理
        glBindFramebuffer(GL_FRAMEBUFFER, postProcessor.getFBO());
        // clean screen
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // sart ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui panel
        ImGui::Begin("Marching Cubes Control");
        // TODO: change the range from 0 to 100
        ImGui::Text("Data range: %.1f to %.1f", volumeData.minValue, volumeData.maxValue);

        // Slider from min_val to max_val
        if (ImGui::SliderFloat("ISO Level", &tempIsoLevel, volumeData.minValue, volumeData.maxValue, "%.1f"))
        {
            // (No immediate recalc here if you want to wait for a button)
        }

        // 应用ISO值的按钮
        if (ImGui::Button("Apply ISO"))
        {
            isoLevel = tempIsoLevel;
            // 调用者需处理网格重新生成
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f", isoLevel);

        // 添加爆炸视图控制
        ImGui::Separator();
        if (ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections))
        {
            std::cout << "Model explosion: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }

        ImGui::End(); // end ImGui panel

        renderExplosionAxisGUI(currentExplosionStrategy);
        
        // 渲染弹出错误窗口
        renderErrorPopup();

        // 渲染ImGui界面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // viewport size
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        // use shader
        glUseProgram(shaderProgram);

        // build transform
        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / mesh.max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // center model
        glm::vec3 glmCenter(mesh.center.x, mesh.center.y, mesh.center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // pass uniform
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        unsigned int axisShader = createLineShaderProgram();
        glUseProgram(axisShader);

        // 将之前计算的 model、view、projection 传入该着色器
        int axisModelLoc = glGetUniformLocation(axisShader, "model");
        int axisViewLoc = glGetUniformLocation(axisShader, "view");
        int axisProjLoc = glGetUniformLocation(axisShader, "projection");
        glUniformMatrix4fv(axisModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(axisViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(axisProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // 设置线宽
        glLineWidth(20.0f);

        // 绑定爆炸轴 VAO 并绘制线段（两个顶点）
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);

        // 如果启用了交线显示，绘制交线
        if (showIntersections && intersectionVAO != 0 && numIntersectionSegments > 0)
        {
            // 使用简单的着色器来绘制红色线段
            unsigned int simpleColorShader = createLineShaderProgram();
            glUseProgram(simpleColorShader);

            // 传递变换矩阵
            int modelLoc = glGetUniformLocation(simpleColorShader, "model");
            int viewLoc = glGetUniformLocation(simpleColorShader, "view");
            int projLoc = glGetUniformLocation(simpleColorShader, "projection");

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            // 设置线宽和颜色
            glLineWidth(3.0f);

            // 绘制交线
            glBindVertexArray(intersectionVAO);
            glDrawArrays(GL_LINES, 0, numIntersectionSegments * 2);
            glBindVertexArray(0);

            // 恢复线宽
            glLineWidth(1.0f);

            // 删除着色器程序
            glDeleteProgram(simpleColorShader);
        }

        // 切换回默认帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 应用后处理效果
        postProcessor.render(width, height);

        // render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // 爆炸视图渲染函数
    void MC::renderExplodedView(
        GLFWwindow *window,
        const ExplodedView &explodedView,
        unsigned int shaderProgram,
        const Mesh &mesh,
        float &isoLevel,
        float &tempIsoLevel,
        const VolumeData &volumeData)
    {
        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui面板
        ImGui::Begin("Marching Cubes Control");
        ImGui::Text("Data range: %.1f to %.1f", volumeData.minValue, volumeData.maxValue);
        if (ImGui::SliderFloat("ISO Level", &tempIsoLevel, volumeData.minValue, volumeData.maxValue, "%.1f"))
        {
        }
        if (ImGui::Button("Apply ISO"))
        {
            isoLevel = tempIsoLevel;
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f", isoLevel);

        // 爆炸视图控制
        ImGui::Separator();
        ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections);

        // 爆炸距离控制
        if (showIntersections)
        {
            float explosionDistance = explodedView.explosionDistance;
            if (ImGui::SliderFloat("Explosion Distance", &explosionDistance, 0.5f, 10.0f, "%.1f"))
            {
                // 注意：这里需要修改 explodedView，但由于它是 const 参数，
                // 需要调用方在外部处理这个距离变更
            }
        }

        ImGui::End();

        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 视口大小
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        // 构建变换矩阵
        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / mesh.max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // 居中模型
        glm::vec3 glmCenter(mesh.center.x, mesh.center.y, mesh.center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // 对每个片段进行渲染
        if (explodedView.enabled)
        {
            for (const auto &segment : explodedView.segments)
            {
                if (segment.indices.empty() || segment.VAO == 0)
                {
                    continue;
                }

                glUseProgram(shaderProgram);

                // 创建模型矩阵，应用位移
                glm::mat4 segmentModel = model;
                segmentModel = glm::translate(segmentModel, glm::vec3(
                                                                segment.displacement.x,
                                                                segment.displacement.y,
                                                                segment.displacement.z));

                // 传递变换矩阵
                int modelLoc = glGetUniformLocation(shaderProgram, "model");
                int viewLoc = glGetUniformLocation(shaderProgram, "view");
                int projLoc = glGetUniformLocation(shaderProgram, "projection");

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(segmentModel));
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

                // 绘制片段
                glBindVertexArray(segment.VAO);
                glDrawElements(GL_TRIANGLES, segment.indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        // 渲染ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }


    // 设置ImGui
    void setupImGui(GLFWwindow *window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    // 清理ImGui
    void cleanupImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // initialize OpenGL
    GLFWwindow *initOpenGL()
    {
        // initialize GLFW
        if (!glfwInit())
        {
            std::cerr << "GLFW init failed\n";
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        GLFWwindow *window = glfwCreateWindow(800, 600, "Marching Cubes Visualization", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetKeyCallback(window, key_callback);

        // initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to init GLAD\n";
            return nullptr;
        }

        // 启用深度测试和背面剔除
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        return window;
    }

} // namespace MC