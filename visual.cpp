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

// explosion strategy
#include "explosionaxis/explosion_axis_strategy.h"

#include "post_processor.h"

namespace MC
{

    // global variables
    Camera camera;
    bool firstMouse = true;
    float lastX, lastY;
    bool showIntersections = false; // control exploded view display
    bool showErrorPopup = false;
    std::string errorPopupMessage = "";
    std::string errorPopupTitle = "Error";
    float g_explosionDistancePercent = 87.5f;
    float g_explosionDistance = 35.0f;
    bool showExplosionAxis = false;
    float g_isoLevelPercent = 10.0f; // default 50%
    float g_tempIsoLevelPercent = 10.0f;
    float tempExplosionPercent = g_explosionDistancePercent;
    static bool panelFirstTime = true;

    // vertex shader source for mesh rendering
    const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

    // fragment shader source for basic lighting
    const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;

void main()
{
    vec3 normal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 ambient = vec3(0.2, 0.2, 0.2);
    
    FragColor = vec4(ambient + diffuse, 1.0);
}
)glsl";

    // line shader for symmetry axis visualization
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

    // imgui style
    void setupImGuiStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        // configure colors and style
        colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);         // 白色背景
        colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // 黑色文字
        colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);          // 浅灰色标题背景
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 较深的灰色活动标题
        colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);           // 浅灰色按钮
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // 悬停时稍深
        colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);     // 活动时更深
        colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);       // 非常浅的灰色框架背景
        colors[ImGuiCol_CheckMark] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);        // 深灰色复选标记
        colors[ImGuiCol_SliderGrab] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);       // 灰色滑块抓取
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // 活动时较深的滑块
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);   // 悬停时浅灰色框架
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 活动时稍深
        colors[ImGuiCol_Header] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);           // 浅灰色标头
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // 悬停时稍深
        colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);     // 活动时更深
        colors[ImGuiCol_Separator] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);        // 灰色分隔符
        colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);          // 白色弹出背景

        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(6.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;

        style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // 居中标题
    }

    // calculate viewport size
    void calculateViewport(GLFWwindow *window, int &viewportX, int &viewportY, int &viewportWidth, int &viewportHeight)
    {
        // window size
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // panel width and padding
        float panelWidth = 300.0f;
        float padding = 10.0f;

        // viewport size
        viewportX = 0;
        viewportY = 0;
        viewportWidth = windowWidth - panelWidth - padding * 2;
        viewportHeight = windowHeight;

        // ensure valid viewport dimensions
        if (viewportWidth < 1)
            viewportWidth = 1;
        if (viewportHeight < 1)
            viewportHeight = 1;
    }

    //  apply calculated viewport to OpenGL
    void updateViewport(GLFWwindow *window)
    {
        int viewportX, viewportY, viewportWidth, viewportHeight;
        calculateViewport(window, viewportX, viewportY, viewportWidth, viewportHeight);
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    }

    // window resize callback
    void framebuffer_size_callback(GLFWwindow *window, int width, int height)
    {
        updateViewport(window);
    }

    // mouse callback function
    void mouse_callback(GLFWwindow *window, double xpos, double ypos)
    {
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

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;

            const float sensitivity = 0.1f;
            camera.rotationY += xoffset * sensitivity;
            camera.rotationX += yoffset * sensitivity;

            // limit pitch angle
            if (camera.rotationX > 89.0f)
                camera.rotationX = 89.0f;
            if (camera.rotationX < -89.0f)
                camera.rotationX = -89.0f;
        }

        lastX = xpos;
        lastY = ypos;
    }

    // scroll callback for zooming
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
    {
        std::cout << "Scroll event: yoffset = " << yoffset << std::endl;
        camera.zoom -= yoffset * 0.2f;
    }

    // Key callback for additional controls
    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // adjust camera distance with + and - keys
        if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera.distance *= 0.9f; // 拉近
        }
        if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera.distance *= 1.1f; // 拉远
        }
    }

    // compile individual shader
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

    // create shader program for mesh rendering
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

    // create shader program for line rendering
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

    //---------------------- OpenGL ----------------------

    // set mesh VAO/VBO/EBO
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

    // update mesh
    void updateMesh(const Mesh &mesh, unsigned int &VAO, unsigned int &VBO, unsigned int &EBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex),
                     mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(IndexType),
                     mesh.indices.data(), GL_STATIC_DRAW);
    }

    void renderExplosionAxisGUI(std::string &currentStrategy)
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);         // 白色背景
        style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // 黑色文字
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);          // 浅灰色标题背景
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 较深的灰色活动标题
        style.Colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);           // 浅灰色按钮
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // 悬停时稍深
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);     // 活动时更深
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);       // 非常浅的灰色框架背景

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;
        float panelWidth = 300.0f;
        float spacing = 10.0f;
        float panelY = spacing + ImGui::GetFrameHeight() + spacing + 150.0f; // 在第一个面板下方

        if(panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - spacing, panelY));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, 0));    
        }

        if (ImGui::Begin("Explosion Axis Settings"))
        {
            MC::ExplosionAxisConfig &config = MC::getExplosionAxisConfig();
            std::string currentInternalStrategy = MC::getCurrentExplosionStrategyName();
            if (currentStrategy.empty())
            {
                currentStrategy = MC::ExplosionAxisConfig::convertToUIName(currentInternalStrategy);
            }

            std::vector<std::string> strategies = MC::ExplosionAxisConfig::getStrategyNames();

            std::vector<const char *> strategyNames;
            for (const auto &strategy : strategies)
            {
                strategyNames.push_back(strategy.c_str());
            }

            int currentIndex = 0;
            for (size_t i = 0; i < strategies.size(); ++i)
            {
                if (strategies[i] == currentStrategy)
                {
                    currentIndex = static_cast<int>(i);
                    break;
                }
            }

            static std::string previousStrategy = currentStrategy;
            ImGui::Text("Custom Explosion Axis");

            if (ImGui::Checkbox("Use Custom Axis", &config.useCustomExplosionAxis))
            {
                // 切换状态时的行为
                if (!config.useCustomExplosionAxis)
                {
                    // 如果取消自定义轴，恢复策略计算的轴
                    *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true; // 触发重新计算
                }

                // 应用配置更新
                MC::applyExplosionAxisConfig(config);
            }

            if (config.useCustomExplosionAxis)
            {
                // 自定义X,Y,Z轴控制
                bool axisChanged = false;
                axisChanged |= ImGui::DragFloat("Axis X", &config.customExplosionAxis.x, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Y", &config.customExplosionAxis.y, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Z", &config.customExplosionAxis.z, 0.01f, -1.0f, 1.0f);

                if (axisChanged)
                {
                    // 如果轴值改变，应用配置更新
                    MC::applyExplosionAxisConfig(config);
                }

                // 计算轴向量长度
                float length = std::sqrt(
                    config.customExplosionAxis.x * config.customExplosionAxis.x +
                    config.customExplosionAxis.y * config.customExplosionAxis.y +
                    config.customExplosionAxis.z * config.customExplosionAxis.z);

                // 显示归一化的轴向量
                if (length > 0.0001f)
                {
                    ImGui::Text("Normalized Axis: (%.2f, %.2f, %.2f)",
                                config.customExplosionAxis.x / length,
                                config.customExplosionAxis.y / length,
                                config.customExplosionAxis.z / length);
                }

                // 应用自定义轴的按钮
                if (ImGui::Button("Apply Custom Axis", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)))
                {
                    if (length > 0.0001f)
                    {
                        // 进行归一化
                        Vec3 normalizedAxis = {
                            config.customExplosionAxis.x / length,
                            config.customExplosionAxis.y / length,
                            config.customExplosionAxis.z / length};

                        // 更新配置中的自定义轴为归一化后的值
                        config.customExplosionAxis = normalizedAxis;

                        // 应用配置
                        MC::applyExplosionAxisConfig(config);

                        // 重绘模型 - 设置重新计算标志
                        *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true;

                        // 显示成功应用的提示
                        ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.0f, 1.0f), "Custom axis applied!");
                    }
                    else
                    {
                        MC::showError("Invalid Axis", "Cannot use a zero-length axis vector.\nPlease specify a valid direction.");
                    }
                }
            }

            ImGui::Separator();
            ImGui::Text("Strategy Selection");

            // 创建下拉选择框
            if (ImGui::Combo("##Explosion Axis Strategy", &currentIndex, strategyNames.data(), static_cast<int>(strategyNames.size())))
            {
                // 用户选择了新的策略，仅更新UI显示，不立即应用
                currentStrategy = strategies[currentIndex];

                // 只在UI上显示已选择新策略，但不实际应用
                if (previousStrategy != currentStrategy)
                {
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
            if (!config.lastDetectionSuccessful)
            {
                ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f),
                                   "No symmetry detected with current strategy!");
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
                                   "Using previous axis as fallback.");
                ImGui::Separator();

                shouldShowError = true;
                errorMessage = "No symmetry detected with the current strategy!\n\n"
                               "Using previous axis as fallback.\n\n"
                               "Try adjusting the parameters or selecting a different strategy.";
            }

            // 以下代码与原始函数相同，根据策略显示不同参数
            if (currentStrategy == "Rotational Symmetry")
            {
                // (Rotational Symmetry 的现有代码保持不变)
                ImGui::Text("Rotational Symmetry Parameters:");

                // 特定策略的状态提示
                if (!config.rotationalDetectionSuccessful && config.lastDetectionSuccessful)
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
                                       "No rotational symmetry detected in last analysis.");

                    shouldShowError = true;
                    errorMessage = "No rotational symmetry detected in last analysis.\n\n"
                                   "Try adjusting the sample count or symmetry order,\n"
                                   "or switch to a different strategy.";
                }

                // (其余 Rotational Symmetry 代码保持不变)
            }
            // (其他策略分支保持不变)

            // 在所有策略分支之后添加轴线可见性控制
            ImGui::Text("Explosion Axis Visibility");

            ImGui::Checkbox("Show Explosion Axis", &showExplosionAxis);

            // 如果应该显示错误弹窗且之前未显示过
            if (shouldShowError && !errorShown)
            {
                MC::showError("Symmetry Detection Failed", errorMessage);
                errorShown = true;
            }
            else if (!shouldShowError)
            {
                // 重置错误显示标志
                errorShown = false;
            }

            ImGui::Separator();

            // 显示说明信息
            if (previousStrategy != currentStrategy && !config.useCustomExplosionAxis)
            {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
                                   "Strategy change will be applied when you click 'Recalculate'");
            }

            // 添加重新计算按钮（仅在未选择自定义轴时显示）
            if (!config.useCustomExplosionAxis && ImGui::Button("Recalculate Explosion Axis", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)))
            {
                // 如果策略已更改，立即应用新策略
                if (previousStrategy != currentStrategy)
                {
                    std::string newInternalStrategy = MC::ExplosionAxisConfig::convertToInternalName(currentStrategy);
                    config.strategyName = newInternalStrategy;
                    MC::setExplosionStrategy(newInternalStrategy);
                    MC::applyExplosionAxisConfig(config);
                    previousStrategy = currentStrategy; // 更新先前策略记录
                }

                // 设置重新计算标志
                *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true;
                // 重置错误显示标志，以便在重新计算后能显示新的错误
                errorShown = false;
            }
        }
        ImGui::End();

        // 调用渲染弹出窗口的函数
        MC::renderErrorPopup();
    } 
    // 设置弹出错误窗口的函数
    void showError(const std::string &title, const std::string &message)
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
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 黑色文字
                ImGui::TextWrapped("%s", errorPopupMessage.c_str());
                ImGui::PopStyleColor();
                ImGui::Separator();

                // 创建居中的确认按钮
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);

                // 设置按钮样式
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

                if (ImGui::Button("OK", ImVec2(120, 0)) ||
                    ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                    ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    showErrorPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleColor(3); // 弹出按钮样式

                ImGui::EndPopup();
            }

            // 如果showErrorPopup为true，打开弹出窗口
            if (showErrorPopup)
            {
                ImGui::OpenPopup(errorPopupTitle.c_str());
            }
        }
    }

    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     std::string &currentExplosionStrategy, PostProcess &postProcessor,
                     unsigned int axisVAO)
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // 绑定后处理FBO，渲染到纹理
        glBindFramebuffer(GL_FRAMEBUFFER, postProcessor.getFBO());
        // 清屏
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 白色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 应用白色主题
        setupImGuiStyle();

        // 计算面板位置和大小以垂直排列
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;

        float panelWidth = 300.0f;                   // 所有面板的固定宽度
        float panelSpacing = 10.0f;                  // 面板间距
        float panel1Height = viewportSize.y * 0.2f; // 每个面板占屏幕高度的1/4
        float panel2Height = viewportSize.y * 0.45f;
        float panel3Height = viewportSize.y * 0.2f;

        // 面板1：主控制
        if (panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel1Height));
        }
        ImGui::Begin("Marching Cubes Control", nullptr);

        ImGui::Text("Data range: %.1f to %.1f", volumeData.minValue, volumeData.maxValue);

        // 从0-100的百分比滑块
        if (ImGui::SliderFloat("ISO Level (%)", &g_tempIsoLevelPercent, 0.0f, 100.0f, "%.1f%%"))
        {
            // 转换百分比到绝对值
            tempIsoLevel = volumeData.minValue + (g_tempIsoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
        }

        // 应用ISO值的按钮
        if (ImGui::Button("Apply ISO"))
        {
            g_isoLevelPercent = g_tempIsoLevelPercent;
            isoLevel = volumeData.minValue + (g_isoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue); 
            // 调用者需处理网格重新生成
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f (%.1f%%)", isoLevel, g_isoLevelPercent);

        ImGui::End(); // 结束主控制面板

        // 面板2：爆炸轴设置
        if (panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing * 2 + panel1Height));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel2Height));
        }
        renderExplosionAxisGUI(currentExplosionStrategy);

        // 面板3：爆炸视图控制
        if (panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, viewportSize.y - panel3Height - panelSpacing));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel3Height));
        }
        ImGui::Begin("Explosion View Control", nullptr);

        // 添加爆炸视图控制
        if (ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections))
        {
            std::cout << "Model explosion: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }

        // 爆炸距离控制
        const float MAX_EXPLOSION_DISTANCE = 40.0f; // 最大爆炸距离

        // 创建一个静态临时变量用于滑块
        static bool firstRun = true;
        if (firstRun)
        {
            tempExplosionPercent = g_explosionDistancePercent;
            firstRun = false;
        }
        ImGui::Text("Explosion Distance");
        if (showIntersections)
        {
            // 滑块控制临时值
            if (ImGui::SliderFloat("##Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%"))

            {
                // 不立即应用，等用户点击按钮确认
            }

            // 应用按钮
            if (ImGui::Button("Apply Distance"))
            {
                // 应用临时值到实际使用的变量
                g_explosionDistancePercent = tempExplosionPercent;
                g_explosionDistance = (g_explosionDistancePercent / 100.0f) * MAX_EXPLOSION_DISTANCE;
            }

            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
        }
        else
        {
            // 禁用状态下的滑块和按钮
            ImGui::BeginDisabled();
            ImGui::SliderFloat("Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%");
            if (ImGui::Button("Apply Distance"))
            {
                // 禁用状态不会触发
            }
            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
            ImGui::EndDisabled();
        }

        ImGui::End();

        // 渲染ImGui界面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        // 获取视口大小
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height != 0) ? (float)width / height : 1.0f;

        // 使用着色器
        glUseProgram(shaderProgram);

        // 构建变换
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

        // 传递uniform
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // 绘制
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        unsigned int axisShader = createLineShaderProgram();
        glUseProgram(axisShader);

        // 仅在 showExplosionAxis 为 true 时绘制轴线
        if (showExplosionAxis)
        {
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
        }

        // 切换回默认帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 应用后处理效果
        postProcessor.render(width, height);

        // 渲染ImGui
        ImGui::Render();
        // 仅在 showExplosionAxis 为 true 时绘制轴线
        if (showExplosionAxis)
        {
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

            // 绘制轴线 - 需要您确认是否有axisVAO
            glBindVertexArray(axisVAO);
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
        }
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        panelFirstTime = false;
    }
    void renderExplodedView(
        GLFWwindow *window,
        const ExplodedView &explodedView,
        unsigned int shaderProgram,
        const Mesh &mesh,
        float &isoLevel,
        float &tempIsoLevel,
        const VolumeData &volumeData,
        unsigned int axisVAO)
    {
        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 应用白色主题
        setupImGuiStyle();

        // 计算面板位置和大小以垂直排列
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;

        float panelWidth = 300.0f;                   // 所有面板的固定宽度
        float panelSpacing = 10.0f;                  // 面板间距
        float panel1Height = viewportSize.y * 0.2f; // 每个面板占屏幕高度的1/4
        float panel2Height = viewportSize.y * 0.45f;
        float panel3Height = viewportSize.y * 0.2f;

        // Panel 1: Main control
        ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panel1Height));
        
        ImGui::Begin("Marching Cubes Control", nullptr);

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
        ImGui::End();

        // Panel 2: Explosion axis
        ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing * 2 + panel1Height));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panel2Height));

        // Using current/default strategy
        std::string currentStrategy = "";
        renderExplosionAxisGUI(currentStrategy);

        // Panel 3: Explosion views
        ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, viewportSize.y - panel3Height - panelSpacing));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panel3Height));
        ImGui::Begin("Explosion View Control", nullptr);

        // Added control
        if (ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections))
        {
            std::cout << "Model explosion: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }

        // Distance control
        const float MAX_EXPLOSION_DISTANCE = 40.0f; // Maximum distance

        if (showIntersections)
        {
            // Calculate percent for display
            float currentDistancePercent = (explodedView.explosionDistance / MAX_EXPLOSION_DISTANCE) * 100.0f;
            static bool firstRun = true;
            if (firstRun)
            {
                tempExplosionPercent = g_explosionDistancePercent;
                firstRun = false;
            }

            // Temporary values
            if (ImGui::SliderFloat("Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%"))
            {
                // Waiting for user
            }

            // Applying button
            if (ImGui::Button("Apply Distance"))
            {
                // Applying temp values
                g_explosionDistancePercent = tempExplosionPercent;
                g_explosionDistance = (tempExplosionPercent / 100.0f) * MAX_EXPLOSION_DISTANCE;
            }

            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", currentDistancePercent);
        }
        else
        {
            // 禁用状态下的滑块和按钮
            ImGui::BeginDisabled();
            ImGui::SliderFloat("Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%");
            if (ImGui::Button("Apply Distance"))
            {
                // 禁用状态不会触发
            }
            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
            ImGui::EndDisabled();
        }

        ImGui::End();

        // 清屏
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 白色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Window Size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height != 0) ? (float)width / height : 1.0f;

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

        if (showExplosionAxis)
        {
            unsigned int axisShader = createLineShaderProgram();
            glUseProgram(axisShader);

            // 传递矩阵和渲染轴线的代码
            int axisModelLoc = glGetUniformLocation(axisShader, "model");
            int axisViewLoc = glGetUniformLocation(axisShader, "view");
            int axisProjLoc = glGetUniformLocation(axisShader, "projection");
            glUniformMatrix4fv(axisModelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(axisViewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(axisProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

            glLineWidth(20.0f);

            glBindVertexArray(axisVAO);
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
        }
    }

    // ImGui
    void setupImGui(GLFWwindow *window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;

        // 使用亮色主题而不是暗色主题
        ImGui::StyleColorsLight();

        // 应用我们的自定义白色主题
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        // 设置UI颜色为白色背景和黑色文字
        colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);         // 白色背景
        colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // 黑色文字
        colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);          // 浅灰色标题背景
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 较深的灰色活动标题
        colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);           // 浅灰色按钮
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // 悬停时稍深
        colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);     // 活动时更深
        colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);       // 非常浅的灰色框架背景
        colors[ImGuiCol_CheckMark] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);        // 深灰色复选标记
        colors[ImGuiCol_SliderGrab] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);       // 灰色滑块抓取
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // 活动时较深的滑块
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);   // 悬停时浅灰色框架
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 活动时稍深
        colors[ImGuiCol_Header] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);           // 浅灰色标头
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // 悬停时稍深
        colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);     // 活动时更深
        colors[ImGuiCol_Separator] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);        // 灰色分隔符
        colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);          // 白色弹出背景

        // 调整填充和间距以获得更整洁的外观
        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(6.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);

        // 圆角以获得更现代的外观
        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;

        // 窗口标题对齐
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Centered

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

    // Initialize OpenGL
    GLFWwindow *initOpenGL()
    {
        // Initialize GLFW
        if (!glfwInit())
        {
            std::cerr << "GLFW initialization failed\n";
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        // 获取主显示器的分辨率
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

        // 根据屏幕分辨率计算窗口大小（屏幕大小的80%）
        int windowWidth = static_cast<int>(mode->width * 0.8f);
        int windowHeight = static_cast<int>(mode->height * 0.8f);

        // 创建具有计算大小的窗口
        GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Marching Cubes Visualization", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "GLFW Window failed to create\n";
            glfwTerminate();
            return nullptr;
        }

        // 在屏幕上居中窗口
        int screenWidth = mode->width;
        int screenHeight = mode->height;
        glfwSetWindowPos(window, (screenWidth - windowWidth) / 2, (screenHeight - windowHeight) / 2);

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetKeyCallback(window, key_callback);

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "GLAD initialization failed\n";
            return nullptr;
        }

        // 启用深度测试和背面剔除
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        return window;
    }

    // 修改runRenderingLoop函数中的postProcessor初始化部分
    void runRenderingLoop(GLFWwindow *window, const VolumeData &volumeData, Mesh &mesh)
    {
        // 设置初始状态
        float isoLevel = (volumeData.maxValue + volumeData.minValue) / 2.0f; // 从中间值开始
        float tempIsoLevel = isoLevel;

        // 相机初始化
        camera.distance = 100.0f;
        camera.zoom = 1.0f;
        camera.rotationX = 0.0f;
        camera.rotationY = 0.0f;

        // 设置着色器
        unsigned int shaderProgram = createShaderProgram();

        // 设置网格
        unsigned int VAO, VBO, EBO;
        setupMesh(mesh, VAO, VBO, EBO);

        // 设置轴可视化
        unsigned int axisVAO = 0;
        // 初始化axisVAO和相关缓冲区

        // 获取窗口大小来初始化后处理器
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // 设置后处理，传入窗口宽度和高度
        PostProcess postProcessor;
        postProcessor.init(windowWidth, windowHeight);

        // 当前爆炸策略
        std::string currentExplosionStrategy = "";

        // 重新计算标志（用于ImGui）
        bool recalculateNeeded = false;
        ImGuiIO &io = ImGui::GetIO();
        io.UserData = &recalculateNeeded;

        // 主渲染循环
        while (!glfwWindowShouldClose(window))
        {
            // 轮询事件
            glfwPollEvents();

            // 检查窗口大小是否改变，如果改变则重新初始化后处理器
            int currentWidth, currentHeight;
            glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
            if (currentWidth != windowWidth || currentHeight != windowHeight)
            {
                windowWidth = currentWidth;
                windowHeight = currentHeight;
                postProcessor.cleanup();
                postProcessor.init(windowWidth, windowHeight);
            }

            // 检查是否需要基于UI交互进行重新计算
            if (recalculateNeeded)
            {
                // 在这里执行重新计算
                // ...

                // 重置标志
                recalculateNeeded = false;
            }

            // 更新视口
            updateViewport(window);

            // 渲染帧
            renderFrame(window, shaderProgram, VAO, mesh, camera, isoLevel, tempIsoLevel,
                        volumeData, currentExplosionStrategy, postProcessor,
                        axisVAO);

            // 交换缓冲区
            glfwSwapBuffers(window);
        }

        // 清理
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);

        if (axisVAO != 0)
            glDeleteVertexArrays(1, &axisVAO);

        // Cleanup postprocessor
        postProcessor.cleanup();

        // Cleanup ImGui
        cleanupImGui();

        // Terminate GLFW
        glfwTerminate();
    }
} // namespace MC
