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

namespace MC
{

    // 全局变量
    Camera camera;
    bool firstMouse = true;
    float lastX, lastY;
    bool showIntersections = false; // 默认不显示交线

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
    vec3 diffuse = diff * vec3(0.8, 0.5, 0.2);
    vec3 ambient = vec3(0.2, 0.1, 0.05);
    
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
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color for the axis
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

    // 渲染一帧
    void renderFrame(GLFWwindow *window, unsigned int shaderProgram, unsigned int VAO,
                     const Mesh &mesh, Camera &camera, float &isoLevel, float &tempIsoLevel,
                     const VolumeData &volumeData,
                     unsigned int intersectionVAO, int numIntersectionSegments)
    {
        // clean screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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

        // 添加交线显示控制
        ImGui::Separator();
        if (ImGui::Checkbox("Show Cutting Plane Intersections", &showIntersections))
        {
            std::cout << "Intersection display: " << (showIntersections ? "ON" : "OFF") << std::endl;
        }

        // 如果有交线可显示，添加一些额外信息
        if (numIntersectionSegments > 0 && showIntersections)
        {
            ImGui::Text("Showing %d intersection segments", numIntersectionSegments);
        }

        ImGui::End(); // end ImGui panel

        // viewport size
        int width, height;
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

        // render ImGui
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