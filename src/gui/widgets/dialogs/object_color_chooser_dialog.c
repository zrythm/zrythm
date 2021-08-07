/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/flags.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ObjectColorChooserDialogWidget,
  object_color_chooser_dialog_widget,
  GTK_TYPE_COLOR_CHOOSER_DIALOG)

static void
get_current_obj_color (
  ObjectColorChooserDialogWidget * self,
  GdkRGBA *                        color)
{
  if (self->track)
    {
      g_return_if_fail (IS_TRACK (self->track));
      *color = self->track->color;
    }
  else if (self->region)
    {
      /**color = self->region->color;*/
    }
  else
    {
      g_return_if_reached ();
    }
}

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the color was set or not.
 */
bool
object_color_chooser_dialog_widget_run (
  ObjectColorChooserDialogWidget * self)
{
  int res = gtk_dialog_run (GTK_DIALOG (self));
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
    GTK_COLOR_CHOOSER (self), &sel_color);

  if (color_set)
    {
      if (self->track)
        {
          /* get current object color */
          GdkRGBA cur_color;
          get_current_obj_color (self, &cur_color);

          /* if changed, apply the change */
          if (!color_is_same (
                &sel_color, &cur_color))
            {
              track_set_color (
                self->track, &sel_color, F_UNDOABLE,
                F_PUBLISH_EVENTS);
            }
        }
      else if (self->tracklist_selections)
        {
          TracklistSelections * sel =
            self->tracklist_selections;
          tracklist_selections_set_color_with_action (
            sel, &sel_color);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (self));

  return color_set;
}

/**
 * Creates a new dialog.
 */
ObjectColorChooserDialogWidget *
object_color_chooser_dialog_widget_new_for_track (
  Track * track)
{
  g_return_val_if_fail (IS_TRACK (track), NULL);

  char * str =
    g_strdup_printf (_("%s color"), track->name);
  ObjectColorChooserDialogWidget * self =
    g_object_new (
      OBJECT_COLOR_CHOOSER_DIALOG_WIDGET_TYPE,
      "title", str,
      "rgba", &track->color,
      "use-alpha", false,
      NULL);
  g_free (str);

  self->track = track;
  g_warn_if_fail (IS_TRACK (self->track));

  return self;
}

/**
 * Creates a new dialog.
 */
ObjectColorChooserDialogWidget *
object_color_chooser_dialog_widget_new_for_tracklist_selections (
  TracklistSelections * sel)
{
  Track * track = sel->tracks[0];
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);
  ObjectColorChooserDialogWidget * self =
    g_object_new (
      OBJECT_COLOR_CHOOSER_DIALOG_WIDGET_TYPE,
      "title", _("Track color"),
      "rgba", &track->color,
      "use-alpha", false,
      NULL);

  self->tracklist_selections = sel;

  return self;
}

/**
 * Creates a new dialog.
 */
ObjectColorChooserDialogWidget *
object_color_chooser_dialog_widget_new_for_region (
  ZRegion * region)
{
  ObjectColorChooserDialogWidget * self =
    g_object_new (
      OBJECT_COLOR_CHOOSER_DIALOG_WIDGET_TYPE,
      "title", _("Region color"),
      NULL);

  self->region = region;

  return self;
}

static void
object_color_chooser_dialog_widget_class_init (
  ObjectColorChooserDialogWidgetClass * _klass)
{
  /*GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);*/
}

static void
object_color_chooser_dialog_widget_init (
  ObjectColorChooserDialogWidget * self)
{
  z_gtk_widget_add_style_class (
    GTK_WIDGET (self), "object-color-chooser-dialog");
}
