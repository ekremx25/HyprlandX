#include <render/ShaderLoader.hpp>
#include <config/ConfigManager.hpp>
#include <glslang/Public/resource_limits_c.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <utility>

using namespace Render;

// Keep shader preprocessing independent of a compositor, Lua state, and user config.
class CExternalTextureConfigManager : public Config::IConfigManager {
  public:
    Config::eConfigManagerType type() override { return Config::CONFIG_LUA; }
    void                      init() override { ; }
    void                      reload() override { ; }
    std::string               verify() override { return {}; }
    int                       getDeviceInt(const std::string&, const std::string&, const std::string&) override { return 0; }
    float                     getDeviceFloat(const std::string&, const std::string&, const std::string&) override { return 0.F; }
    Vector2D                  getDeviceVec(const std::string&, const std::string&, const std::string&) override { return {}; }
    std::string               getDeviceString(const std::string&, const std::string&, const std::string&) override { return {}; }
    bool                      deviceConfigExplicitlySet(const std::string&, const std::string&) override { return false; }
    bool                      deviceConfigExists(const std::string&) override { return false; }

    Config::SConfigOptionReply getConfigValue(const std::string& name) override {
        EXPECT_EQ(name, "render:cm_enabled");
        // getVariantSource caches a CConfigValue; its backing must outlive this manager.
        static bool  enabled = true;
        static void* value   = &enabled;
        return {.dataptr = &value, .type = &typeid(bool)};
    }

    std::string                     getMainConfigPath() override { return {}; }
    std::string                     currentConfigPath() override { return {}; }
    std::string                     getConfigString() override { return {}; }
    const std::vector<std::string>& getConfigPaths() override { return m_paths; }
    bool                            configLoaded() override { return true; }
    bool                            configVerifPassed() override { return true; }
    std::string                     getErrors() override { return {}; }

    std::expected<void, std::string> generateDefaultConfig(const std::filesystem::path&, bool) override { return {}; }
    void                             handlePluginLoads() override { ; }
    std::expected<void, std::string> registerPluginValue(void*, SP<Config::Values::IValue>) override { return {}; }
    void                             onPluginUnload(void*) override { ; }
    void                             warnDeprecated(std::string, std::string) override { ; }
    void                             clearDeprecations() override { ; }
    std::vector<std::string>         deprecationNotices() const override { return {}; }

  private:
    std::vector<std::string> m_paths;
};

class ExternalTexture : public ::testing::Test {
  protected:
    void SetUp() override {
        m_glslangInitialized = glslang_initialize_process();
        ASSERT_TRUE(m_glslangInitialized);
        m_previousConfig = std::move(Config::mgr());
        Config::mgr()    = makeUnique<CExternalTextureConfigManager>();
    }

    void TearDown() override {
        if (m_glslangInitialized) {
            Config::mgr() = std::move(m_previousConfig);
            glslang_finalize_process();
        }
    }

  private:
    bool                       m_glslangInitialized = false;
    UP<Config::IConfigManager> m_previousConfig;
};

TEST_F(ExternalTexture, surfaceVariantUsesExternalSamplerAndRetainsColorManagement) {
    // Match OpenGL.cpp's include set. Every fragment slot must contain a valid file.
    const std::vector<std::string> includes = {
        "defines.h",       "constants.h",      "cm_helpers.glsl", "rounding.glsl", "CM.glsl",         "tonemap.glsl",     "gain.glsl",     "border.glsl",      "shadow.glsl",
        "inner_glow.glsl", "blurprepare.glsl", "blur1.glsl",      "blur2.glsl",    "blurFinish.glsl", "motion_blur.glsl", "motion_blur_sample.glsl", "gradient.glsl", "glassFinish.glsl", "fluidJar.glsl",
    };
    std::array<std::string, SH_FRAG_LAST> frags;
    frags.fill("surface.frag");
    const auto    shaderPath = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src/render/shaders/glsl";
    CShaderLoader loader(includes, frags, shaderPath.string());

    SShaderVariant variant{
        .features = SH_FEAT_RGBA | SH_FEAT_CM | SH_FEAT_TONEMAP,
        .sourceTF = NColorManagement::CM_TRANSFER_FUNCTION_ST2084_PQ,
        .targetTF = NColorManagement::CM_TRANSFER_FUNCTION_SRGB,
    };

    for (const bool external : {false, true}) {
        SCOPED_TRACE(external ? "external" : "texture2D");
        // Reserve bit 14 without depending on the not-yet-added production enum.
        if (external)
            variant.features |= (1U << 14);

        for (const ShaderFeatureFlags extra : std::array<ShaderFeatureFlags, 9>{
                 0, SH_FEAT_MOTION_BLUR, SH_FEAT_MOTION_BLUR | SH_FEAT_BLUR | SH_FEAT_BLUR_MATTE, SH_FEAT_ICC, SH_FEAT_MIRROR, SH_FEAT_SATURATION, SH_FEAT_SATURATION | SH_FEAT_ICC,
                 SH_FEAT_SATURATION | SH_FEAT_MIRROR, SH_FEAT_SATURATION | SH_FEAT_MOTION_BLUR | SH_FEAT_BLUR | SH_FEAT_BLUR_MATTE}) {
            SCOPED_TRACE(extra);
            auto compiledVariant = variant;
            compiledVariant.features |= extra;
            if (extra == SH_FEAT_SATURATION)
                compiledVariant.features &= ~(SH_FEAT_CM | SH_FEAT_TONEMAP);
            auto compiledSource = loader.getVariantSource(SH_FRAG_SURFACE, compiledVariant);
            ASSERT_FALSE(compiledSource.empty());
            if (extra & SH_FEAT_SATURATION)
                EXPECT_NE(compiledSource.find("windowSaturation"), std::string::npos);
            const glslang_input_t input = {
                .language = GLSLANG_SOURCE_GLSL,
                .stage = GLSLANG_STAGE_FRAGMENT,
                .client = GLSLANG_CLIENT_NONE,
                .target_language = GLSLANG_TARGET_NONE,
                .code = compiledSource.c_str(),
                .default_version = 300,
                .default_profile = GLSLANG_ES_PROFILE,
                .messages = GLSLANG_MSG_DEFAULT_BIT,
                .resource = glslang_default_resource(),
            };
            auto* shader = glslang_shader_create(&input);
            ASSERT_NE(shader, nullptr);
            const bool preprocessed = glslang_shader_preprocess(shader, &input);
            EXPECT_TRUE(preprocessed) << glslang_shader_get_info_log(shader);
            if (preprocessed)
                EXPECT_TRUE(glslang_shader_parse(shader, &input)) << glslang_shader_get_info_log(shader);
            glslang_shader_delete(shader);
        }

        auto source = loader.getVariantSource(SH_FRAG_SURFACE, variant);
        ASSERT_FALSE(source.empty());
        ASSERT_EQ(source.find("#include"), std::string::npos) << "Shader preprocessing failed";
        std::erase_if(source, [](unsigned char c) { return std::isspace(c); });

        if (external) {
            EXPECT_NE(source.find("samplerExternalOEStex;"), std::string::npos);
            EXPECT_EQ(source.find("uniformsampler2Dtex;"), std::string::npos);
            EXPECT_NE(source.find("GL_OES_EGL_image_external_essl3"), std::string::npos);
        } else {
            EXPECT_NE(source.find("uniformsampler2Dtex;"), std::string::npos);
            EXPECT_EQ(source.find("samplerExternalOES"), std::string::npos);
        }

        EXPECT_NE(source.find("texture(tex,v_texcoord)"), std::string::npos);
        EXPECT_NE(source.find("pixColor=doColorManagement(pixColor,alpha,sourceTF,targetTF,convertMatrix"), std::string::npos);
        EXPECT_NE(source.find("pixColor.rgb=toLinearRGB(pixColor.rgb,srcTF);"), std::string::npos);
        EXPECT_NE(source.find("pixColor.rgb=convertMatrix*pixColor.rgb;"), std::string::npos);
        EXPECT_NE(source.find("pixColor=tonemap(pixColor,"), std::string::npos);
        EXPECT_NE(source.find("pixColor=fromLinearNit(pixColor,dstTF,dstTFRange);"), std::string::npos);
    }
}
