// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cmath>

#include "utils/color.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

TEST (ColorTest, ToOklchOfKnownColors)
{
  const auto white = Color::from_hex ("#FFFFFF").to_oklch ();
  EXPECT_NEAR (white.lightness, 1.0, 1e-6);
  EXPECT_NEAR (white.chroma, 0.0, 1e-6);

  const auto black = Color::from_hex ("#000000").to_oklch ();
  EXPECT_NEAR (black.lightness, 0.0, 1e-6);
  EXPECT_NEAR (black.chroma, 0.0, 1e-6);

  const auto orange = Color::from_hex ("#FF9616").to_oklch ();
  EXPECT_NEAR (orange.lightness, 0.7673, 0.001);
  EXPECT_NEAR (orange.chroma, 0.1721, 0.001);
  EXPECT_NEAR (orange.hue, 62.0, 0.1);

  const auto teal = Color::from_hex ("#003631").to_oklch ();
  EXPECT_NEAR (teal.lightness, 0.2993, 0.001);
  EXPECT_NEAR (teal.chroma, 0.0528, 0.001);
  EXPECT_NEAR (teal.hue, 184.9, 0.1);
}

// Worked examples from DESIGN.md "Clips"
TEST (ColorTest, FromOklchMatchesWorkedExamples)
{
  struct Anchor
  {
    double       lightness;
    double       chroma;
    double       hue;
    const char * hex;
  };
  const auto anchors = std::array<Anchor, 16>{
    Anchor{ .lightness = 0.30, .chroma = 0.065, .hue = 185, .hex = "#003631" },
    Anchor{ .lightness = 0.38, .chroma = 0.065, .hue = 185, .hex = "#054d47" },
    Anchor{ .lightness = 0.22, .chroma = 0.065, .hue = 185, .hex = "#00211d" },
    Anchor{ .lightness = 0.60, .chroma = 0.21,  .hue = 185, .hex = "#009488" },
    Anchor{ .lightness = 0.89, .chroma = 0.06,  .hue = 185, .hex = "#aee8e0" },
    Anchor{ .lightness = 0.84, .chroma = 0.06,  .hue = 185, .hex = "#9ed8cf" },
    Anchor{ .lightness = 0.81, .chroma = 0.06,  .hue = 185, .hex = "#95cec6" },
    Anchor{ .lightness = 0.44, .chroma = 0.18,  .hue = 185, .hex = "#006058" },
    Anchor{ .lightness = 0.30, .chroma = 0.065, .hue = 70,  .hex = "#422700" },
    Anchor{ .lightness = 0.38, .chroma = 0.065, .hue = 70,  .hex = "#583c17" },
    Anchor{ .lightness = 0.22, .chroma = 0.065, .hue = 70,  .hex = "#2a1500" },
    Anchor{ .lightness = 0.60, .chroma = 0.21,  .hue = 70,  .hex = "#b17000" },
    Anchor{ .lightness = 0.89, .chroma = 0.06,  .hue = 70,  .hex = "#f5d5b0" },
    Anchor{ .lightness = 0.84, .chroma = 0.06,  .hue = 70,  .hex = "#e4c4a1" },
    Anchor{ .lightness = 0.81, .chroma = 0.06,  .hue = 70,  .hex = "#dbbb97" },
    Anchor{ .lightness = 0.44, .chroma = 0.18,  .hue = 70,  .hex = "#744700" },
  };
  for (const auto &anchor : anchors)
    {
      const Color color =
        Color::from_oklch (anchor.lightness, anchor.chroma, anchor.hue);
      const QColor expected (anchor.hex);
      EXPECT_NEAR (color.red_, expected.redF (), 1.0 / 255.0);
      EXPECT_NEAR (color.green_, expected.greenF (), 1.0 / 255.0);
      EXPECT_NEAR (color.blue_, expected.blueF (), 1.0 / 255.0);
    }
}

TEST (ColorTest, OklchRoundTripKeepsColor)
{
  for (
    const char * hex : { "#FF9616", "#7C009B", "#26A269", "#003631", "#E3E3E3" })
    {
      const Color original = Color::from_hex (hex);
      const auto  oklch = original.to_oklch ();
      const Color back = Color::from_oklch (oklch);
      EXPECT_NEAR (back.red_, original.red_, 1.5 / 255.0);
      EXPECT_NEAR (back.green_, original.green_, 1.5 / 255.0);
      EXPECT_NEAR (back.blue_, original.blue_, 1.5 / 255.0);
    }
}

// In-gamut OKLCH coordinates survive a conversion round trip; the hue
// comparison wraps around the 360-degree circle
TEST (ColorTest, OklchRoundTripKeepsCoordinates)
{
  const auto samples = std::array<Oklch, 5>{
    Oklch{ .lightness = 0.40, .chroma = 0.04, .hue = 185 },
    Oklch{ .lightness = 0.40, .chroma = 0.05, .hue = 70  },
    Oklch{ .lightness = 0.89, .chroma = 0.03, .hue = 300 },
    Oklch{ .lightness = 0.82, .chroma = 0.10, .hue = 145 },
    Oklch{ .lightness = 0.50, .chroma = 0.05, .hue = 0   },
  };
  for (const auto &sample : samples)
    {
      const Oklch back = Color::from_oklch (sample).to_oklch ();
      EXPECT_NEAR (back.lightness, sample.lightness, 0.005);
      EXPECT_NEAR (back.chroma, sample.chroma, 0.008);
      const double hue_delta = std::abs (back.hue - sample.hue);
      const double wrapped = std::min (hue_delta, 360.0 - hue_delta);
      EXPECT_NEAR (wrapped, 0.0, 0.5);
    }
}

TEST (ColorTest, FromOklchReducesChromaToFitGamut)
{
  const Color over_saturated = Color::from_oklch (0.72, 0.5, 62.0);
  const Color also_over = Color::from_oklch (0.72, 0.2, 62.0);
  const Color in_gamut = Color::from_oklch (0.72, 0.15, 62.0);

  for (const Color * color : { &over_saturated, &also_over, &in_gamut })
    {
      EXPECT_GE (color->red_, 0.f);
      EXPECT_LE (color->red_, 1.f);
      EXPECT_GE (color->green_, 0.f);
      EXPECT_LE (color->green_, 1.f);
      EXPECT_GE (color->blue_, 0.f);
      EXPECT_LE (color->blue_, 1.f);
    }

  // chroma past what the gamut holds at this lightness converges to the
  // largest fitting chroma
  EXPECT_NEAR (over_saturated.red_, also_over.red_, 1.0 / 255.0);
  EXPECT_NEAR (over_saturated.green_, also_over.green_, 1.0 / 255.0);
  EXPECT_NEAR (over_saturated.blue_, also_over.blue_, 1.0 / 255.0);

  const bool matches_in_gamut =
    std::abs (over_saturated.red_ - in_gamut.red_) <= 1.f / 255.f
    && std::abs (over_saturated.green_ - in_gamut.green_) <= 1.f / 255.f
    && std::abs (over_saturated.blue_ - in_gamut.blue_) <= 1.f / 255.f;
  EXPECT_FALSE (matches_in_gamut);
}

TEST (ColorTest, LuminanceOfKnownColors)
{
  EXPECT_NEAR (Color::from_hex ("#FFFFFF").luminance (), 1.0, 1e-6);
  EXPECT_NEAR (Color::from_hex ("#000000").luminance (), 0.0, 1e-6);
  EXPECT_NEAR (Color::from_hex ("#808080").luminance (), 0.2159, 1e-3);
}

TEST (ColorTest, ContrastRatioOfKnownPairs)
{
  const auto white = Color::from_hex ("#FFFFFF");
  const auto black = Color::from_hex ("#000000");
  EXPECT_NEAR (Color::contrast_ratio (white, black), 21.0, 1e-6);
  EXPECT_NEAR (Color::contrast_ratio (white, white), 1.0, 1e-9);
  EXPECT_NEAR (
    Color::contrast_ratio (
      Color::from_hex ("#E3E3E3"), Color::from_hex ("#161616")),
    14.1, 0.05);
}

} // namespace zrythm::utils
