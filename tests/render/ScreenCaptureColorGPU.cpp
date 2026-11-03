#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <render/shaders/Shaders.hpp>

class ScreenCaptureColorGPU : public ::testing::Test {
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

        // Use the production transfer functions and mirror conversion on a real GPU.
        std::string constants, helpers;
        for (const auto& [name, text] : SHADERS) {
            if (name == "constants.h")
                constants = text;
            if (name == "cm_helpers.glsl")
                helpers = text;
        }
        ASSERT_FALSE(helpers.empty());
        for (const auto& include : {std::string("#include \"defines.h\""), std::string("#include \"constants.h\"")}) {
            const auto pos = helpers.find(include);
            ASSERT_NE(pos, std::string::npos);
            helpers.erase(pos, include.size());
        }
        const std::string fragment = "#version 300 es\nprecision highp float;\nprecision highp int;\n#define ALLOW_INCLUDES\n#define USE_MIRROR 1\n#define USE_ICC 0\n#define "
                                     "USE_TONEMAP 0\n#define USE_SDR_MOD 0\n" +
            constants + helpers +
            "\nuniform vec4 inputColor; uniform float referenceWhite; uniform int sourceTF; uniform int outputIndex; out vec4 fragColor;\n"
            "void main(){vec2 range=sourceTF==11 ? vec2(0,10000) : vec2(0.2,251);"
            "vec4 encoded=inputColor; if(sourceTF==11) encoded.rgb=tfPQ(tfInvSRGB(inputColor.rgb/max(inputColor.a,0.001))*referenceWhite/10000.0)*inputColor.a;"
            "vec4 result[2]=doColorManagement(encoded,1.0,sourceTF,0,mat3(1),range,vec2(0,10000),0.0,referenceWhite);fragColor=result[outputIndex];}";
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

    std::array<float, 4> sample(const std::array<float, 4>& input, float white, int tf = 11, int outputIndex = 1) {
        glUniform4fv(glGetUniformLocation(program, "inputColor"), 1, input.data());
        glUniform1f(glGetUniformLocation(program, "referenceWhite"), white);
        glUniform1i(glGetUniformLocation(program, "sourceTF"), tf);
        glUniform1i(glGetUniformLocation(program, "outputIndex"), outputIndex);
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

TEST_F(ScreenCaptureColorGPU, PQDesktopMatchesSDRCaptureAtDifferentReferenceWhites) {
    for (const float white : {80.F, 203.F, 301.F}) {
        for (const auto& input : std::array<std::array<float, 4>, 4>{{{0, 0, 0, 1}, {.5F, .5F, .5F, 1}, {.8F, .3F, .1F, 1}, {1, 1, 1, 1}}}) {
            const auto output = sample(input, white);
            for (size_t i = 0; i < 4; ++i)
                EXPECT_NEAR(output[i], input[i], .001F) << "white=" << white << " channel=" << i;
        }
    }
}

TEST_F(ScreenCaptureColorGPU, PreservesPremultipliedAlpha) {
    for (const auto& input : std::array<std::array<float, 4>, 3>{{{.3F, .15F, .1F, .5F}, {0, 0, 0, 0}, {.2F, .2F, .2F, .25F}}}) {
        const auto output = sample(input, 301.F);
        for (size_t i = 0; i < 4; ++i)
            EXPECT_NEAR(output[i], input[i], .001F);
    }
}

TEST_F(ScreenCaptureColorGPU, SDRCaptureAndHDRDisplayRemainUnchanged) {
    const std::array<float, 4> input{.5F, .5F, .5F, 1};
    const auto                 sdr = sample(input, 301.F, 9);
    for (size_t i = 0; i < 4; ++i)
        EXPECT_NEAR(sdr[i], input[i], .0001F);
    const auto  hdr          = sample(input, 301.F, 11, 0);
    const float expectedNits = std::pow((.5F + .055F) / 1.055F, 2.4F) * 301.F;
    for (size_t i = 0; i < 3; ++i)
        EXPECT_NEAR(hdr[i], expectedNits, .02F);
    EXPECT_FLOAT_EQ(hdr[3], 1.F);
}
