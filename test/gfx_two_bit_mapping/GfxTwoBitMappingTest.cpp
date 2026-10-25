#include <gtest/gtest.h>

#include "DirectPixelWriter.h"
#include "GfxRenderer.h"

namespace {

bool mappedBit(const GfxRenderer::RenderMode mode, const GfxRenderer::TwoBitPixel pixel) {
  return pixel.draw && !GfxRenderer::framebufferState(mode, pixel.state);
}

bool directBit(const GfxRenderer::RenderMode mode, const uint8_t value) {
  uint8_t framebuffer = 0;
  DirectPixelWriter writer{};
  writer.fb = &framebuffer;
  writer.mode = mode;
  writer.displayWidthBytes = 1;
  writer.originY = 0;
  writer.clipRows = 1;
  writer.phyXBase = 0;
  writer.phyYBase = 0;
  writer.phyXStepX = 1;
  writer.phyYStepX = 0;
  writer.phyXStepY = 0;
  writer.phyYStepY = 1;
  writer.beginRow(0);
  writer.writePixel(0, value);
  return (framebuffer & 0x80) != 0;
}

}  // namespace

TEST(GfxTwoBitMapping, WaveshareDirectGlyphAndBitmapProduceTheSameFramebufferBit) {
  for (const auto mode : {GfxRenderer::GRAYSCALE_MSB, GfxRenderer::GRAYSCALE_LSB}) {
    for (uint8_t value = 0; value < 4; ++value) {
      const bool direct = directBit(mode, value);
      const bool glyph = mappedBit(mode, GfxRenderer::mapTwoBitGlyphCoverage(mode, 3 - value));
      const bool bitmap = mappedBit(mode, GfxRenderer::mapTwoBitPixel(mode, value));
      EXPECT_EQ(direct, glyph) << "mode=" << mode << " value=" << static_cast<int>(value);
      EXPECT_EQ(direct, bitmap) << "mode=" << mode << " value=" << static_cast<int>(value);
    }
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
