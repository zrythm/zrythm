/*
 * gui/widgets/snap_grid.c - Snap & grid selection widget
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

#include "project/snap_grid.h"
#include "gui/widgets/snap_grid.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SnapGridWidget, snap_grid_widget, GTK_TYPE_MENU_BUTTON)

static void
on_clicked (GtkButton * button,
            gpointer  user_data)
{
  SnapGridWidget * self = SNAP_GRID_WIDGET (user_data);
  gtk_widget_show_all (GTK_WIDGET (self->popover));
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
SnapGridWidget *
snap_grid_widget_new (SnapGrid * snap_grid)
{
  SnapGridWidget * self = g_object_new (SNAP_GRID_WIDGET_TYPE, NULL);

  self->snap_grid = snap_grid;
  self->popover = GTK_POPOVER (gtk_popover_new (GTK_WIDGET (self)));

  char * string = snap_grid_stringize (snap_grid);
  g_message (string);
  self->label = GTK_LABEL (gtk_label_new (string));
  g_free (string);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->label));
  gtk_container_add (GTK_CONTAINER (self->popover),
                     gtk_label_new ("aa"));
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self),
                               GTK_WIDGET (self->popover));
  g_signal_connect (G_OBJECT (self),
                    "clicked",
                    G_CALLBACK (on_clicked),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));

  return self;
}

static void
snap_grid_widget_class_init (SnapGridWidgetClass * klass)
{
}

static void
snap_grid_widget_init (SnapGridWidget * self)
{
}

