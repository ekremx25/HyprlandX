#include <helpers/Format.hpp>

#include <gtest/gtest.h>

#include <drm_fourcc.h>
#include <wayland-server-protocol.h>
#include <hyprgraphics/egl/Egl.hpp>
#include <limits>
#include <array>

using namespace Hyprgraphics::Egl;
using namespace NFormatUtils;

TEST(Helpers, formatDrmToShm) {
    EXPECT_EQ(drmToShm(DRM_FORMAT_XRGB8888), WL_SHM_FORMAT_XRGB8888);
    EXPECT_EQ(drmToShm(DRM_FORMAT_ARGB8888), WL_SHM_FORMAT_ARGB8888);
}

TEST(Helpers, formatShmToDrm) {
    EXPECT_EQ(shmToDRM(WL_SHM_FORMAT_XRGB8888), DRM_FORMAT_XRGB8888);
    EXPECT_EQ(shmToDRM(WL_SHM_FORMAT_ARGB8888), DRM_FORMAT_ARGB8888);
}

TEST(Helpers, formatDrmShmRoundTrip) {
    EXPECT_EQ(shmToDRM(drmToShm(DRM_FORMAT_XRGB8888)), DRM_FORMAT_XRGB8888);
    EXPECT_EQ(shmToDRM(drmToShm(DRM_FORMAT_ARGB8888)), DRM_FORMAT_ARGB8888);
}

TEST(Helpers, formatIsFormatYUV) {
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_YUYV));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_NV12));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_NV21));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_XRGB8888));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_ARGB8888));
}

TEST(Helpers, formatP010IsYUVNotRGB10) {
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_P010));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_XRGB2101010));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_ARGB2101010));
}

TEST(Helpers, formatGetPixelFormatFromDRM) {
    const auto* xrgb = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(xrgb, nullptr);
    EXPECT_EQ(xrgb->drmFormat, DRM_FORMAT_XRGB8888);
    EXPECT_FALSE(xrgb->withAlpha);

    const auto* argb = getPixelFormatFromDRM(DRM_FORMAT_ARGB8888);
    ASSERT_NE(argb, nullptr);
    EXPECT_EQ(argb->drmFormat, DRM_FORMAT_ARGB8888);
    EXPECT_TRUE(argb->withAlpha);

    EXPECT_EQ(getPixelFormatFromDRM(0), nullptr);
}

TEST(Helpers, formatIsFormatOpaque) {
    EXPECT_TRUE(isDrmFormatOpaque(DRM_FORMAT_XRGB8888));
    EXPECT_FALSE(isDrmFormatOpaque(DRM_FORMAT_ARGB8888));
}

TEST(Helpers, formatPixelsPerBlock) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);
    EXPECT_GT(pixelsPerBlock(fmt), 0);
}

TEST(Helpers, formatMinStride) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);
    // XRGB8888 = 4 bytes per pixel, 1920 wide = 7680 bytes stride
    EXPECT_EQ(minStride(fmt, 1920), 1920 * 4);
    EXPECT_EQ(minStride(fmt, 0), 0);
}

TEST(Helpers, formatShmBufferLayoutValid) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);

    const auto stride = static_cast<int32_t>(minStride(fmt, 64));
    const auto size   = static_cast<size_t>(stride) * 64;

    EXPECT_TRUE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 0, size));
    EXPECT_TRUE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 16, size + 16));

    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride - 1, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 1, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, -1, size));
    EXPECT_FALSE(isShmBufferLayoutValid(0, {64, 64}, stride, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {0, 64}, stride, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), 0));
}

TEST(Helpers, formatDrmFormatName) {
    EXPECT_FALSE(drmFormatName(DRM_FORMAT_XRGB8888).empty());
    EXPECT_FALSE(drmFormatName(DRM_FORMAT_ARGB8888).empty());
    EXPECT_EQ(drmFormatName(0), "INVALID");
}

TEST(Helpers, formatAlphaFormat) {
    EXPECT_EQ(alphaFormat(DRM_FORMAT_XRGB8888), DRM_FORMAT_ARGB8888);
    EXPECT_EQ(alphaFormat(DRM_FORMAT_XBGR8888), DRM_FORMAT_ABGR8888);
    // Format without alpha stripped entry returns DRM_FORMAT_INVALID (0)
    EXPECT_EQ(alphaFormat(DRM_FORMAT_ARGB8888), 0u);
}

TEST(Helpers, formatTexture2DRejectsExternalOnlyVideo) {
    const std::array<uint64_t, 2>   modifiers = {0x0200000000082305ULL, DRM_FORMAT_MOD_LINEAR};
    const std::array<EGLBoolean, 2> external  = {EGL_TRUE, EGL_TRUE};
    EXPECT_FALSE(getTexture2DModifiers(modifiers, external).has_value());
}

TEST(Helpers, formatTexture2DRejectsExternalOnlyWithoutLinear) {
    const std::array<uint64_t, 1>   modifiers = {0x0200000000082305ULL};
    const std::array<EGLBoolean, 1> external  = {EGL_TRUE};
    EXPECT_FALSE(getTexture2DModifiers(modifiers, external).has_value());
}

TEST(Helpers, formatTexture2DPreservesRGBAndLinear) {
    const std::array<uint64_t, 2>   modifiers = {0x0200000000082305ULL, DRM_FORMAT_MOD_LINEAR};
    const std::array<EGLBoolean, 2> external  = {EGL_FALSE, EGL_FALSE};
    const auto                      result    = getTexture2DModifiers(modifiers, external);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (std::vector<uint64_t>{modifiers[0], DRM_FORMAT_MOD_LINEAR}));
}

TEST(Helpers, formatTexture2DFiltersExternalLinearFromMixedSupport) {
    const std::array<uint64_t, 2>   modifiers = {0x0200000000082305ULL, DRM_FORMAT_MOD_LINEAR};
    const std::array<EGLBoolean, 2> external  = {EGL_FALSE, EGL_TRUE};
    const auto                      result    = getTexture2DModifiers(modifiers, external);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (std::vector<uint64_t>{modifiers[0]}));
}

TEST(Helpers, formatTexture2DPreservesImplicitOnlyFallback) {
    const auto result = getTexture2DModifiers({}, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(Helpers, formatTexture2DKeepsLinearFallbackForSupportedFormat) {
    const std::array<uint64_t, 1>   modifiers = {0x0200000000082305ULL};
    const std::array<EGLBoolean, 1> external  = {EGL_FALSE};
    const auto                      result    = getTexture2DModifiers(modifiers, external);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (std::vector<uint64_t>{modifiers[0], DRM_FORMAT_MOD_LINEAR}));
}
