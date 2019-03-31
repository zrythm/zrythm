/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * User Interface utils.
 *
 */

#ifndef __UTILS_UI_H__
#define __UTILS_UI_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct Position Position;

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

/**
 * Drag n Drop related.
 */
/* Gtk target entries */
/* Plugin descriptor, used to instantiate plugins */
#define TARGET_ENTRY_PLUGIN_DESCR "PLUGIN_DESCR"
#define TARGET_ENTRY_ID_PLUGIN_DESCR 0
/* Plugin ID, used to move/copy plugins */
#define TARGET_ENTRY_PLUGIN "PLUGIN"
#define TARGET_ENTRY_ID_PLUGIN 1
/* File descriptor */
#define TARGET_ENTRY_FILE_DESCR "FILE_DESCR"
#define TARGET_ENTRY_ID_FILE_DESCR 2
/* */
#define TARGET_ENTRY_TL_SELECTIONS \
  "TL_SELECTIONS"
#define TARGET_ENTRY_ID_TL_SELECTIONS 3

#define GET_ATOM(x) \
  gdk_atom_intern (x, 1)

#define ui_add_widget_tooltip(widget,txt) \
  gtk_widget_set_tooltip_text ( \
    GTK_WIDGET (widget), txt)

#define ui_set_hover_status_bar_signals(w,t) \
  g_signal_connect ( \
    G_OBJECT (w), "enter-notify-event", \
    G_CALLBACK ( \
      ui_on_motion_set_status_bar_text_cb), \
    g_strdup (t)); \
  g_signal_connect ( \
    G_OBJECT(w), "leave-notify-event", \
    G_CALLBACK ( \
      ui_on_motion_set_status_bar_text_cb), \
    g_strdup (t));

/**
 * Shows the notification when idle.
 *
 * This should be called from threads other than GTK main
 * thread.
 */
#define ui_show_notification_idle(msg) \
  char * text = g_strdup (msg); \
  g_message (msg); \
  g_idle_add ((GSourceFunc) ui_show_notification_idle_func, \
              (void *) text)

/**
 * Wrapper to show error message so that no casting
 * of the window is needed on the caller side.
 */
#define ui_show_error_message(win, msg) \
    ui_show_error_message_full ( \
      GTK_WINDOW (win), \
      msg);

#define ui_is_widget_revealed(widget) \
  (gtk_widget_get_allocated_height ( \
     GTK_WIDGET (widget)) > 1 || \
   gtk_widget_get_allocated_width ( \
     GTK_WIDGET (widget)) > 1)

/**
 * Used in handlers to get the state mask.
 */
#define UI_GET_STATE_MASK(gesture) \
  GdkEventSequence * _sequence = \
    gtk_gesture_single_get_current_sequence ( \
      GTK_GESTURE_SINGLE (gesture)); \
  const GdkEvent * _event = \
    gtk_gesture_get_last_event ( \
      GTK_GESTURE (gesture), _sequence); \
  GdkModifierType state_mask; \
  gdk_event_get_state (_event, &state_mask)

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
  UI_OVERLAY_ACTION_RESIZING_L,
  UI_OVERLAY_ACTION_RESIZING_R,
  UI_OVERLAY_ACTION_RESIZING_UP,
  UI_OVERLAY_ACTION_STRETCHING_L,
  UI_OVERLAY_ACTION_STRETCHING_R,

  UI_OVERLAY_ACTION_AUDITIONING,

  /** Auto-filling in edit mode. */
  UI_OVERLAY_ACTION_AUTOFILLING,

  /** Erasing. */
  UI_OVERLAY_ACTION_ERASING,

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
} UiOverlayAction;

#define ui_set_pointer_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-edit-select", \
    3, 6);

#define ui_set_pencil_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-editor", \
    3, 6);

#define ui_set_eraser_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-draw-eraser", \
    3, 6);

#define ui_set_line_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-draw-line", \
    3, 6);

#define ui_set_speaker_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-audio-speakers-symbolic", \
    3, 6);

#define ui_set_hand_cursor(widget) \
  ui_set_cursor_from_icon_name ( \
    GTK_WIDGET (widget), \
    "z-hand", \
    3, 6);

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
ui_set_cursor_from_name (GtkWidget * widget, char * name);

gboolean
ui_on_motion_set_status_bar_text_cb (
  GtkWidget * widget,
  GdkEvent *  event,
  char *      text);

void
ui_show_error_message_full (
  GtkWindow * parent_window,
  const char * message);

/**
 * Returns if the child is hit or not by the coordinates in
 * parent.
 */
int
ui_is_child_hit (GtkContainer * parent,
                 GtkWidget *    child,
                 double         x, ///< x in parent space
                 double         y); ///< y in parent space

/**
 * Returns the matching hit child, or NULL.
 */
GtkWidget *
ui_get_hit_child (GtkContainer * parent,
                  double         x, ///< x in parent space
                  double         y, ///< y in parent space
                  GType          type); ///< type to look for

/**
 * Converts from pixels to position.
 */
void
ui_px_to_pos_timeline (double               px,
           Position *        pos,
           int               has_padding); ///< whether the given px contain padding

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 */
long
ui_px_to_frames_timeline (double   px,
                 int   has_padding); ///< whether the given px contain padding

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 */
long
ui_px_to_frames_piano_roll (double   px,
                 int   has_padding); ///< whether the given px contain padding

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 */
long
ui_px_to_frames_audio_clip_editor (double   px,
                 int   has_padding); ///< whether the given px contain padding

/**
 * Converts position to px, optionally adding the ruler
 * padding.
 */
int
ui_pos_to_px_timeline (Position *       pos,
           int              use_padding);

/**
 * Converts position to px, optionally adding the ruler
 * padding.
 */
int
ui_pos_to_px_piano_roll (Position *       pos,
           int              use_padding);

/**
 * Converts position to px, optionally adding the ruler
 * padding.
 */
int
ui_pos_to_px_audio_clip_editor (
  Position *       pos,
  int              use_padding);

/**
 * Converts from pixels to position.
 */
void
ui_px_to_pos_piano_roll (double               px,
           Position *        pos,
           int               has_padding); ///< whether the given px contain padding

/**
 * Converts from pixels to position.
 */
void
ui_px_to_pos_audio_clip_editor (
  double               px,
  Position *        pos,
  int               has_padding); ///< whether the given px contain padding


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
 * Returns the modifier type (state mask) from the
 * given gesture.
 */
void
ui_get_modifier_type_from_gesture (
  GtkGestureSingle * gesture,
  GdkModifierType *  state_mask); ///< return value

/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_combo_box (
  GtkComboBox * language);

/**
 * @}
 */

#endif
