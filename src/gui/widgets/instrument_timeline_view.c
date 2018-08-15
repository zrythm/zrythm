/*
 * gui/instrument_timeline_view.cpp - The view of an instrument left of its
 *   timeline counterpart
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

#include <dazzle.h>
#include <gtk/gtk.h>

/**
 * Adds an instrument timeline view based on the given instrument
 * TODO: implement instrument struct
 * FIXME: should be a widget
 */
void
set_instrument_timeline_view (GtkWidget * container)
{
  GtkWidget * flowbox = gtk_flow_box_new ();
  GtkWidget * spinbutton = gtk_button_new_with_label ("test");
  gtk_flow_box_insert (flowbox,
                       spinbutton,
                       -1);
  gtk_container_add (GTK_CONTAINER (container),
                     flowbox);
  gtk_widget_set_halign( GTK_WIDGET (spinbutton),
                         GTK_ALIGN_START);
  gtk_widget_set_valign( GTK_WIDGET (spinbutton),
                         GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (container),
                     flowbox);
}
