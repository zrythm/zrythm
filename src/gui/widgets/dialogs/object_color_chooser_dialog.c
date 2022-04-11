/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "project.h"
#include "utils/color.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @param track Track, if track.
 * @param sel TracklistSelections, if multiple
 *   tracks.
 * @param region ZRegion, if region.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  GtkWindow *           parent,
  Track *               track,
  TracklistSelections * sel,
  ZRegion *             region)
{
  GtkColorChooserDialog * dialog = NULL;
  if (track)
    {
      char * str = g_strdup_printf (
        _ ("%s color"), track->name);
      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (str, parent));
      gtk_color_chooser_set_rgba (
        GTK_COLOR_CHOOSER (dialog), &track->color);
      gtk_color_chooser_set_use_alpha (
        GTK_COLOR_CHOOSER (dialog), false);
      g_free (str);
    }
  else if (region)
    {
      char * str = g_strdup_printf (
        _ ("%s color"), region->name);
      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (str, parent));
    }
  else if (sel)
    {
      Track * tr = sel->tracks[0];
      g_return_val_if_fail (
        IS_TRACK_AND_NONNULL (tr), false);

      dialog = GTK_COLOR_CHOOSER_DIALOG (
        gtk_color_chooser_dialog_new (
          _ ("Track color"), parent));
      gtk_color_chooser_set_rgba (
        GTK_COLOR_CHOOSER (dialog), &tr->color);
      gtk_color_chooser_set_use_alpha (
        GTK_COLOR_CHOOSER (dialog), false);
    }

  g_return_val_if_fail (dialog != NULL, false);

  gtk_widget_add_css_class (
    GTK_WIDGET (dialog),
    "object-color-chooser-dialog");

  int res =
    z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  bool color_set = false;
  switch (res)
    {
    case GTK_RESPONSE_OK:
      color_set = true;
      break;
    default:
      break;
    }

  /* get selected color */
  GdkRGBA sel_color;
  gtk_color_chooser_get_rgba (
    GTK_COLOR_CHOOSER (dialog), &sel_color);

  if (color_set)
    {
      if (track)
        {
          /* get current object color */
          GdkRGBA cur_color = track->color;

          /* if changed, apply the change */
          if (!color_is_same (
                &sel_color, &cur_color))
            {
              track_set_color (
                track, &sel_color, F_UNDOABLE,
                F_PUBLISH_EVENTS);
            }
        }
      else if (sel)
        {
          GError * err = NULL;
          bool     ret =
            tracklist_selections_action_perform_edit_color (
              sel, &sel_color, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to change color"));
            }
        }
    }

  gtk_window_destroy (GTK_WINDOW (dialog));

  return color_set;
}
