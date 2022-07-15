/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/snap_grid.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/snap_grid_popover.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (SnapGridWidget, snap_grid_widget, GTK_TYPE_BOX)

#if 0
static void
on_clicked (GtkButton * button,
            SnapGridWidget * self)
{
  /*gtk_widget_show_all (GTK_WIDGET (self->popover));*/
}
#endif

static void
set_label (SnapGridWidget * self)
{
  SnapGrid * sg = self->snap_grid;
  char *     snap_str = snap_grid_stringize (sg);

  char new_str[600];
  if (sg->length_type == NOTE_LENGTH_LINK)
    {
      sprintf (new_str, "%s - 🔗", snap_str);
    }
  else if (sg->length_type == NOTE_LENGTH_LAST_OBJECT)
    {
      sprintf (new_str, _ ("%s - Last object"), snap_str);
    }
  else
    {
      char * default_str = snap_grid_stringize (sg);
      sprintf (new_str, "%s - %s", snap_str, default_str);
      g_free (default_str);
    }
  gtk_label_set_text (self->label, new_str);

  g_free (snap_str);
}

void
snap_grid_widget_refresh (SnapGridWidget * self)
{
  set_label (self);
}

void
snap_grid_widget_setup (
  SnapGridWidget * self,
  SnapGrid *       snap_grid)
{
  self->snap_grid = snap_grid;
  self->popover = snap_grid_popover_widget_new (self);
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self->menu_btn),
    GTK_WIDGET (self->popover));

  set_label (self);
}

static void
snap_grid_widget_class_init (SnapGridWidgetClass * klass)
{
}

static void
snap_grid_widget_init (SnapGridWidget * self)
{
  self->menu_btn = GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->menu_btn));

  self->box =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->img =
    GTK_IMAGE (gtk_image_new_from_icon_name ("snap-to-grid"));
  self->label = GTK_LABEL (gtk_label_new (""));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->box), _ ("Snap/Grid options"));
  gtk_box_append (self->box, GTK_WIDGET (self->img));
  gtk_box_append (self->box, GTK_WIDGET (self->label));
  gtk_menu_button_set_child (
    GTK_MENU_BUTTON (self->menu_btn), GTK_WIDGET (self->box));
#if 0
  g_signal_connect (
    G_OBJECT (self), "clicked",
    G_CALLBACK (on_clicked), self);
#endif
}
