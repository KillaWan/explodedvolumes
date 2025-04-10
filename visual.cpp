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
        colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

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

        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
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
            camera.distance *= 0.9f; //Scroll in
        }
        if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            camera.distance *= 1.1f; //Scroll out
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
        style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;
        float panelWidth = 300.0f;
        float spacing = 10.0f;
        float panelY = spacing + ImGui::GetFrameHeight() + spacing + 150.0f;
    
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
                if (!config.useCustomExplosionAxis)
                {
                    *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true; 
                }
    
                MC::applyExplosionAxisConfig(config);
            }
    
            if (config.useCustomExplosionAxis)
            {
                // custom X, Y, Z axis control
                bool axisChanged = false;
                axisChanged |= ImGui::DragFloat("Axis X", &config.customExplosionAxis.x, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Y", &config.customExplosionAxis.y, 0.01f, -1.0f, 1.0f);
                axisChanged |= ImGui::DragFloat("Axis Z", &config.customExplosionAxis.z, 0.01f, -1.0f, 1.0f);
    
                if (axisChanged)
                {
                    MC::applyExplosionAxisConfig(config);
                }
    
                // calculate the axis vector length
                float length = std::sqrt(
                    config.customExplosionAxis.x * config.customExplosionAxis.x +
                    config.customExplosionAxis.y * config.customExplosionAxis.y +
                    config.customExplosionAxis.z * config.customExplosionAxis.z);
    
                if (length > 0.0001f)
                {
                    ImGui::Text("Normalized Axis: (%.2f, %.2f, %.2f)",
                                config.customExplosionAxis.x / length,
                                config.customExplosionAxis.y / length,
                                config.customExplosionAxis.z / length);
                }
    
                if (ImGui::Button("Apply Custom Axis", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)))
                {
                    if (length > 0.0001f)
                    {
                        Vec3 normalizedAxis = {
                            config.customExplosionAxis.x / length,
                            config.customExplosionAxis.y / length,
                            config.customExplosionAxis.z / length};
    
                        config.customExplosionAxis = normalizedAxis;
    
                        MC::applyExplosionAxisConfig(config);
    
                        *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true;
    
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
    
            bool strategyChanged = false;
            if (ImGui::Combo("##Explosion Axis Strategy", &currentIndex, strategyNames.data(), static_cast<int>(strategyNames.size())))
            {
                currentStrategy = strategies[currentIndex];
                strategyChanged = (previousStrategy != currentStrategy);
            }
    
            if (strategyChanged)
            {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
                                  "Strategy change will be applied when you click 'Recalculate'");
            }
    
            ImGui::Separator();
    
            static bool errorShown = false;
            bool shouldShowError = false;
            std::string errorMessage;
    
            if (previousStrategy == currentStrategy && !config.lastDetectionSuccessful)
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
    
            if (currentStrategy == "Rotational Symmetry" && previousStrategy == currentStrategy)
            {
                ImGui::Text("Rotational Symmetry Parameters:");
    
                if (!config.rotationalDetectionSuccessful && config.lastDetectionSuccessful)
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
                                      "No rotational symmetry detected in last analysis.");
    
                    shouldShowError = true;
                    errorMessage = "No rotational symmetry detected in last analysis.\n\n"
                                  "Try adjusting the sample count or symmetry order,\n"
                                  "or switch to a different strategy.";
                }
            }
            
            ImGui::Text("Explosion Axis Visibility");
            ImGui::Checkbox("Show Explosion Axis", &showExplosionAxis);
    
            if (shouldShowError && !errorShown)
            {
                MC::showError("Symmetry Detection Failed", errorMessage);
                errorShown = true;
            }
            else if (!shouldShowError)
            {
                errorShown = false;
            }
    
            ImGui::Separator();
    
            if (!config.useCustomExplosionAxis)
            {
                if (ImGui::Button("Recalculate Explosion Axis", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)))
                {
                    if (previousStrategy != currentStrategy)
                    {
                        std::string newInternalStrategy = MC::ExplosionAxisConfig::convertToInternalName(currentStrategy);
                        
                        config.rotationalDetectionSuccessful = true;
                        config.reflectiveDetectionSuccessful = true;
                        config.lastDetectionSuccessful = true;
                        
                        config.strategyName = newInternalStrategy;
                        MC::setExplosionStrategy(newInternalStrategy);
                        MC::applyExplosionAxisConfig(config);
                        
                        previousStrategy = currentStrategy;
                    }
    
                    *reinterpret_cast<bool *>(ImGui::GetIO().UserData) = true;
                    errorShown = false;
                }
            }
        }
        ImGui::End();
        MC::renderErrorPopup();
    }

    void showError(const std::string &title, const std::string &message)
    {
        errorPopupTitle = title;
        errorPopupMessage = message;
        showErrorPopup = true;
    }

    // render error popup
    void renderErrorPopup()
    {
        if (showErrorPopup)
        {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

            if (ImGui::BeginPopupModal(errorPopupTitle.c_str(), &showErrorPopup,
                                       ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 黑色文字
                ImGui::TextWrapped("%s", errorPopupMessage.c_str());
                ImGui::PopStyleColor();
                ImGui::Separator();

                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);

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

                ImGui::PopStyleColor(3);

                ImGui::EndPopup();
            }

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

        glBindFramebuffer(GL_FRAMEBUFFER, postProcessor.getFBO());
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        setupImGuiStyle();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;

        float panelWidth = 300.0f;
        float panelSpacing = 10.0f;
        float panel1Height = viewportSize.y * 0.2f;
        float panel2Height = viewportSize.y * 0.5f;
        float panel3Height = viewportSize.y * 0.2f;

        // panel 1
        if (panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel1Height));
        }
        ImGui::Begin("Marching Cubes Control", nullptr);

        ImGui::Text("ISO level: %.1f to %.1f", volumeData.minValue, volumeData.maxValue);

        if (ImGui::SliderFloat("##ISO Level (%)", &g_tempIsoLevelPercent, 0.0f, 100.0f, "%.1f%%"))
        {
            tempIsoLevel = volumeData.minValue + (g_tempIsoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
        }

        if (ImGui::Button("Apply ISO"))
        {
            g_isoLevelPercent = g_tempIsoLevelPercent;
            isoLevel = volumeData.minValue + (g_isoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue); 
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f (%.1f%%)", isoLevel, g_isoLevelPercent);
        ImGui::End();

        // panel 2
        if (panelFirstTime)
        {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing * 2 + panel1Height));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel2Height));
        }
        renderExplosionAxisGUI(currentExplosionStrategy);

        // panel 3
        if (panelFirstTime) {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, viewportSize.y - panel3Height - panelSpacing));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel3Height));
        }
        ImGui::Begin("Explosion View Control", nullptr);

        if (ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections))
        {
            std::cout << "Model explosion: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }
        const float MAX_EXPLOSION_DISTANCE = 40.0f;

        static bool firstRun = true;
        if (firstRun)
        {
            tempExplosionPercent = g_explosionDistancePercent;
            firstRun = false;
        }
        ImGui::Text("Explosion Distance");
        if (showIntersections)
        {
            if (ImGui::SliderFloat("##Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%"))
            {
            }

            if (ImGui::Button("Apply Distance"))
            {
                g_explosionDistancePercent = tempExplosionPercent;
                g_explosionDistance = (g_explosionDistancePercent / 100.0f) * MAX_EXPLOSION_DISTANCE;
            }

            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::SliderFloat("##Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%");
            if (ImGui::Button("Apply Distance"))
            {
            }
            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
            ImGui::EndDisabled();
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height != 0) ? (float)width / height : 1.0f;

        glUseProgram(shaderProgram);

        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / mesh.max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // center
        glm::vec3 glmCenter(mesh.center.x, mesh.center.y, mesh.center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // transform uniform
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        unsigned int axisShader = createLineShaderProgram();
        glUseProgram(axisShader);

        // draw the axis only when showExplosionAxis is true
        if (showExplosionAxis)
        {
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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // post processor
        postProcessor.render(width, height);

        // render ImGui
        ImGui::Render();

        // draw the axis only when showExplosionAxis is true
        if (showExplosionAxis)
        {
            unsigned int axisShader = createLineShaderProgram();
            glUseProgram(axisShader);

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
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        setupImGuiStyle();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;

        float panelWidth = 300.0f;
        float panelSpacing = 10.0f;
        float panel1Height = viewportSize.y * 0.2f;
        float panel2Height = viewportSize.y * 0.5f;
        float panel3Height = viewportSize.y * 0.2f;

        // panel 1
        if (panelFirstTime)
        {
            ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, panel1Height));
        }
        ImGui::Begin("Marching Cubes Control", nullptr);

        ImGui::Text("ISO level: %.1f to %.1f", volumeData.minValue, volumeData.maxValue);

        if (ImGui::SliderFloat("##ISO Level (%)", &g_tempIsoLevelPercent, 0.0f, 100.0f, "%.1f%%"))
        {
            tempIsoLevel = volumeData.minValue + (g_tempIsoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
        }

        if (ImGui::Button("Apply ISO"))
        {
            g_isoLevelPercent = g_tempIsoLevelPercent;
            isoLevel = volumeData.minValue + (g_isoLevelPercent / 100.0f) * (volumeData.maxValue - volumeData.minValue);
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f (%.1f%%)", isoLevel, g_isoLevelPercent);
        ImGui::End();

        // panel 2
        ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, panelSpacing * 2 + panel1Height));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panel2Height));

        std::string currentStrategy = "";
        renderExplosionAxisGUI(currentStrategy);

        // panel 3
        ImGui::SetNextWindowPos(ImVec2(viewportSize.x - panelWidth - panelSpacing, viewportSize.y - panel3Height - panelSpacing));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panel3Height));
        ImGui::Begin("Explosion View Control", nullptr);

        if (ImGui::Checkbox("Explode Model at Cutting Planes", &showIntersections))
        {
            std::cout << "Model explosion: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }
        const float MAX_EXPLOSION_DISTANCE = 40.0f;

        ImGui::Text("Explosion Distance");
        if (showIntersections)
        {
            float currentDistancePercent = (explodedView.explosionDistance / MAX_EXPLOSION_DISTANCE) * 100.0f;
            static bool firstRun = true;
            if (firstRun)
            {
                tempExplosionPercent = g_explosionDistancePercent;
                firstRun = false;
            }

            if (ImGui::SliderFloat("##Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%"))
            {
            }

            if (ImGui::Button("Apply Distance"))
            {
                g_explosionDistancePercent = tempExplosionPercent;
                g_explosionDistance = (tempExplosionPercent / 100.0f) * MAX_EXPLOSION_DISTANCE;
            }

            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", currentDistancePercent);
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::SliderFloat("##Explosion Distance", &tempExplosionPercent, 0.0f, 100.0f, "%.1f%%");
            if (ImGui::Button("Apply Distance"))
            {
            }
            ImGui::SameLine();
            ImGui::Text("Current: %.1f%%", g_explosionDistancePercent);
            ImGui::EndDisabled();
        }

        ImGui::End();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height != 0) ? (float)width / height : 1.0f;

        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / mesh.max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // center
        glm::vec3 glmCenter(mesh.center.x, mesh.center.y, mesh.center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // render each fragment
        if (explodedView.enabled)
        {
            for (const auto &segment : explodedView.segments)
            {
                if (segment.indices.empty() || segment.VAO == 0)
                {
                    continue;
                }

                glUseProgram(shaderProgram);

                glm::mat4 segmentModel = model;
                segmentModel = glm::translate(segmentModel, glm::vec3(
                                                                segment.displacement.x,
                                                                segment.displacement.y,
                                                                segment.displacement.z));

                int modelLoc = glGetUniformLocation(shaderProgram, "model");
                int viewLoc = glGetUniformLocation(shaderProgram, "view");
                int projLoc = glGetUniformLocation(shaderProgram, "projection");

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(segmentModel));
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

                glBindVertexArray(segment.VAO);
                glDrawElements(GL_TRIANGLES, segment.indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        // render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (showExplosionAxis)
        {
            unsigned int axisShader = createLineShaderProgram();
            glUseProgram(axisShader);

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

    // set up ImGui
    void setupImGui(GLFWwindow *window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;

        ImGui::StyleColorsLight();
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

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

        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    // clean ImGui
    void cleanupImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // initialize OpenGL
    GLFWwindow *initOpenGL()
    {
        if (!glfwInit())
        {
            std::cerr << "GLFW initialization failed\n";
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

        int windowWidth = static_cast<int>(mode->width * 0.8f);
        int windowHeight = static_cast<int>(mode->height * 0.8f);

        GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Marching Cubes Visualization", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return nullptr;
        }

        int screenWidth = mode->width;
        int screenHeight = mode->height;
        glfwSetWindowPos(window, (screenWidth - windowWidth) / 2, (screenHeight - windowHeight) / 2);

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetKeyCallback(window, key_callback);

        // initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "GLAD initialization failed\n";
            return nullptr;
        }
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        return window;
    }

    // modify the postProcessor initialization part in the runRenderingLoop function
    void runRenderingLoop(GLFWwindow *window, const VolumeData &volumeData, Mesh &mesh)
    {
        float isoLevel = (volumeData.maxValue + volumeData.minValue) / 2.0f; // 从中间值开始
        float tempIsoLevel = isoLevel;

        camera.distance = 100.0f;
        camera.zoom = 1.0f;
        camera.rotationX = 0.0f;
        camera.rotationY = 0.0f;

        unsigned int shaderProgram = createShaderProgram();

        unsigned int VAO, VBO, EBO;
        setupMesh(mesh, VAO, VBO, EBO);

        unsigned int axisVAO = 0;
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        PostProcess postProcessor;
        postProcessor.init(windowWidth, windowHeight);

        std::string currentExplosionStrategy = "";

        bool recalculateNeeded = false;
        ImGuiIO &io = ImGui::GetIO();
        io.UserData = &recalculateNeeded;

        // main render loop
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            int currentWidth, currentHeight;
            glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
            if (currentWidth != windowWidth || currentHeight != windowHeight)
            {
                windowWidth = currentWidth;
                windowHeight = currentHeight;
                postProcessor.cleanup();
                postProcessor.init(windowWidth, windowHeight);
            }

            if (recalculateNeeded)
            {
                // Reset Flag
                recalculateNeeded = false;
            }

            updateViewport(window);

            renderFrame(window, shaderProgram, VAO, mesh, camera, isoLevel, tempIsoLevel,
                        volumeData, currentExplosionStrategy, postProcessor,
                        axisVAO);

            glfwSwapBuffers(window);
        }

        // clean
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);

        if (axisVAO != 0)
            glDeleteVertexArrays(1, &axisVAO);

        postProcessor.cleanup();

        cleanupImGui();

        glfwTerminate();
    }
} // namespace MC
