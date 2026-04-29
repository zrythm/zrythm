// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cmath>

#include "utils/float_ranges.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::utils::float_ranges
{

TEST (FloatRangesTest, Fill)
{
  std::array<float, 4> buf = { 1.0f, 1.0f, 1.0f, 1.0f };
  fill (buf, 0.5f);
  for (float i : buf)
    {
      EXPECT_FLOAT_EQ (i, 0.5f);
    }
}

TEST (FloatRangesTest, Clip)
{
  std::array<float, 4> buf = { -2.0f, -0.5f, 0.5f, 2.0f };
  clip (buf, -1.0f, 1.0f);
  EXPECT_FLOAT_EQ (buf[0], -1.0f);
  EXPECT_FLOAT_EQ (buf[1], -0.5f);
  EXPECT_FLOAT_EQ (buf[2], 0.5f);
  EXPECT_FLOAT_EQ (buf[3], 1.0f);
}

TEST (FloatRangesTest, Copy)
{
  std::array<float, 4> src = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> dest = { 0.0f, 0.0f, 0.0f, 0.0f };
  copy (dest, src);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], src[i]);
    }
}

TEST (FloatRangesTest, MultiplyByConstant)
{
  std::array<float, 4> buf = { 1.0f, 2.0f, 3.0f, 4.0f };
  mul_k2 (buf, 2.0f);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (buf[i], (i + 1) * 2.0f);
    }
}

TEST (FloatRangesTest, AbsMax)
{
  std::array<float, 4> buf1 = { -0.7f, 0.2f, -0.9f, 0.5f };
  EXPECT_FLOAT_EQ (abs_max (buf1), 0.9f);

  std::array<float, 4> buf2 = { 0.1f, 0.4f, 0.8f, 0.6f };
  EXPECT_FLOAT_EQ (abs_max (buf2), 0.8f);

  std::array<float, 4> buf3 = { -0.2f, -0.95f, -0.4f, -0.6f };
  EXPECT_FLOAT_EQ (abs_max (buf3), 0.95f);

  std::array<float, 4> buf4 = { 0.001f, -0.002f, 0.001f, -0.001f };
  EXPECT_FLOAT_EQ (abs_max (buf4), 0.002f);

  std::array<float, 4> buf5 = { -0.75f, 0.3f, 0.75f, -0.2f };
  EXPECT_FLOAT_EQ (abs_max (buf5), 0.75f);
}

TEST (FloatRangesTest, MinMax)
{
  std::array<float, 4> buf = { -2.0f, 1.0f, -3.0f, 2.5f };
  EXPECT_FLOAT_EQ (min (buf), -3.0f);
  EXPECT_FLOAT_EQ (max (buf), 2.5f);
}

TEST (FloatRangesTest, Add)
{
  std::array<float, 4> dest = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> src = { 0.5f, 1.0f, 1.5f, 2.0f };
  add2 (dest, src);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], (i + 1) + (i + 1) * 0.5f);
    }
}

TEST (FloatRangesTest, Product)
{
  std::array<float, 4> src = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> dest = { 0.0f, 0.0f, 0.0f, 0.0f };
  product (dest, src, 2.5f);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], (i + 1) * 2.5f);
    }

  std::array<float, 3> src2 = { -1.0f, 0.0f, 2.0f };
  std::array<float, 3> dest2{};
  product (dest2, src2, -1.5f);
  EXPECT_FLOAT_EQ (dest2[0], 1.5f);
  EXPECT_FLOAT_EQ (dest2[1], 0.0f);
  EXPECT_FLOAT_EQ (dest2[2], -3.0f);

  std::array<float, 2> src3 = { 5.0f, -7.0f };
  std::array<float, 2> dest3{};
  product (dest3, src3, 0.0f);
  EXPECT_FLOAT_EQ (dest3[0], 0.0f);
  EXPECT_FLOAT_EQ (dest3[1], 0.0f);
}

TEST (FloatRangesTest, MixProduct)
{
  std::array<float, 4> dest = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> src = { 0.5f, 1.0f, 1.5f, 2.0f };
  mix_product (dest, src, 2.0f);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], (i + 1) + (i + 1) * 0.5f * 2.0f);
    }
}

TEST (FloatRangesTest, Reverse)
{
  std::array<float, 4> src = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> dest{};
  utils::float_ranges::reverse (dest, src);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], src[3 - i]);
    }
}

TEST (FloatRangesTest, Normalize)
{
  std::array<float, 4> src = { -2.0f, 1.0f, -3.0f, 2.5f };
  std::array<float, 4> dest{};
  normalize (dest, src);

  EXPECT_FLOAT_EQ (dest[0], -2.0f / 3.0f);
  EXPECT_FLOAT_EQ (dest[1], 1.0f / 3.0f);
  EXPECT_FLOAT_EQ (dest[2], -1.0f);
  EXPECT_FLOAT_EQ (dest[3], 2.5f / 3.0f);

  EXPECT_FLOAT_EQ (abs_max (dest), 1.0f);
}

TEST (FloatRangesTest, LinearFades)
{
  std::array<float, 8> buf = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  linear_fade_in_from (std::span (buf).first (4), 0, 4, 0.0f);
  EXPECT_FLOAT_EQ (buf[0], 0.0f);
  EXPECT_FLOAT_EQ (buf[3], 1.0f);

  linear_fade_out_to (std::span (buf).subspan (4), 0, 4, 0.0f);
  EXPECT_FLOAT_EQ (buf[4], 1.0f);
  EXPECT_FLOAT_EQ (buf[7], 0.0f);
}

TEST (FloatRangesTest, MakeMono)
{
  std::array<float, 4> l = { 1.0f, 2.0f, 3.0f, 4.0f };
  std::array<float, 4> r = { 2.0f, 3.0f, 4.0f, 5.0f };

  make_mono (l, r, true);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (l[i], r[i]);
    }

  make_mono (l, r, false);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (l[i], r[i]);
    }
}

TEST (FloatRangesTest, NormalizeSilentBuffer)
{
  std::array<float, 4> src = { 0.0f, 0.0f, 0.0f, 0.0f };
  std::array<float, 4> dest{};
  normalize (dest, src);

  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], 0.0f);
      EXPECT_FALSE (std::isnan (dest[i]));
      EXPECT_FALSE (std::isinf (dest[i]));
    }
}
}
