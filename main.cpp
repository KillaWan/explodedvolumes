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

// NIfTI
#include "nifti1_io.h"
#include "file_dialog.h"

// GLM for MVP matrix
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "marching_cubes_v3.h"
#include "symmetry_axis.h"  





// Camera parameters
struct Camera
{
    float rotationX = 15.0f;  // Initial tilt down
    float rotationY = -45.0f; // Initial rotation for better view
    float distance = 5.0f;
    float zoom = 5.0f; // Much higher initial zoom for larger appearance
};

// Global camera state
Camera camera;
bool firstMouse = true;
float lastX, lastY;

// Mouse callback function
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    // Ask ImGui if it wants the mouse
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        // The user is interacting with an ImGui widget, so don't rotate the camera
        return;
    }

    // Otherwise, proceed with your camera rotation logic
    static bool firstMouse = true;
    static float lastX, lastY;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    // Only rotate if left mouse button is pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed

        const float sensitivity = 0.1f;
        camera.rotationY += xoffset * sensitivity;
        camera.rotationX += yoffset * sensitivity;

        // Limit pitch
        if (camera.rotationX > 89.0f)
            camera.rotationX = 89.0f;
        if (camera.rotationX < -89.0f)
            camera.rotationX = -89.0f;
    }

    lastX = xpos;
    lastY = ypos;
}

// Scroll callback for zooming - reversed direction so scrolling up zooms in
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    // Positive yoffset (scroll up) increases zoom (zooms in)
    // Negative yoffset (scroll down) decreases zoom (zooms out)
    std::cout << "Scroll event: yoffset = " << yoffset << std::endl;
    camera.zoom -= yoffset * 0.2f; // Increased sensitivity
    if (camera.zoom < 0.1f)
        camera.zoom = 0.1f;
    if (camera.zoom > 6.0f)
        camera.zoom = 6.0f; // Allow higher maximum zoom
}

// -------------------- NIfTI File Loading --------------------
bool loadNiiFile(const char *filename, float *&data, int dims[3],
                 float &outMinVal, float &outMaxVal)
{
    nifti_image *nim = nifti_image_read(filename, 1);
    if (!nim)
    {
        std::cerr << "Failed to read NIfTI file: " << filename << std::endl;
        return false;
    }

    dims[0] = nim->nx;
    dims[1] = nim->ny;
    dims[2] = nim->nz;

    // We'll track the min and max as we read the data
    float min_val = std::numeric_limits<float>::max();
    float max_val = std::numeric_limits<float>::lowest();

    if (nim->datatype == NIFTI_TYPE_FLOAT32)
    {
        data = new float[nim->nvox];
        float *src = static_cast<float *>(nim->data);
        for (size_t i = 0; i < nim->nvox; i++)
        {
            data[i] = src[i];
            min_val = std::min(min_val, data[i]);
            max_val = std::max(max_val, data[i]);
        }
    }
    else if (nim->datatype == NIFTI_TYPE_UINT8)
    {
        auto *src = static_cast<uint8_t *>(nim->data);
        data = new float[nim->nvox];
        for (size_t i = 0; i < nim->nvox; i++)
        {
            data[i] = static_cast<float>(src[i]);
            min_val = std::min(min_val, data[i]);
            max_val = std::max(max_val, data[i]);
        }
    }
    else
    {
        std::cerr << "Unsupported data type: " << nim->datatype << std::endl;
        nifti_image_free(nim);
        return false;
    }

    // Output the final min/max to the caller
    outMinVal = min_val;
    outMaxVal = max_val;

    std::cout << "Data range: " << min_val << " to " << max_val << std::endl;

    nifti_image_free(nim);
    return true;
}


// -------------------- Shader Source Code --------------------
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

const char *lineVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main(){
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )glsl";
    
    const char *lineFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    void main(){
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
    }
    )glsl";
    

// Window size change callback
static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Key callback for additional controls
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Adjust camera distance with + and - keys
    if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        camera.distance *= 0.9f; // Zoom in
    }
    if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        camera.distance *= 1.1f; // Zoom out
    }
}

// Shader helper function
unsigned int compileShader(GLenum type, const char *src)
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

// Create shader program
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

unsigned int createLineShaderProgram() {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, lineVertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, lineFragmentShaderSource);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Line shader program linking failed:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

int main()
{
    // 1) Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW init failed\n";
        return -1;
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
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // 2) Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    // 3) Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 4) Load volume data
    float *volumeData = nullptr;
    int dims[3] = {0, 0, 0};
    float min_val = 0.0f;
    float max_val = 0.0f;

    std::string filePath = FileDialog::openFile("Open File", std::filesystem::current_path().string(), {{"NIfTI Files", "*.nii"}})[0];
    const char* niiFilename = filePath.c_str();
    std::cout << "Loading NIfTI file: " << niiFilename << std::endl;

    if (!loadNiiFile(niiFilename, volumeData, dims, min_val, max_val))
    {
        std::cerr << "Failed to load file\n";
        return -1;
    }

    // 5) Initialize Marching Cubes
    std::vector<Vertex> vertices;
    std::vector<IndexType> indices;

    // We'll keep a "live" isoLevel and a "temp" for the slider
    // Start in the middle of data range
    float isoLevel = 10.0f;
    float tempIsoLevel = isoLevel;

    std::cout << "Initial isoLevel = " << isoLevel << std::endl;
    marchingCubesFull(volumeData, dims, isoLevel, vertices, indices);

    // 6) Compute mesh bounds
    Vec3 min_bounds, max_bounds, center;
    calculateMeshBounds(vertices, min_bounds, max_bounds, center);
    float max_dimension = std::max({max_bounds.x - min_bounds.x,
                                    max_bounds.y - min_bounds.y,
                                    max_bounds.z - min_bounds.z});
    camera.distance = max_dimension * 0.05f;

    //exposion axis computing
    Vec3 explosionAxis = computeExplosionAxis(vertices);
    std::cout << "Computed explosion axis: (" 
          << explosionAxis.x << ", " << explosionAxis.y << ", " << explosionAxis.z << ")\n";
          
    Vertex axisStart, axisEnd;
          axisStart.x = center.x;
          axisStart.y = center.y;
          axisStart.z = center.z;
          float axisLength = max_dimension * 2.0f; // 根据需要调整线段长度
          axisEnd.x = center.x + explosionAxis.x * axisLength;
          axisEnd.y = center.y + explosionAxis.y * axisLength;
          axisEnd.z = center.z + explosionAxis.z * axisLength;
          
    unsigned int axisVAO, axisVBO;
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    Vertex axisLine[2] = { axisStart, axisEnd };
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisLine), axisLine, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
          
    // 7) Create VAO/VBO/EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IndexType),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // 8) Create shader program
    unsigned int shaderProgram = createShaderProgram();
    unsigned int lineShaderProgram = createLineShaderProgram();


    // 9) Enable depth test, cull
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // ESC check
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- ImGui Panel ---
        ImGui::Begin("Marching Cubes Control");

        // TODO: change the range from 0 to 100
        ImGui::Text("Data range: %.1f to %.1f", min_val, max_val);

        // Slider from min_val to max_val
        if (ImGui::SliderFloat("ISO Level", &tempIsoLevel, min_val, max_val, "%.1f"))
        {
            // (No immediate recalc here if you want to wait for a button)
        }

        // Optional: apply changes only on button press
        if (ImGui::Button("Apply ISO"))
        {
            isoLevel = tempIsoLevel;
            vertices.clear();
            indices.clear();
            marchingCubesFull(volumeData, dims, isoLevel, vertices, indices);

            // Re-upload
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                         vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IndexType),
                         indices.data(), GL_STATIC_DRAW);

            std::cout << "Updated mesh with isoLevel = " << isoLevel << std::endl;
        }
        ImGui::SameLine();
        ImGui::Text("Current ISO: %.1f", isoLevel);

        ImGui::End(); // End ImGui panel

        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Viewport size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        // Use shader
        glUseProgram(shaderProgram);

        // Build transform
        glm::mat4 model(1.0f);
        float scale_factor = 20.0f / max_dimension;
        model = glm::scale(model, glm::vec3(scale_factor, scale_factor, scale_factor));

        // Center model
        glm::vec3 glmCenter(center.x, center.y, center.z);
        model = glm::translate(model, -glmCenter);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -camera.distance * camera.zoom));
        view = glm::rotate(view, glm::radians(camera.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(camera.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

        // Pass uniforms
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 设置较粗线宽
        glLineWidth(5.0f);  
        // 使用专用的线段着色器
        glUseProgram(lineShaderProgram);

        // 如果需要，将模型、视图、投影矩阵传递给线段着色器（通常与 mesh 一致）
        int lineModelLoc = glGetUniformLocation(lineShaderProgram, "model");
        int lineViewLoc  = glGetUniformLocation(lineShaderProgram, "view");
        int lineProjLoc  = glGetUniformLocation(lineShaderProgram, "projection");
        glUniformMatrix4fv(lineModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(lineViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lineProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
        // 绘制爆炸轴线段
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);


        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete[] volumeData;

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}