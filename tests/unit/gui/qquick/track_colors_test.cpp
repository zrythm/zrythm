// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/track_colors.h"
#include "utils/color_math.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

using Type = structure::tracks::Track::Type;

// The hue table is an exhaustive switch over Track::Type, so a track
// type without a hue fails to compile; these tests pin the derivation
// properties of the table
TEST (TrackColorTableTest, CanonicalTypesCoverEveryTypeInHueOrder)
{
  const QVariantList types = TrackColorTable::canonicalTypes ();
  EXPECT_EQ (types.size (), 12);

  double previous_hue = -1;
  for (const auto &type_variant : types)
    {
      const auto   type = static_cast<Type> (type_variant.toInt ());
      const double hue = TrackColorTable::hueOf (type);
      EXPECT_GE (hue, previous_hue);
      previous_hue = hue;
    }
}

TEST (TrackColorTableTest, DefaultsAreValidInBothModes)
{
  for (const auto &type_variant : TrackColorTable::canonicalTypes ())
    {
      const auto type = static_cast<Type> (type_variant.toInt ());
      for (bool dark : { true, false })
        {
          const QColor color = TrackColorTable::defaultColor (type, dark);
          EXPECT_TRUE (color.isValid ());
          EXPECT_EQ (color.alpha (), 255);
        }
    }
}

TEST (TrackColorTableTest, LightDefaultsMeetContrastTarget)
{
  const QColor light_page ("#E3E3E3");
  for (const auto &type_variant : TrackColorTable::canonicalTypes ())
    {
      const auto   type = static_cast<Type> (type_variant.toInt ());
      const QColor light_default = TrackColorTable::defaultColor (type, false);
      EXPECT_GE (utils::ColorMath::contrastRatio (light_default, light_page), 3.0)
        << "type " << type_variant.toInt () << " below 3:1";
    }
}

} // namespace zrythm::gui::qquick
