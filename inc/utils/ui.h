// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * User Interface utils.
 *
 */

#ifndef __UTILS_UI_H__
#define __UTILS_UI_H__

#include <utility>

#include "utils/color.h"
#include "utils/localization.h"
#include "utils/logger.h"
#include "utils/types.h"

#include <glib/gi18n.h>

#include "format.h"
#include "gtk_wrapper.h"
#include "libadwaita_wrapper.h"

class Position;
class Port;

/**
 * @addtogroup utils
 *
 * @{
 */

#define UI_CACHES (zrythm_app->ui_caches)
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

static const Color UI_COLOR_BLACK = { 0, 0, 0, 1 };

enum class UiDetail
{
  High,
  Normal,
  Low,
  UltraLow,
};

DEFINE_ENUM_FORMATTER (
  UiDetail,
  UiDetail,
  N_ ("High"),
  N_ ("Normal"),
  N_ ("Low"),
  N_ ("Ultra Low"));

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
 * Specification for a cursor.
 *
 * Used for caching.
 */
class UiCursor
{
public:
  UiCursor (std::string name, GdkCursor * cursor, int offset_x, int offset_y)
      : name_ (std::move (name)), cursor_ (cursor), offset_x_ (offset_x),
        offset_y_ (offset_y)
  {
  }
  ~UiCursor ()
  {
    if (cursor_)
      g_object_unref (cursor_);
  }

public:
  std::string name_;
  GdkCursor * cursor_;
  int         offset_x_;
  int         offset_y_;
};

/**
 * Caches.
 */
class UiCaches
{
public:
  UiCaches ();

public:
  UiColors colors_;
  // UiTextures    textures;
  std::vector<UiCursor> cursors_;

  bool     detail_level_set_;
  UiDetail detail_level_;
};

/**
 * Space on the edges to show resize cursors
 */
static constexpr int UI_RESIZE_CURSOR_SPACE = 8;

/*
 * Drag n Drop related.
 * Gtk target entries.
 */

/** Plugin descriptor, used to instantiate plugins.
 */
static constexpr const char * TARGET_ENTRY_PLUGIN_DESCR = "PLUGIN_DESCR";

/** For FileDescriptor pointers. */
static constexpr const char * TARGET_ENTRY_SUPPORTED_FILE = "SUPPORTED_FILE";

/** Plugin ID, used to move/copy plugins. */
static constexpr const char * TARGET_ENTRY_PLUGIN = "PLUGIN";

/* File path. Not used at the moment. */
static constexpr const char * TARGET_ENTRY_FILE_PATH = "FILE_PATH";

/** URI list. */
static constexpr const char * TARGET_ENTRY_URI_LIST = "text/uri-list";

/**
 * Track target entry.
 *
 * This is just the identifier. The
 * TracklistSelections will be used.
 */
static constexpr const char * TARGET_ENTRY_TRACK = "TRACK";

/**
 * Chord descriptor target entry.
 */
static constexpr const char * TARGET_ENTRY_CHORD_DESCR = "CHORD_DESCR";

/* */
static constexpr const char * TARGET_ENTRY_TL_SELECTIONS = "TL_SELECTIONS";

/**
 * Shows the notification when idle.
 *
 * This should be called from threads other than GTK main thread.
 */
#define ui_show_notification_idle_printf(fmt, ...) \
  char * text = g_strdup_printf (fmt, __VA_ARGS__); \
  g_idle_add ((GSourceFunc) ui_show_notification_idle_func, (void *) text)

#define ui_show_notification_idle(msg) \
  ui_show_notification_idle_printf ("%s", msg)

#define ui_is_widget_revealed(widget) \
  (gtk_widget_get_height (GTK_WIDGET (widget)) > 1 \
   || gtk_widget_get_width (GTK_WIDGET (widget)) > 1)

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
  NONE,
  CREATING_RESIZING_R,
  CREATING_MOVING,
  RESIZING_L,
  RESIZING_L_LOOP,
  RESIZING_L_FADE,
  RESIZING_R,
  RESIZING_R_LOOP,
  RESIZING_R_FADE,
  RESIZING_UP,
  RESIZING_UP_FADE_IN,
  RESIZING_UP_FADE_OUT,
  STRETCHING_L,
  STRETCHING_R,

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
  MOVING_COPY,
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

  STARTING_PANNING,
  PANNING,
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
  "--INVALID--");

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

void
ui_set_pointer_cursor (GtkWidget * widget);

#define ui_set_pencil_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "edit-cursor", 2, 3);

#define ui_set_brush_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "brush-cursor", 2, 3);

#define ui_set_cut_clip_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "cut-cursor", 9, 7);

#define ui_set_eraser_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "eraser-cursor", 4, 2);

#define ui_set_line_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "ramp-cursor", 2, 3);

#define ui_set_speaker_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "audition-cursor", 10, 12);

#define ui_set_hand_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "move-cursor", 12, 11);

#define ui_set_left_resize_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "w-resize-cursor", 14, 11);

#define ui_set_left_stretch_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), "w-stretch-cursor", 14, 11);

#define ui_set_left_resize_loop_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "w-loop-cursor", 14, 11);

#define ui_set_right_resize_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "e-resize-cursor", 10, 11);

#define ui_set_right_stretch_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), "e-stretch-cursor", 10, 11);

#define ui_set_right_resize_loop_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "e-loop-cursor", 10, 11);

#define ui_set_time_select_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), "time-select-cursor", 10, 12);

#define ui_set_fade_in_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "fade-in-cursor", 3, 1);

#define ui_set_fade_out_cursor(widget) \
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "fade-out-cursor", 3, 1);

/**
 * Sets cursor from icon name.
 */
void
ui_set_cursor_from_icon_name (
  GtkWidget *        widget,
  const std::string &name,
  int                offset_x,
  int                offset_y);

/**
 * Sets cursor from standard cursor name.
 */
void
ui_set_cursor_from_name (GtkWidget * widget, const char * name);

gboolean
ui_on_motion_set_status_bar_text_cb (
  GtkWidget * widget,
  GdkEvent *  event,
  char *      text);

/**
 * Shows a popup message of the given type with the given message.
 *
 * @note Only works for non-markup. use AdwMessageDialog directly to show
 * Pango markup.
 *
 * @return The message window.
 */
AdwDialog *
ui_show_message_full (
  GtkWidget *  parent,
  const char * title,
  const char * format,
  ...) G_GNUC_PRINTF (3, 4);

#define UI_ACTIVE_WINDOW_OR_NULL \
  (gtk_application_get_active_window (GTK_APPLICATION (zrythm_app.get ())) \
     ? GTK_WIDGET (gtk_application_get_active_window ( \
         GTK_APPLICATION (zrythm_app.get ()))) \
     : nullptr)

/**
 * Type can be GTK_MESSAGE_ERROR, etc.
 */
#define ui_show_message_printf(title, fmt, ...) \
  ui_show_message_full ( \
    GTK_WIDGET (UI_ACTIVE_WINDOW_OR_NULL), title, fmt, __VA_ARGS__)

/**
 * Type can be GTK_MESSAGE_ERROR, etc.
 */
#define ui_show_message_literal(title, str) \
  ui_show_message_full (GTK_WIDGET (UI_ACTIVE_WINDOW_OR_NULL), title, "%s", str)

/**
 * Wrapper to show error message so that no casting
 * of the window is needed on the caller side.
 */
#define ui_show_error_message_printf(title, fmt, ...) \
  ui_show_message_printf (title, fmt, __VA_ARGS__);

#define ui_show_error_message(title, msg) ui_show_message_literal (title, msg)

/**
 * Returns if @ref rect is hit or not by the
 * given coordinate.
 *
 * @param check_x Check x-axis for match.
 * @param check_y Check y-axis for match.
 * @param x x in parent space.
 * @param y y in parent space.
 * @param x_padding Padding to add to the x
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 * @param y_padding Padding to add to the y
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 */
bool
ui_is_point_in_rect_hit (
  GdkRectangle * rect,
  const bool     check_x,
  const bool     check_y,
  double         x,
  double         y,
  double         x_padding,
  double         y_padding);

/**
 * Returns if the child is hit or not by the
 * coordinates in parent.
 *
 * @param check_x Check x-axis for match.
 * @param check_y Check y-axis for match.
 * @param x x in parent space.
 * @param y y in parent space.
 * @param x_padding Padding to add to the x
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 * @param y_padding Padding to add to the y
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 */
NONNULL int
ui_is_child_hit (
  GtkWidget *  parent,
  GtkWidget *  child,
  const int    check_x,
  const int    check_y,
  const double x,
  const double y,
  const double x_padding,
  const double y_padding);

/**
 * Returns the matching hit child, or NULL.
 *
 * @param x X in parent space.
 * @param y Y in parent space.
 * @param type Type to look for.
 */
GtkWidget *
ui_get_hit_child (GtkWidget * parent, double x, double y, GType type);

UiDetail
ui_get_detail_level (void);

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be clamped at 0. If a
 * negative is needed, pass the abs to this function and then change the sign.
 *
 * @param has_padding Whether @ref px contains padding.
 */
Position
ui_px_to_pos_timeline (double px, bool has_padding);

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 *
 * @param has_padding Whether then given px contains padding.
 */
signed_frame_t
ui_px_to_frames_timeline (double px, bool has_padding);

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 *
 * @param has_padding Whether then given px contains padding.
 */
signed_frame_t
ui_px_to_frames_editor (double px, bool has_padding);

/**
 * Converts position to px, optionally adding the
 * ruler padding.
 */
NONNULL int
ui_pos_to_px_timeline (const Position pos, int use_padding);

/**
 * Converts position to px, optionally adding the ruler
 * padding.
 */
NONNULL int
ui_pos_to_px_editor (const Position pos, bool use_padding);

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will
 * be clamped at 0. If a negative is needed, pass
 * the abs to this function and then change the
 * sign.
 *
 * @param has_padding Whether @ref px contains
 *   padding.
 */
Position
ui_px_to_pos_editor (double px, bool has_padding);

/**
 * Shows a notification in the revealer.
 */
void
ui_show_notification (const char * msg);

/**
 * Show notification from non-GTK threads.
 *
 * This should be used internally. Use the
 * ui_show_notification_idle macro instead.
 */
int
ui_show_notification_idle_func (char * msg);

/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_combo_row (AdwComboRow * combo_row);

/**
 * Generates a combo row for selecting the audio backend.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
AdwComboRow *
ui_gen_audio_backends_combo_row (bool with_signal);

/**
 * Generates a combo row for selecting the MIDI backend.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
AdwComboRow *
ui_gen_midi_backends_combo_row (bool with_signal);

/**
 * Sets up a combo row for selecting the audio device name.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
void
ui_setup_audio_device_name_combo_row (
  AdwComboRow * combo_row,
  bool          populate,
  bool          with_signal);

/**
 * Returns the "a locale for the language you have
 * selected..." text based on the given language.
 *
 * Must be free'd by caller.
 */
char *
ui_get_locale_not_available_string (LocalizationLanguage lang);

void
ui_show_warning_for_tempo_track_experimental_feature (void);

/**
 * Returns if the 2 rectangles overlay.
 */
NONNULL static inline bool
ui_rectangle_overlap (
  const GdkRectangle * const rect1,
  const GdkRectangle * const rect2)
{
  /* if one rect is on the side of the other */
  if (rect1->x > rect2->x + rect2->width || rect2->x > rect1->x + rect1->width)
    return false;

  /* if one rect is above the other */
  if (rect1->y > rect2->y + rect2->height || rect2->y > rect1->y + rect1->height)
    return false;

  return true;
}

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
void
ui_get_db_value_as_string (float val, char * buf);

/**
 * @}
 */

#endif
