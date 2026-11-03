#include <desktop/rule/windowRule/WindowRule.hpp>
#include <render/OpenGL.hpp>
#include <render/pass/TexPassElement.hpp>
#include <gtest/gtest.h>

using namespace Desktop::Rule;

TEST(WindowSaturation, DefaultsAreNeutral) {
    EXPECT_FLOAT_EQ(Render::GL::CHyprOpenGLImpl::STextureRenderData{}.saturation, 1.F);
    EXPECT_FLOAT_EQ(CTexPassElement::SRenderData{}.saturation, 1.F);
}

TEST(WindowSaturation, AcceptsFiniteBoundedValues) {
    for (const auto& raw : {"0", "1", "1.08", "2"}) {
        CWindowRule rule("saturation-test");
        ASSERT_TRUE(rule.addEffect(WINDOW_RULE_EFFECT_SATURATION, std::string{raw})) << raw;
        ASSERT_EQ(rule.effects().size(), 1);
        EXPECT_FLOAT_EQ(std::get<float>(rule.effects().front().value), std::stof(raw));
    }
}

TEST(WindowSaturation, RejectsInvalidValues) {
    for (const auto& raw : {"-0.01", "2.01", "nan", "inf", "-inf", "", "1.08garbage"}) {
        CWindowRule rule("saturation-test");
        EXPECT_FALSE(rule.addEffect(WINDOW_RULE_EFFECT_SATURATION, std::string{raw})) << raw;
        EXPECT_TRUE(rule.effects().empty());
    }
}
