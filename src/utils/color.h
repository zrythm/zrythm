// SPDX-FileCopyrightText: © 2020-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>

#include <boost/describe.hpp>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::utils
{

class Color final
{
public:
  static constexpr auto DEFAULT_BRIGHTEN_VAL = 0.1f;

  Color () noexcept = default;
  Color (const QColor &color) noexcept { *this = color; }
  Color (float r, float g, float b, float a = 1.0f) noexcept;

  /**
   * @brief Construct a new Color object by parsing a hex color string.
   *
   * @param hex Hex color string (e.g., "#ff5588" or "#ff5588cc")
   */
  Color (std::string_view hex);

  Color &operator= (const QColor &color);

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

  QColor to_qcolor () const;

  /**
   * Gets the color the widget should be.
   *
   * @param color The original color.
   * @param is_selected Whether the widget is supposed to be selected or not.
   */
  static Color get_arranger_object_color (
    const Color &color,
    bool         is_hovered,
    bool         is_selected,
    bool         is_transient,
    bool         is_muted);

  /**
   * @brief Converts the color to a hex string (RGB only).
   *
   * @return std::string Hex color string (e.g., "#ff5588")
   */
  std::string to_hex () const;

  /**
   * @brief Converts RGB float values to a hex string.
   *
   * @param r Red (0.0-1.0)
   * @param g Green (0.0-1.0)
   * @param b Blue (0.0-1.0)
   * @return std::string Hex color string (e.g., "#ff5588")
   */
  static std::string rgb_to_hex (float r, float g, float b);

  /**
   * @brief Parses a hex color string using QColor.
   *
   * Supports QColor-compatible formats: #RGB, #RRGGBB
   * Alpha defaults to 1.0.
   *
   * @param hex Hex color string (e.g., "#ff5588")
   * @return Color Parsed color
   */
  static Color from_hex (std::string_view hex);

  friend bool operator== (const Color &lhs, const Color &rhs);

  friend void to_json (nlohmann::json &j, const Color &color);
  friend void from_json (const nlohmann::json &j, Color &color);

public:
  float red_ = 0.f;    ///< Red.
  float green_ = 0.f;  ///< Green.
  float blue_ = 0.f;   ///< Blue.
  float alpha_ = 1.0f; ///< Alpha.

  BOOST_DESCRIBE_CLASS (Color, (), (red_, green_, blue_, alpha_), (), ())
};

}; // namespace zrythm::utils
