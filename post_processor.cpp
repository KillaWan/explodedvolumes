#include "post_processor.h"
#include <iostream>
#include <vector>
#include <cmath>

// postProcessVertexShaderSource
const char* postProcessVertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
)glsl";

// postProcessFragmentShaderSource using sobel mimic Pen and Ink
const char* postProcessFragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform vec2 texelSize;   // 1.0 / textureWidth, 1.0 / textureHeight

void main()
{
    float kernelX[9] = float[]( -1,  0,  1,
                                 -2,  0,  2,
                                 -1,  0,  1 );
    float kernelY[9] = float[]( -1, -2, -1,
                                  0,  0,  0,
                                  1,  2,  1 );
    
    vec3 sampleTex[9];
    int index = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            sampleTex[index++] = texture(scene, TexCoords + offset).rgb;
        }
    }
    
    float gray[9];
    for (int i = 0; i < 9; i++) {
        gray[i] = dot(sampleTex[i], vec3(0.299, 0.587, 0.114));
    }
    
    float gx = 0.0;
    float gy = 0.0;
    for (int i = 0; i < 9; i++) {
        gx += kernelX[i] * gray[i];
        gy += kernelY[i] * gray[i];
    }
    
    float edge = length(vec2(gx, gy));
    
    float edgeThreshold = 0.4;
    vec3 penColor = edge > edgeThreshold ? vec3(0.0) : vec3(1.0);
    
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    float mixFactor = 0.3;
    vec3 finalColor = mix(sceneColor, penColor, mixFactor);
    
    FragColor = vec4(finalColor, 1.0);
}
)glsl";

PostProcess::PostProcess()
    : fbo(0), sceneTexture(0), rbo(0),
      postProcessShader(0), quadVAO(0), quadVBO(0)
{
}

PostProcess::~PostProcess()
{
    cleanup();
}

GLuint PostProcess::compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint PostProcess::createPostProcessShader()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, postProcessVertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, postProcessFragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "PostProcess shader linking failed: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

void PostProcess::initQuad()
{
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Attribute 0: positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Attribute 1: texCoords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

bool PostProcess::init(int screenWidth, int screenHeight)
{
    // FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Creating Color Attachment Textures
    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);
    
    // Creating Buffer
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer not complete!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    initQuad();
    
    // Create PostProcess Shader
    postProcessShader = createPostProcessShader();
    
    return true;
}

void PostProcess::render(int screenWidth, int screenHeight)
{
    // Cleanup screen
    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
    if(screenWidth<=0||screenHeight<=0)
    {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(postProcessShader);
    
    // Bind texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(glGetUniformLocation(postProcessShader, "scene"), 0);
    
    // Pass Textural Info
    glUniform2f(glGetUniformLocation(postProcessShader, "texelSize"), 1.0f / screenWidth, 1.0f / screenHeight);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
}

void PostProcess::resize(int screenWidth, int screenHeight)
{
    if(screenWidth<=0||screenHeight<=0)
    {
        return;
    }
    // Update texture and size
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void PostProcess::cleanup()
{
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (postProcessShader) {
        glDeleteProgram(postProcessShader);
        postProcessShader = 0;
    }
    if (sceneTexture) {
        glDeleteTextures(1, &sceneTexture);
        sceneTexture = 0;
    }
    if (rbo) {
        glDeleteRenderbuffers(1, &rbo);
        rbo = 0;
    }
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}
