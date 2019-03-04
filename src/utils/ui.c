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

#include <math.h>

#include "audio/engine.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/ui.h"

void
ui_set_cursor (GtkWidget * widget, char * name)
{
  GdkWindow * win =
    gtk_widget_get_parent_window (widget);
  GdkCursor * cursor =
    gdk_cursor_new_from_name (
      gdk_display_get_default (),
      name);
  gdk_window_set_cursor(win, cursor);
}

/**
 * Shows error popup.
 */
void
ui_show_error_message_full (
  GtkWindow * parent_window,
  const char * message)
{
  GtkDialogFlags flags =
    GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_message_dialog_new (parent_window,
      flags,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      "%s",
      message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/**
 * Returns the matching hit child, or NULL.
 */
GtkWidget *
ui_get_hit_child (
  GtkContainer * parent,
  double         x, ///< x in parent space
  double         y, ///< y in parent space
  GType          type) ///< type to look for
{
  GList *children, *iter;

  /* go through each overlay child */
  children =
    gtk_container_get_children (parent);
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        widget,
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (parent),
        GTK_WIDGET (widget),
        x,
        y,
        &wx,
        &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          /* if type matches */
          if (G_TYPE_CHECK_INSTANCE_TYPE (
                widget,
                type))
            {
              return widget;
            }
        }
    }
  return NULL;
}

/**
 * Returns if the child is hit or not by the coordinates in
 * parent.
 */
int
ui_is_child_hit (GtkContainer * parent,
                 GtkWidget *    child,
                 double         x, ///< x in parent space
                 double         y) ///< y in parent space
{
  GtkWidget * widget =
    ui_get_hit_child (parent,
                      x,
                      y,
                      G_OBJECT_TYPE (child));
  if (widget == child)
    return 1;
  else
    return 0;
}

static void
px_to_pos (
  double        px,
  Position *    pos,
  int           use_padding,
  RulerWidget * ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler);

  if (use_padding)
    {
      px -= SPACE_BEFORE_START_D;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  pos->bars = px / rw_prv->px_per_bar + 1;
  px = fmod (px, rw_prv->px_per_bar);
  pos->beats = px / rw_prv->px_per_beat + 1;
  px = fmod (px, rw_prv->px_per_beat);
  pos->sixteenths = px / rw_prv->px_per_sixteenth + 1;
  px = fmod (px, rw_prv->px_per_sixteenth);
  pos->ticks = px / rw_prv->px_per_tick;
}

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_timeline (
  double     px,
  Position * pos,
  int        has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (MW_RULER));
}


/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_piano_roll (double               px,
           Position *        pos,
           int               has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_audio_clip_editor (
  double               px,
  Position *           pos,
  int                  has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (AUDIO_RULER));
}

static int
pos_to_px (
  Position *       pos,
  int              use_padding,
  RulerWidget *    ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler)

  int px =
    (pos->bars - 1) * rw_prv->px_per_bar +
    (pos->beats - 1) * rw_prv->px_per_beat +
    (pos->sixteenths - 1) * rw_prv->px_per_sixteenth +
    pos->ticks * rw_prv->px_per_tick;

  if (use_padding)
    px += SPACE_BEFORE_START;

  return px;
}

/**
 * Gets pixels from the position, based on the
 * timeline ruler.
 */
int
ui_pos_to_px_timeline (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (MW_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_piano_roll (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_audio_clip_editor (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (AUDIO_RULER));
}

static long
px_to_frames (
  double        px,
  int           has_padding, ///< whether the given px contain padding
  RulerWidget * ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler)

  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  return (AUDIO_ENGINE->frames_per_tick * px) /
    rw_prv->px_per_tick;
}

/**
 * Converts from pixels to frames, based on the
 * timeline.
 */
long
ui_px_to_frames_timeline (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (MW_RULER));
}

/**
 * Converts from pixels to frames, based on the
 * piano_roll.
 */
long
ui_px_to_frames_piano_roll (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Converts from pixels to frames, based on the
 * piano_roll.
 */
long
ui_px_to_frames_audio_clip_editor (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (AUDIO_RULER));
}

/**
 * Hides the notification.
 *
 * Used ui_show_notification to be called after
 * a timeout.
 */
static int
hide_notification_async ()
{
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    0);

  return FALSE;
}

/**
 * Shows a notification in the revealer.
 */
void
ui_show_notification (const char * msg)
{
  gtk_label_set_text (MAIN_WINDOW->notification_label,
                      msg);
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    1);
  g_timeout_add_seconds (
    3, (GSourceFunc) hide_notification_async, NULL);
}

/**
 * Show notification from non-GTK threads.
 *
 * This should be used internally. Use the
 * ui_show_notification_idle macro instead.
 */
int
ui_show_notification_idle_func (char * msg)
{
  ui_show_notification (msg);
  g_free (msg);

  return G_SOURCE_REMOVE;
}

/**
 * Returns the modifier type (state mask) from the
 * given gesture.
 */
void
ui_get_modifier_type_from_gesture (
  GtkGestureSingle * gesture,
  GdkModifierType *  state_mask) ///< return value
{
  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      gesture);
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  gdk_event_get_state (event, state_mask);
}
