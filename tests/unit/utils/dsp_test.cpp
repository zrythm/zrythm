// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/dsp.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::utils::float_ranges;

TEST (DspTest, Fill)
{
  float buf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  fill (buf, 0.5f, 4);
  for (float i : buf)
    {
      EXPECT_FLOAT_EQ (i, 0.5f);
    }
}

TEST (DspTest, Clip)
{
  float buf[4] = { -2.0f, -0.5f, 0.5f, 2.0f };
  clip (buf, -1.0f, 1.0f, 4);
  EXPECT_FLOAT_EQ (buf[0], -1.0f);
  EXPECT_FLOAT_EQ (buf[1], -0.5f);
  EXPECT_FLOAT_EQ (buf[2], 0.5f);
  EXPECT_FLOAT_EQ (buf[3], 1.0f);
}

TEST (DspTest, Copy)
{
  float src[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float dest[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  copy (dest, src, 4);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], src[i]);
    }
}

TEST (DspTest, MultiplyByConstant)
{
  float buf[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  mul_k2 (buf, 2.0f, 4);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (buf[i], (i + 1) * 2.0f);
    }
}

TEST (DspTest, AbsMax)
{
  // Test with mixed positive/negative values
  float buf1[4] = { -0.7f, 0.2f, -0.9f, 0.5f };
  EXPECT_FLOAT_EQ (abs_max (buf1, 4), 0.9f);

  // Test with all positive values
  float buf2[4] = { 0.1f, 0.4f, 0.8f, 0.6f };
  EXPECT_FLOAT_EQ (abs_max (buf2, 4), 0.8f);

  // Test with all negative values
  float buf3[4] = { -0.2f, -0.95f, -0.4f, -0.6f };
  EXPECT_FLOAT_EQ (abs_max (buf3, 4), 0.95f);

  // Test with very quiet signals
  float buf4[4] = { 0.001f, -0.002f, 0.001f, -0.001f };
  EXPECT_FLOAT_EQ (abs_max (buf4, 4), 0.002f);

  // Test with equal positive/negative peaks
  float buf5[4] = { -0.75f, 0.3f, 0.75f, -0.2f };
  EXPECT_FLOAT_EQ (abs_max (buf5, 4), 0.75f);
}

TEST (DspTest, MinMax)
{
  float buf[4] = { -2.0f, 1.0f, -3.0f, 2.5f };
  EXPECT_FLOAT_EQ (min (buf, 4), -3.0f);
  EXPECT_FLOAT_EQ (max (buf, 4), 2.5f);
}

TEST (DspTest, Add)
{
  float dest[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float src[4] = { 0.5f, 1.0f, 1.5f, 2.0f };
  add2 (dest, src, 4);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], (i + 1) + (i + 1) * 0.5f);
    }
}

TEST (DspTest, MixProduct)
{
  float dest[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float src[4] = { 0.5f, 1.0f, 1.5f, 2.0f };
  mix_product (dest, src, 2.0f, 4);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], (i + 1) + (i + 1) * 0.5f * 2.0f);
    }
}

TEST (DspTest, Reverse)
{
  float src[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float dest[4];
  reverse (dest, src, 4);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (dest[i], src[3 - i]);
    }
}

TEST (DspTest, Normalize)
{
  float src[4] = { -2.0f, 1.0f, -3.0f, 2.5f };
  float dest[4];
  normalize (dest, src, 4, true);

  // Peak value is -3.0f, so scale factor should be 1/3
  EXPECT_FLOAT_EQ (dest[0], -2.0f / 3.0f);
  EXPECT_FLOAT_EQ (dest[1], 1.0f / 3.0f);
  EXPECT_FLOAT_EQ (dest[2], -1.0f);
  EXPECT_FLOAT_EQ (dest[3], 2.5f / 3.0f);

  // Verify peak is 1.0
  EXPECT_FLOAT_EQ (abs_max (dest, 4), 1.0f);
}

TEST (DspTest, LinearFades)
{
  float buf[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  // Test fade in
  linear_fade_in_from (buf, 0, 4, 4, 0.0f);
  EXPECT_FLOAT_EQ (buf[0], 0.0f);
  EXPECT_FLOAT_EQ (buf[3], 1.0f);

  // Test fade out
  linear_fade_out_to (buf + 4, 0, 4, 4, 0.0f);
  EXPECT_FLOAT_EQ (buf[4], 1.0f);
  EXPECT_FLOAT_EQ (buf[7], 0.0f);
}

TEST (DspTest, MakeMono)
{
  float l[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  float r[4] = { 2.0f, 3.0f, 4.0f, 5.0f };

  // Test equal power
  make_mono (l, r, 4, true);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (l[i], r[i]);
    }

  // Test equal amplitude
  make_mono (l, r, 4, false);
  for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ (l[i], r[i]);
    }
}
