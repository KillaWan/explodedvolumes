#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include <glad/glad.h>
#include <string>

class PostProcess {
public:
    PostProcess();
    ~PostProcess();

    // 初始化FBO、纹理、RBO以及后处理着色器和全屏quad
    bool init(int screenWidth, int screenHeight);

    // 渲染后处理效果，将FBO中内容经过后处理shader绘制到屏幕上
    void render(int screenWidth, int screenHeight);

    // 当窗口大小变化时调用
    void resize(int screenWidth, int screenHeight);

    // 清理资源
    void cleanup();

    GLuint getFBO() const { return fbo; }

private:
    // 帧缓冲对象、纹理和渲染缓冲对象
    GLuint fbo;
    GLuint sceneTexture;
    GLuint rbo;

    // 后处理着色器程序ID
    GLuint postProcessShader;

    // 全屏四边形VAO和VBO
    GLuint quadVAO, quadVBO;

    // 内部函数：编译着色器
    GLuint compileShader(GLenum type, const char* src);

    // 内部函数：创建后处理着色器程序
    GLuint createPostProcessShader();

    // 内部函数：初始化全屏四边形数据
    void initQuad();
};

#endif // POSTPROCESS_H
