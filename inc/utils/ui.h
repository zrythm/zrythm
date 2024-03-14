// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * User Interface utils.
 *
 */

#ifndef __UTILS_UI_H__
#define __UTILS_UI_H__

#include <stdbool.h>

#include "utils/localization.h"
#include "utils/types.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

typedef struct Position Position;
typedef struct Port     Port;

/**
 * @addtogroup utils
 *
 * @{
 */

#define UI_MAX_CURSORS 400

#define UI_CACHES (zrythm_app->ui_caches)
#define UI_COLORS (&UI_CACHES->colors)

/* copied from css */
#define UI_COLOR_DARK_TEXT "#323232"
#define UI_COLOR_BRIGHT_TEXT "#cdcdcd"
#define UI_COLOR_YELLOW "#F9CA1B"
#define UI_COLOR_PURPLE "#9D3955"
#define UI_COLOR_BUTTON_NORMAL "#343434"
#define UI_COLOR_BUTTON_HOVER "#444444"
#define UI_COLOR_RECORD_CHECKED "#ED2939"
#define UI_COLOR_RECORD_ACTIVE "#FF2400"
#define UI_COLOR_BRIGHT_GREEN "#1DD169"
#define UI_COLOR_DARKISH_GREEN "#19664c"
#define UI_COLOR_DARK_ORANGE "#D68A0C"
#define UI_COLOR_Z_YELLOW "#F9CA1B"
#define UI_COLOR_BRIGHT_ORANGE "#F79616"
#define UI_COLOR_Z_PURPLE "#9D3955"
#define UI_COLOR_MATCHA "#2eb398"
#define UI_COLOR_LIGHT_BLUEISH "#1aa3ffcc"
#define UI_COLOR_PREFADER_SEND "#D21E6D"
#define UI_COLOR_POSTFADER_SEND "#901ed2"
#define UI_COLOR_SOLO_ACTIVE UI_COLOR_MATCHA
#define UI_COLOR_SOLO_CHECKED UI_COLOR_DARKISH_GREEN
#define UI_COLOR_HIGHLIGHT_SCALE_BG "#662266"
#define UI_COLOR_HIGHLIGHT_CHORD_BG "#BB22BB"
#define UI_COLOR_HIGHLIGHT_BASS_BG UI_COLOR_LIGHT_BLUEISH
#define UI_COLOR_HIGHLIGHT_BOTH_BG "#FF22FF"
#define UI_COLOR_HIGHLIGHT_SCALE_FG "#F79616"
#define UI_COLOR_HIGHLIGHT_CHORD_FG UI_COLOR_HIGHLIGHT_SCALE_FG
#define UI_COLOR_HIGHLIGHT_BASS_FG "white"
#define UI_COLOR_HIGHLIGHT_BOTH_FG "white"
#define UI_COLOR_FADER_FILL_END UI_COLOR_Z_YELLOW

#define UI_DELETE_ICON_NAME "z-edit-delete"

static const GdkRGBA UI_COLOR_BLACK = { 0, 0, 0, 1 };

typedef enum UiDetail
{
  UI_DETAIL_HIGH,
  UI_DETAIL_NORMAL,
  UI_DETAIL_LOW,
  UI_DETAIL_ULTRA_LOW,
} UiDetail;

static const char * ui_detail_str[] = {
  N_ ("High"),
  N_ ("Normal"),
  N_ ("Low"),
  N_ ("Ultra Low"),
};

static inline const char *
ui_detail_to_string (UiDetail detail)
{
  return ui_detail_str[detail];
}

/**
 * Commonly used UI colors.
 */
typedef struct UiColors
{
  GdkRGBA dark_text;
  GdkRGBA dark_orange;
  GdkRGBA bright_orange;
  GdkRGBA bright_text;
  GdkRGBA matcha;
  GdkRGBA bright_green;
  GdkRGBA darkish_green;
  GdkRGBA prefader_send;
  GdkRGBA postfader_send;
  GdkRGBA record_active;
  GdkRGBA record_checked;
  GdkRGBA solo_active;
  GdkRGBA solo_checked;
  GdkRGBA fader_fill_start;
  GdkRGBA fader_fill_end;
  GdkRGBA highlight_scale_bg;
  GdkRGBA highlight_chord_bg;
  GdkRGBA highlight_bass_bg;
  GdkRGBA highlight_both_bg;
  GdkRGBA highlight_scale_fg;
  GdkRGBA highlight_chord_fg;
  GdkRGBA highlight_bass_fg;
  GdkRGBA highlight_both_fg;
  GdkRGBA z_yellow;
  GdkRGBA z_purple;
} UiColors;

/**
 * Commonly used UI textures.
 */
typedef struct UiTextures
{
} UiTextures;

/**
 * Specification for a cursor.
 *
 * Used for caching.
 */
typedef struct UiCursor
{
  char        name[400];
  GdkCursor * cursor;
  int         offset_x;
  int         offset_y;
} UiCursor;

/**
 * Caches.
 */
typedef struct UiCaches
{
  UiColors colors;
  // UiTextures    textures;
  UiCursor cursors[UI_MAX_CURSORS];
  int      num_cursors;

  bool     detail_level_set;
  UiDetail detail_level;
} UiCaches;

/**
 * Space on the edges to show resize cursors
 */
#define UI_RESIZE_CURSOR_SPACE 8

/*
 * Drag n Drop related.
 * Gtk target entries.
 */

/** Plugin descriptor, used to instantiate plugins.
 */
#define TARGET_ENTRY_PLUGIN_DESCR "PLUGIN_DESCR"

/** For SupportedFile pointers. */
#define TARGET_ENTRY_SUPPORTED_FILE "SUPPORTED_FILE"

/** Plugin ID, used to move/copy plugins. */
#define TARGET_ENTRY_PLUGIN "PLUGIN"

/* File path. Not used at the moment. */
#define TARGET_ENTRY_FILE_PATH "FILE_PATH"

/** URI list. */
#define TARGET_ENTRY_URI_LIST "text/uri-list"

/**
 * Track target entry.
 *
 * This is just the identifier. The
 * TracklistSelections will be used.
 */
#define TARGET_ENTRY_TRACK "TRACK"

/**
 * Chord descriptor target entry.
 */
#define TARGET_ENTRY_CHORD_DESCR "CHORD_DESCR"

/* */
#define TARGET_ENTRY_TL_SELECTIONS "TL_SELECTIONS"

#define GET_ATOM(x) gdk_atom_intern (x, 1)

#define ui_add_widget_tooltip(widget, txt) \
  gtk_widget_set_tooltip_text (GTK_WIDGET (widget), txt)

#define ui_set_hover_status_bar_signals(w, t) \
  g_signal_connect ( \
    G_OBJECT (w), "enter-notify-event", \
    G_CALLBACK (ui_on_motion_set_status_bar_text_cb), g_strdup (t)); \
  g_signal_connect ( \
    G_OBJECT (w), "leave-notify-event", \
    G_CALLBACK (ui_on_motion_set_status_bar_text_cb), g_strdup (t));

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
typedef enum UiCursorState
{
  UI_CURSOR_STATE_DEFAULT,
  UI_CURSOR_STATE_RESIZE_L,
  UI_CURSOR_STATE_REPEAT_L,
  UI_CURSOR_STATE_RESIZE_R,
  UI_CURSOR_STATE_REPEAT_R,
  UI_CURSOR_STATE_RESIZE_UP,
} UiCursorState;

/**
 * Various overlay actions to be shared.
 */
typedef enum UiOverlayAction
{
  UI_OVERLAY_ACTION_NONE,
  UI_OVERLAY_ACTION_CREATING_RESIZING_R,
  UI_OVERLAY_ACTION_CREATING_MOVING,
  UI_OVERLAY_ACTION_RESIZING_L,
  UI_OVERLAY_ACTION_RESIZING_L_LOOP,
  UI_OVERLAY_ACTION_RESIZING_L_FADE,
  UI_OVERLAY_ACTION_RESIZING_R,
  UI_OVERLAY_ACTION_RESIZING_R_LOOP,
  UI_OVERLAY_ACTION_RESIZING_R_FADE,
  UI_OVERLAY_ACTION_RESIZING_UP,
  UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN,
  UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT,
  UI_OVERLAY_ACTION_STRETCHING_L,
  UI_OVERLAY_ACTION_STRETCHING_R,

  UI_OVERLAY_ACTION_STARTING_AUDITIONING,
  UI_OVERLAY_ACTION_AUDITIONING,

  /**
   * Auto-filling in edit mode.
   *
   * @note This is also used for the pencil tool in
   *   velocity and automation editors.
   */
  UI_OVERLAY_ACTION_AUTOFILLING,

  /** Erasing. */
  UI_OVERLAY_ACTION_ERASING,
  UI_OVERLAY_ACTION_STARTING_ERASING,

  /**
   * To be set in drag_start.
   */
  UI_OVERLAY_ACTION_STARTING_MOVING,
  UI_OVERLAY_ACTION_STARTING_MOVING_COPY,
  UI_OVERLAY_ACTION_STARTING_MOVING_LINK,
  UI_OVERLAY_ACTION_MOVING,
  UI_OVERLAY_ACTION_MOVING_COPY,
  UI_OVERLAY_ACTION_MOVING_LINK,
  UI_OVERLAY_ACTION_STARTING_CHANGING_CURVE,
  UI_OVERLAY_ACTION_CHANGING_CURVE,

  /**
   * To be set in drag_start.
   *
   * Useful to check if nothing was clicked.
   */
  UI_OVERLAY_ACTION_STARTING_SELECTION,
  UI_OVERLAY_ACTION_SELECTING,

  /** Like selecting but it auto deletes whatever
   * touches the selection. */
  UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION,
  UI_OVERLAY_ACTION_DELETE_SELECTING,

  UI_OVERLAY_ACTION_STARTING_RAMP,
  UI_OVERLAY_ACTION_RAMPING,
  UI_OVERLAY_ACTION_CUTTING,

  UI_OVERLAY_ACTION_RENAMING,

  UI_OVERLAY_ACTION_STARTING_PANNING,
  UI_OVERLAY_ACTION_PANNING,
  NUM_UI_OVERLAY_ACTIONS,
} UiOverlayAction;

/**
 * Various overlay actions to be shared.
 */
static const char * ui_overlay_strings[] = {
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
  "--INVALID--",
};

static inline const char *
ui_get_overlay_action_string (UiOverlayAction action)
{
  return ui_overlay_strings[action];
}

/**
 * Dragging modes for widgets that have click&drag.
 */
typedef enum UiDragMode
{
  /** Value is wherever the cursor is. */
  UI_DRAG_MODE_CURSOR,

  /** Value is changed based on the offset. */
  UI_DRAG_MODE_RELATIVE,

  /** Value is changed based on the offset, times
   * a multiplier. */
  UI_DRAG_MODE_RELATIVE_WITH_MULTIPLIER,
} UiDragMode;

void
ui_set_pointer_cursor (GtkWidget * widget);

#define ui_set_pointer_stretch_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), "edit-select-stretch", 3, 1);

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
  GtkWidget *  widget,
  const char * name,
  int          offset_x,
  int          offset_y);

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
  (gtk_application_get_active_window (GTK_APPLICATION (zrythm_app)) \
     ? GTK_WIDGET ( \
       gtk_application_get_active_window (GTK_APPLICATION (zrythm_app))) \
     : NULL)

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
 * Returns if \ref rect is hit or not by the
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
 * Only works with positive numbers. Negatives will
 * be clamped at 0. If a negative is needed, pass
 * the abs to this function and then change the
 * sign.
 *
 * @param has_padding Whether @ref px contains
 *   padding.
 */
NONNULL void
ui_px_to_pos_timeline (double px, Position * pos, bool has_padding);

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
ui_pos_to_px_timeline (const Position * pos, int use_padding);

/**
 * Converts position to px, optionally adding the ruler
 * padding.
 */
NONNULL int
ui_pos_to_px_editor (const Position * pos, bool use_padding);

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
NONNULL void
ui_px_to_pos_editor (double px, Position * pos, bool has_padding);

/**
 * Converts RGB to hex string.
 */
void
ui_rgb_to_hex (double red, double green, double blue, char * buf);

void
ui_gdk_rgba_to_hex (GdkRGBA * color, char * buf);

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
 * Sets up the VST paths entry.
 */
void
ui_setup_vst_paths_entry (GtkEntry * entry);

/**
 * Updates the the VST paths in the gsettings from
 * the text in the entry.
 */
void
ui_update_vst_paths_from_entry (GtkEntry * entry);

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
 * Returns the contrasting color (variation of
 * black or white) based on if the given color is
 * dark enough or not.
 *
 * @param src The source color.
 * @param dest The destination color to write to.
 */
void
ui_get_contrast_color (GdkRGBA * src, GdkRGBA * dest);

/**
 * Returns the color in-between two colors.
 *
 * @param transition How far to transition (0.5 for
 *   half).
 */
void
ui_get_mid_color (
  GdkRGBA *       dest,
  const GdkRGBA * c1,
  const GdkRGBA * c2,
  const float     transition);

/**
 * Returns if the 2 rectangles overlay.
 */
NONNULL PURE static inline bool
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
 * Gets the color the widget should be.
 *
 * @param color The original color.
 * @param is_selected Whether the widget is supposed
 *   to be selected or not.
 */
void
ui_get_arranger_object_color (
  GdkRGBA *  color,
  const bool is_hovered,
  const bool is_selected,
  const bool is_transient,
  const bool is_muted);

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

UiCaches *
ui_caches_new (void);

void
ui_caches_free (UiCaches * self);

/**
 * @}
 */

#endif
