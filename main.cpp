#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // For example, with GLUT:
    // glutInit(nullptr, nullptr);
    // glutCreateWindow("OpenGL Version");

    // With GLFW (recommended for modern usage):
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL Version", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;

    // Clean up
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
