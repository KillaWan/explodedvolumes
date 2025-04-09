#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include <glad/glad.h>
#include <string>

class PostProcess {
    public:
        PostProcess();
        ~PostProcess();

        bool init(int screenWidth, int screenHeight);
        void render(int screenWidth, int screenHeight);
        void resize(int screenWidth, int screenHeight);
        void cleanup();
        GLuint getFBO() const { return fbo; }

    private:
        GLuint fbo;
        GLuint sceneTexture;
        GLuint rbo;
        GLuint postProcessShader;
        GLuint quadVAO, quadVBO;
        GLuint compileShader(GLenum type, const char* src);
        GLuint createPostProcessShader();
        void initQuad();
    };

#endif
