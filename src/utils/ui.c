/*
 * utils/ui.c - GTK/GDK utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/ui.h"

void
ui_set_cursor (GtkWidget * widget, char * name)
{
  GdkWindow * win = gtk_widget_get_parent_window (widget);
  GdkCursor * cursor = gdk_cursor_new_from_name (
                                gdk_display_get_default (),
                                name);
  gdk_window_set_cursor(win, cursor);
}

/**
 * Shows error popup.
 */
void
ui_show_error_message (GtkWindow * parent_window,
                       const char * message)
{
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog = gtk_message_dialog_new (parent_window,
                                   flags,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/**
 * Returns the matching hit child, or NULL.
 */
GtkWidget *
ui_get_hit_child (GtkContainer * parent,
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
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
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

/**
 * Converts from pixels to position.
 */
void
ui_px_to_pos (int               px,
           Position *        pos,
           int               has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return;

  RULER_WIDGET_GET_PRIVATE (MW_RULER)

  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0)
        px = 0;
    }

  pos->bars = px / rw_prv->px_per_bar + 1;
  px = px % (int) round (rw_prv->px_per_bar);
  pos->beats = px / rw_prv->px_per_beat + 1;
  px = px % (int) round (rw_prv->px_per_beat);
  pos->sixteenths = px / rw_prv->px_per_sixteenth + 1;
  px = px % (int) round (rw_prv->px_per_sixteenth);
  pos->ticks = px / rw_prv->px_per_tick;
}

int
ui_pos_to_px (Position *       pos,
           int              use_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  RULER_WIDGET_GET_PRIVATE (MW_RULER)

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
 * Converts from pixels to frames.
 */
long
ui_px_to_frames (int   px,
                 int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  RULER_WIDGET_GET_PRIVATE (MW_RULER)

  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0)
        px = 0;
    }

  return (AUDIO_ENGINE->frames_per_tick * px) /
    rw_prv->px_per_tick;
}
