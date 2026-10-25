#include <gtest/gtest.h>

#include "GfxRenderer.h"

namespace {

bool framebufferBitSet(const GfxRenderer::RenderMode mode, const GfxRenderer::TwoBitPixel pixel) {
  return pixel.draw && !GfxRenderer::framebufferState(mode, pixel.state);
}

// Waveshare 2-bit gray: MSB plane uses values 1 and 2; LSB plane uses value 1 only.
constexpr bool kExpectedMsbBit[] = {false, true, true, false};
constexpr bool kExpectedLsbBit[] = {false, true, false, false};

}  // namespace

TEST(GfxTwoBitMapping, WaveshareGrayscaleFramebufferBitsMatchExpectedTable) {
  for (uint8_t value = 0; value < 4; ++value) {
    const bool msbBitmap =
        framebufferBitSet(GfxRenderer::GRAYSCALE_MSB, GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_MSB, value));
    const bool msbGlyph = framebufferBitSet(GfxRenderer::GRAYSCALE_MSB,
                                            GfxRenderer::mapTwoBitGlyphCoverage(GfxRenderer::GRAYSCALE_MSB, 3 - value));
    EXPECT_EQ(msbBitmap, kExpectedMsbBit[value]) << "MSB value=" << static_cast<int>(value);
    EXPECT_EQ(msbGlyph, kExpectedMsbBit[value]) << "MSB glyph value=" << static_cast<int>(value);

    const bool lsbBitmap =
        framebufferBitSet(GfxRenderer::GRAYSCALE_LSB, GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_LSB, value));
    const bool lsbGlyph = framebufferBitSet(GfxRenderer::GRAYSCALE_LSB,
                                            GfxRenderer::mapTwoBitGlyphCoverage(GfxRenderer::GRAYSCALE_LSB, 3 - value));
    EXPECT_EQ(lsbBitmap, kExpectedLsbBit[value]) << "LSB value=" << static_cast<int>(value);
    EXPECT_EQ(lsbGlyph, kExpectedLsbBit[value]) << "LSB glyph value=" << static_cast<int>(value);
  }
}

TEST(GfxTwoBitMapping, WaveshareGrayscaleMappingMatchesPanelExpectations) {
  const auto msb1 = GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_MSB, 1);
  EXPECT_TRUE(msb1.draw);
  EXPECT_FALSE(msb1.state);
  const auto lsb1 = GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_LSB, 1);
  EXPECT_TRUE(lsb1.draw);
  EXPECT_FALSE(lsb1.state);
  EXPECT_FALSE(GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_MSB, 0).draw);
  EXPECT_FALSE(GfxRenderer::mapTwoBitPixel(GfxRenderer::GRAYSCALE_LSB, 0).draw);
}

TEST(GfxTwoBitMapping, BwKeepsLogicalPixelState) {
  for (uint8_t value = 0; value < 4; ++value) {
    const auto pixel = GfxRenderer::mapTwoBitPixel(GfxRenderer::BW, value);
    EXPECT_EQ(pixel.draw, value < 3);
    EXPECT_TRUE(GfxRenderer::framebufferState(GfxRenderer::BW, pixel.state));
  }
}
