// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * User Interface utils (legacy code)
 */

#pragma once

#include <utility>

#include "utils/color.h"
#include "utils/format.h"

/**
 * @addtogroup utils
 *
 * @{
 */

#define UI_CACHES (zrythm_app->ui_caches_)
#define UI_COLORS (&UI_CACHES->colors_)

/* copied from css */
constexpr const char * UI_COLOR_DARK_TEXT = "#323232";
constexpr const char * UI_COLOR_BRIGHT_TEXT = "#cdcdcd";
constexpr const char * UI_COLOR_YELLOW = "#F9CA1B";
constexpr const char * UI_COLOR_PURPLE = "#9D3955";
constexpr const char * UI_COLOR_BUTTON_NORMAL = "#343434";
constexpr const char * UI_COLOR_BUTTON_HOVER = "#444444";
constexpr const char * UI_COLOR_RECORD_CHECKED = "#ED2939";
constexpr const char * UI_COLOR_RECORD_ACTIVE = "#FF2400";
constexpr const char * UI_COLOR_BRIGHT_GREEN = "#1DD169";
constexpr const char * UI_COLOR_DARKISH_GREEN = "#19664c";
constexpr const char * UI_COLOR_DARK_ORANGE = "#D68A0C";
constexpr const char * UI_COLOR_Z_YELLOW = "#F9CA1B";
constexpr const char * UI_COLOR_BRIGHT_ORANGE = "#F79616";
constexpr const char * UI_COLOR_Z_PURPLE = "#9D3955";
constexpr const char * UI_COLOR_MATCHA = "#2eb398";
constexpr const char * UI_COLOR_LIGHT_BLUEISH = "#1aa3ffcc";
constexpr const char * UI_COLOR_PREFADER_SEND = "#D21E6D";
constexpr const char * UI_COLOR_POSTFADER_SEND = "#901ed2";
constexpr const char * UI_COLOR_SOLO_ACTIVE = UI_COLOR_MATCHA;
constexpr const char * UI_COLOR_SOLO_CHECKED = UI_COLOR_DARKISH_GREEN;
constexpr const char * UI_COLOR_HIGHLIGHT_SCALE_BG = "#662266";
constexpr const char * UI_COLOR_HIGHLIGHT_CHORD_BG = "#BB22BB";
constexpr const char * UI_COLOR_HIGHLIGHT_BASS_BG = UI_COLOR_LIGHT_BLUEISH;
constexpr const char * UI_COLOR_HIGHLIGHT_BOTH_BG = "#FF22FF";
constexpr const char * UI_COLOR_HIGHLIGHT_SCALE_FG = "#F79616";
constexpr const char * UI_COLOR_HIGHLIGHT_CHORD_FG = UI_COLOR_HIGHLIGHT_SCALE_FG;
constexpr const char * UI_COLOR_HIGHLIGHT_BASS_FG = "white";
constexpr const char * UI_COLOR_HIGHLIGHT_BOTH_FG = "white";
constexpr const char * UI_COLOR_FADER_FILL_END = UI_COLOR_Z_YELLOW;
constexpr const char * UI_DELETE_ICON_NAME = "z-edit-delete";

using Color = zrythm::utils::Color;

/**
 * Commonly used UI colors.
 */
class UiColors
{
public:
  Color dark_text;
  Color dark_orange;
  Color bright_orange;
  Color bright_text;
  Color matcha;
  Color bright_green;
  Color darkish_green;
  Color prefader_send;
  Color postfader_send;
  Color record_active;
  Color record_checked;
  Color solo_active;
  Color solo_checked;
  Color fader_fill_start;
  Color fader_fill_end;
  Color highlight_scale_bg;
  Color highlight_chord_bg;
  Color highlight_bass_bg;
  Color highlight_both_bg;
  Color highlight_scale_fg;
  Color highlight_chord_fg;
  Color highlight_bass_fg;
  Color highlight_both_fg;
  Color z_yellow;
  Color z_purple;
};

/**
 * Commonly used UI textures.
 */
class UiTextures
{
};

/**
 * Various cursor states to be shared.
 */
enum class UiCursorState
{
  UI_CURSOR_STATE_DEFAULT,
  UI_CURSOR_STATE_RESIZE_L,
  UI_CURSOR_STATE_REPEAT_L,
  UI_CURSOR_STATE_RESIZE_R,
  UI_CURSOR_STATE_REPEAT_R,
  UI_CURSOR_STATE_RESIZE_UP,
};

/**
 * Various overlay actions to be shared.
 */
enum class UiOverlayAction
{
  None,
  CreatingResizingR,
  CREATING_MOVING,
  ResizingL,
  ResizingLLoop,
  ResizingLFade,
  ResizingR,
  ResizingRLoop,
  ResizingRFade,
  RESIZING_UP,
  RESIZING_UP_FADE_IN,
  RESIZING_UP_FADE_OUT,
  StretchingL,
  StretchingR,

  STARTING_AUDITIONING,
  AUDITIONING,

  /**
   * Auto-filling in edit mode.
   *
   * @note This is also used for the pencil tool in
   *   velocity and automation editors.
   */
  AUTOFILLING,

  /** Erasing. */
  ERASING,
  STARTING_ERASING,

  /**
   * To be set in drag_start.
   */
  STARTING_MOVING,
  STARTING_MOVING_COPY,
  STARTING_MOVING_LINK,
  MOVING,
  MovingCopy,
  MOVING_LINK,
  STARTING_CHANGING_CURVE,
  CHANGING_CURVE,

  /**
   * To be set in drag_start.
   *
   * Useful to check if nothing was clicked.
   */
  STARTING_SELECTION,
  SELECTING,

  /** Like selecting but it auto deletes whatever
   * touches the selection. */
  STARTING_DELETE_SELECTION,
  DELETE_SELECTING,

  STARTING_RAMP,
  RAMPING,
  CUTTING,

  RENAMING,

  StartingPanning,
  Panning,
  NUM_UI_OVERLAY_ACTIONS,
};

DEFINE_ENUM_FORMATTER (
  UiOverlayAction,
  UiOverlayAction,
  "NONE",
  "RESIZING_R",
  "MOVING",
  "RESIZING_L",
  "RESIZING_L_LOOP",
  "RESIZING_L_FADE",
  "RESIZING_R",
  "RESIZING_R_LOOP",
  "RESIZING_R_FADE",
  "RESIZING_UP",
  "RESIZING_UP_FADE_IN",
  "RESIZING_UP_FADE_OUT",
  "STRETCHING_L",
  "STRETCHING_R",
  "STARTING_AUDITIONING",
  "AUDITIONING",
  "AUTOFILLING",
  "ERASING",
  "STARTING_ERASING",
  "STARTING_MOVING",
  "STARTING_MOVING_COPY",
  "STARTING_MOVING_LINK",
  "MOVING",
  "MOVING_COPY",
  "MOVING_LINK",
  "STARTING_CHANGING_CURVE",
  "CHANGING_CURVE",
  "STARTING_SELECTION",
  "SELECTING",
  "STARTING_DELETE_SELECTION",
  "DELETE_SELECTING",
  "STARTING_RAMP",
  "RAMPING",
  "CUTTING",
  "RENAMING",
  "STARTING_PANNING",
  "PANNING",
  "--INVALID--")

/**
 * Dragging modes for widgets that have click&drag.
 */
enum class UiDragMode
{
  /** Value is wherever the cursor is. */
  UI_DRAG_MODE_CURSOR,

  /** Value is changed based on the offset. */
  UI_DRAG_MODE_RELATIVE,

  /** Value is changed based on the offset, times
   * a multiplier. */
  UI_DRAG_MODE_RELATIVE_WITH_MULTIPLIER,
};

#if 0
void
ui_set_pointer_cursor (GtkWidget * widget);

#  define ui_set_pencil_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "edit-cursor", 2, 3);

#  define ui_set_brush_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "brush-cursor", 2, 3);

#  define ui_set_cut_clip_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "cut-cursor", 9, 7);

#  define ui_set_eraser_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "eraser-cursor", 4, 2);

#  define ui_set_line_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "ramp-cursor", 2, 3);

#  define ui_set_speaker_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "audition-cursor", 10, 12);

#  define ui_set_hand_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "move-cursor", 12, 11);

#  define ui_set_left_resize_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "w-resize-cursor", 14, 11);

#  define ui_set_left_stretch_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "w-stretch-cursor", 14, 11);

#  define ui_set_left_resize_loop_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "w-loop-cursor", 14, 11);

#  define ui_set_right_resize_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "e-resize-cursor", 10, 11);

#  define ui_set_right_stretch_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "e-stretch-cursor", 10, 11);

#  define ui_set_right_resize_loop_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "e-loop-cursor", 10, 11);

#  define ui_set_time_select_cursor(widget) \
    ui_set_cursor_from_icon_name ( \
      GTK_WIDGET (widget), "time-select-cursor", 10, 12);

#  define ui_set_fade_in_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "fade-in-cursor", 3, 1);

#  define ui_set_fade_out_cursor(widget) \
    ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "fade-out-cursor", 3, 1);
#endif

/**
 * Gets a draggable value as a normalized value
 * between 0 and 1.
 *
 * @param size Widget size (either width or height).
 * @param start_px Px at start of drag.
 * @param cur_px Current px.
 * @param last_px Px during last call.
 */
double
ui_get_normalized_draggable_value (
  double     size,
  double     cur_val,
  double     start_px,
  double     cur_px,
  double     last_px,
  double     multiplier,
  UiDragMode mode);

/**
 * Returns an appropriate string representation of the given
 * dB value.
 */
std::string
ui_get_db_value_as_string (float val);

/**
 * @}
 */
