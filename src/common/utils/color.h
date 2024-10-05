// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_COLOR_H__
#define __UTILS_COLOR_H__

#include "common/io/serialization/iserializable.h"

TYPEDEF_STRUCT_UNDERSCORED (GdkRGBA);

/**
 * @addtogroup utils
 *
 * @{
 */

class Color final : public ISerializable<Color>
{
public:
  static constexpr auto DEFAULT_BRIGHTEN_VAL = 0.1f;

  Color () = default;
  Color (const GdkRGBA &color) { *this = color; }
  Color (float r, float g, float b, float a = 1.0f);

  /**
   * @brief Construct a new Color object by parsing the color string.
   *
   * @param str
   */
  Color (const std::string &str);

  Color &operator= (const GdkRGBA &color);

  /**
   * Brightens the color by the given amount.
   */
  Color &brighten (float val);

  /**
   * Brightens the color by the default amount.
   */
  Color &brighten_default ();

  /**
   * Darkens the color by the given amount.
   */
  Color &darken (float val);

  /**
   * Darkens the color by the default amount.
   */
  Color &darken_default ();

  /**
   * Returns whether the color is the same as another.
   */
  [[nodiscard]] bool is_same (const Color &other) const;

  /**
   * Returns if the color is bright or not.
   */
  [[nodiscard]] bool is_bright () const;

  /**
   * Returns if the color is very bright or not.
   */
  [[nodiscard]] bool is_very_bright () const;

  /**
   * Returns if the color is very very bright or not.
   */
  [[nodiscard]] bool is_very_very_bright () const;

  /**
   * Returns if the color is very dark or not.
   */
  [[nodiscard]] bool is_very_dark () const;

  /**
   * Returns if the color is very very dark or not.
   */
  [[nodiscard]] bool is_very_very_dark () const;

  /**
   * Gets the opposite color.
   */
  [[nodiscard]] Color get_opposite () const;

  /**
   * Gets the brightness of the color.
   */
  [[nodiscard]] float get_brightness () const;

  /**
   * Gets the darkness of the color.
   */
  [[nodiscard]] float get_darkness () const;

  /**
   * Morphs from this color to another, depending on the given amount.
   *
   * Eg, if @a amt is 0, the resulting color will be
   * this color. If @a amt is 1, the resulting color will be
   * @a other.
   */
  [[nodiscard]] Color morph (const Color &other, float amt) const;

  /**
   * Returns the color in-between two colors.
   *
   * @param transition How far to transition (0.5 for half).
   */
  static Color
  get_mid_color (const Color &c1, const Color &c2, const float transition = 0.5f);

  /**
   * Returns the contrasting color (variation of black or white) based on if the
   * given color is dark enough or not.
   */
  Color get_contrast_color () const;

  GdkRGBA to_gdk_rgba () const;
  GdkRGBA to_gdk_rgba_with_alpha (float alpha) const;

  /**
   * Gets the color the widget should be.
   *
   * @param color The original color.
   * @param is_selected Whether the widget is supposed to be selected or not.
   */
  static Color get_arranger_object_color (
    const Color &color,
    const bool   is_hovered,
    const bool   is_selected,
    const bool   is_transient,
    const bool   is_muted);

  /**
   * @brief Converts the color to a hex string.
   *
   * @return std::string
   */
  std::string to_hex () const;

  static std::string rgb_to_hex (float r, float g, float b);

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  float red_ = 0.f;    ///< Red.
  float green_ = 0.f;  ///< Green.
  float blue_ = 0.f;   ///< Blue.
  float alpha_ = 1.0f; ///< Alpha.
};

bool
operator== (const Color &lhs, const Color &rhs);

/**
 * @}
 */

#endif
