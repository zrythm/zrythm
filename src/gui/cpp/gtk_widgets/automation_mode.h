// SPDX-FileCopyrightText: © 2019-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_AUTOMATION_MODE_H__
#define __GUI_WIDGETS_AUTOMATION_MODE_H__

#include "dsp/automation_point.h"
#include "gui/cpp/gtk_widgets/custom_button.h"
#include "utils/color.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

constexpr int AUTOMATION_MODE_HPADDING = 3;
constexpr int AUTOMATION_MODE_HSEPARATOR_SIZE = 1;

/**
 * Custom button group to be drawn inside drawing areas.
 */
class AutomationModeWidget
{
public:
  /**
   * Creates a new track widget from the given track.
   */
  AutomationModeWidget (int height, PangoLayout * layout, AutomationTrack * owner);
  ~AutomationModeWidget ();

  void init ();

  void draw (
    GtkSnapshot *             snapshot,
    double                    x,
    double                    y,
    double                    x_cursor,
    CustomButtonWidget::State state);

private:
  AutomationMode get_hit_mode (double x) const;

  /**
   * Gets the color for @ref state for @ref mode.
   *
   * @param state new state.
   * @param mode AutomationMode.
   */
  void get_color_for_state (
    CustomButtonWidget::State state,
    AutomationMode            mode,
    GdkRGBA *                 c);

  /**
   * @param mode Mode the state applies to.
   * @param state This state only applies to the mode.
   */
  void draw_bg (
    GtkSnapshot * snapshot,
    double        x,
    double        y,
    int           draw_frame,
    bool          keep_clip);

public:
  /** X/y relative to parent drawing area. */
  double x_;
  double y_;

  /** Total width/height. */
  int width_;
  int height_;

  /** Width of each button, including padding. */

  int text_widths_[static_cast<int> (AutomationMode::NUM_AUTOMATION_MODES)];
  int text_heights_[static_cast<int> (AutomationMode::NUM_AUTOMATION_MODES)];
  int max_text_height_;

  int has_hit_mode_;

  /** Currently hit mode. */
  AutomationMode hit_mode_;

  /** Default color. */
  GdkRGBA def_color_;

  /** Hovered color. */
  GdkRGBA hovered_color_;

  /** Toggled color. */
  std::array<GdkRGBA, NUM_AUTOMATION_MODES> toggled_colors_;

  /** Held color (used after clicking and before releasing). */
  std::array<GdkRGBA, NUM_AUTOMATION_MODES> held_colors_;

  /** Aspect ratio for the rounded rectangle. */
  double aspect_;

  /** Corner curvature radius for the rounded
   * rectangle. */
  double corner_radius_;

  /** Used to update caches if state changed. */
  std::array<CustomButtonWidget::State, NUM_AUTOMATION_MODES> last_states_;

  /** Used during drawing. */
  std::array<CustomButtonWidget::State, NUM_AUTOMATION_MODES> current_states_;

  /** Used during transitions. */
  std::array<GdkRGBA, NUM_AUTOMATION_MODES> last_colors_;

  /** Cache layout for drawing text. */
  PangoLayout * layout_;

  /** Owner. */
  AutomationTrack * owner_;

  /** Frames left for a transition in color. */
  int transition_frames_;
};

/**
 * @}
 */

#endif
