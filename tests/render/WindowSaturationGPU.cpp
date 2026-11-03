#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <render/shaders/Shaders.hpp>

class WindowSaturationGPU : public ::testing::Test {
  protected:
    void SetUp() override {
        previousDisplay = eglGetCurrentDisplay();
        previousContext = eglGetCurrentContext();
        previousDraw    = eglGetCurrentSurface(EGL_DRAW);
        previousRead    = eglGetCurrentSurface(EGL_READ);
        if (previousContext != EGL_NO_CONTEXT)
            GTEST_SKIP() << "GPU test requires no existing current context";
        display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr))
            GTEST_SKIP() << "No surfaceless EGL display";
        initialized = true;
        ASSERT_TRUE(eglBindAPI(EGL_OPENGL_ES_API));
        const EGLint attrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
        EGLConfig    config  = nullptr;
        EGLint       count   = 0;
        ASSERT_TRUE(eglChooseConfig(display, attrs, &config, 1, &count));
        ASSERT_GT(count, 0);
        const EGLint contextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context                     = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
        ASSERT_NE(context, EGL_NO_CONTEXT);
        ASSERT_TRUE(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context));

        // Compile the actual production saturation block, not a CPU reimplementation.
        std::string source;
        for (const auto& [name, text] : SHADERS) {
            if (name == "surface.frag")
                source = text;
        }
        ASSERT_FALSE(source.empty());
        const auto begin = source.find("#if USE_SATURATION\n    //");
        ASSERT_NE(begin, std::string::npos);
        const auto end = source.find("#endif", begin);
        ASSERT_NE(end, std::string::npos);
        const auto        body     = source.substr(source.find('\n', begin) + 1, end - source.find('\n', begin) - 1);
        const std::string fragment = "#version 300 es\nprecision highp float;\nuniform vec4 inputColor;\nuniform float windowSaturation;\nuniform vec3 "
                                     "windowSaturationWeights;\nout vec4 fragColor;\nvoid main(){vec4 pixColor=inputColor;\n" +
            body + "\nfragColor=pixColor;}";
        const std::string vertex = "#version 300 es\nvoid main(){vec2 p[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));gl_Position=vec4(p[gl_VertexID],0,1);}";
        program                  = glCreateProgram();
        for (const auto& [kind, text] : std::array<std::pair<GLenum, std::string>, 2>{{{GL_VERTEX_SHADER, vertex}, {GL_FRAGMENT_SHADER, fragment}}}) {
            const auto  shader = glCreateShader(kind);
            const char* ptr    = text.c_str();
            glShaderSource(shader, 1, &ptr, nullptr);
            glCompileShader(shader);
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            std::array<char, 4096> log{};
            glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
            EXPECT_TRUE(compiled) << log.data();
            glAttachShader(program, shader);
            glDeleteShader(shader);
        }
        glLinkProgram(program);
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        ASSERT_TRUE(linked);
        glUseProgram(program);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, nullptr);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            GTEST_SKIP() << "No float framebuffer support";
        glViewport(0, 0, 1, 1);
        glDisable(GL_BLEND);
        glUniform3f(glGetUniformLocation(program, "windowSaturationWeights"), 0.2627F, 0.6780F, 0.0593F);
        ASSERT_EQ(glGetError(), GL_NO_ERROR);
    }

    void TearDown() override {
        if (context != EGL_NO_CONTEXT) {
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &texture);
            glDeleteProgram(program);
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroyContext(display, context);
        }
        if (initialized)
            eglTerminate(display);
        if (previousDisplay != EGL_NO_DISPLAY)
            eglMakeCurrent(previousDisplay, previousDraw, previousRead, previousContext);
    }

    std::array<float, 4> sample(const std::array<float, 4>& input, float amount) {
        glUniform4fv(glGetUniformLocation(program, "inputColor"), 1, input.data());
        glUniform1f(glGetUniformLocation(program, "windowSaturation"), amount);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::array<float, 4> output{};
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, output.data());
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
        return output;
    }

    EGLDisplay display = EGL_NO_DISPLAY, previousDisplay = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT, previousContext = EGL_NO_CONTEXT;
    EGLSurface previousDraw = EGL_NO_SURFACE, previousRead = EGL_NO_SURFACE;
    bool       initialized = false;
    GLuint     program = 0, texture = 0, framebuffer = 0;
};

TEST_F(WindowSaturationGPU, PreservesNeutralAndAlphaIncludingHDR) {
    for (const auto& input : std::array<std::array<float, 4>, 5>{{{0, 0, 0, 0}, {.2F, .2F, .2F, .5F}, {1, 1, 1, 1}, {203, 203, 203, 1}, {1000, 1000, 1000, 1}}}) {
        const auto output = sample(input, 1.08F);
        for (size_t i = 0; i < 4; ++i)
            EXPECT_NEAR(output[i], input[i], 0.0002F);
    }
}

TEST_F(WindowSaturationGPU, IncreasesChromaWithoutChangingAlpha) {
    const std::array<float, 4> input{.6F, .4F, .2F, .75F};
    const auto                 output = sample(input, 1.08F);
    EXPECT_GT(output[0], input[0]);
    EXPECT_LT(output[2], input[2]);
    EXPECT_FLOAT_EQ(output[3], input[3]);
    const auto neutral = sample(input, 1.F);
    for (size_t i = 0; i < 4; ++i)
        EXPECT_NEAR(neutral[i], input[i], 0.00001F);
    const auto gray = sample(input, 0.F);
    EXPECT_NEAR(gray[0], gray[1], 0.00001F);
    EXPECT_NEAR(gray[1], gray[2], 0.00001F);
}

TEST_F(WindowSaturationGPU, BoundsExtremesAndPreservesHDRHeadroom) {
    for (const auto& input : std::array<std::array<float, 4>, 3>{{{1, 0, 0, 1}, {.5F, 0, .5F, .5F}, {203, 100, 10, 1}}}) {
        const auto output = sample(input, 2.F);
        for (size_t i = 0; i < 3; ++i) {
            EXPECT_TRUE(std::isfinite(output[i]));
            EXPECT_GE(output[i], 0.F);
            EXPECT_LE(output[i], std::max(input[3], input[0]));
        }
        EXPECT_FLOAT_EQ(output[3], input[3]);
    }
}
