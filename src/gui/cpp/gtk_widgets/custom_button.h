// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Custom button to be drawn inside drawing areas.
 */

#ifndef __GUI_WIDGETS_CUSTOM_BUTTON_H__
#define __GUI_WIDGETS_CUSTOM_BUTTON_H__

#include <string>

#include "utils/color.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

constexpr int CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES = 9;

/**
 * Custom button to be drawn inside drawing areas.
 */
class CustomButtonWidget
{
public:
  enum class State
  {
    NORMAL,
    HOVERED,
    ACTIVE,
    TOGGLED,

    /** Only border is toggled. */
    SEMI_TOGGLED,
  };

  enum class Owner
  {
    TRACK,
    LANE,
    AT,
  };

public:
  CustomButtonWidget ();
  CustomButtonWidget (const std::string &iicon_name, int isize);
  CustomButtonWidget (const CustomButtonWidget &) = delete;
  CustomButtonWidget &operator= (const CustomButtonWidget &) = delete;
  CustomButtonWidget (CustomButtonWidget &&) = default;
  CustomButtonWidget &operator= (CustomButtonWidget &&) = default;
  ~CustomButtonWidget ();

  void draw (GtkSnapshot * snapshot, double ix, double iy, State state);

  /**
   * @param width Max width for the button to use.
   */
  void draw_with_text (
    GtkSnapshot * snapshot,
    double        ix,
    double        iy,
    double        iwidth,
    State         state);

  /**
   * Sets the text and layout to draw the text width.
   *
   * @param font_descr Font description to set the
   *   pango layout font to.
   */
  void set_text (
    PangoLayout *      ilayout,
    const std::string &itext,
    const std::string &font_descr);

private:
  void init ();

  GdkRGBA get_color_for_state (State state) const;

  void draw_bg (
    GtkSnapshot * snapshot,
    double        ix,
    double        iy,
    double        iwidth,
    int           draw_frame,
    State         state);

  void draw_icon_with_shadow (
    GtkSnapshot * snapshot,
    double        ix,
    double        iy,
    State         state);

public:
  /** Default color. */
  Color def_color = {};

  /** Hovered color. */
  Color hovered_color = {};

  /** Toggled color. */
  Color toggled_color = {};

  /** Held color (used after clicking and before
   * releasing). */
  Color held_color = {};

  /** Name of the icon to show. */
  std::string icon_name;

  /** Size in pixels (width and height will be set to this). */
  int size = 0;

  /** if non-zero, the button has "size" height and this width. */
  int width = 0;

  /** Aspect ratio for the rounded rectangle. */
  double aspect = 1.0;

  /** Corner curvature radius for the rounded rectangle. */
  double corner_radius = 0.0;

  /** The icon surface. */
  GdkTexture * icon_texture = nullptr;

  /** Used to update caches if state changed. */
  State last_state = State::NORMAL;

  /** Owner type. */
  Owner owner_type = Owner::TRACK;

  /** Owner. */
  void * owner = nullptr;

  /** Used during transitions. */
  Color last_color = {};

  /**
   * Text, if any, to show after the icon.
   *
   * This will be ellipsized.
   */
  std::string text;

  int text_height = 0;

  /** Cache layout for drawing the text. */
  PangoLayout * layout = nullptr;

  /** X/y relative to parent drawing area. */
  double x = 0;
  double y = 0;

  /**
   * The id of the button returned by a symap of its
   * icon name, for better performance vs comparing
   * strings.
   *
   * TODO
   */
  unsigned int button_id = 0;

  /** Frames left for a transition in color. */
  int transition_frames = 0;
};

/**
 * @}
 */

#endif
