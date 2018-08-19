/*
 * gui/instrument_timeline_view.c - The view of an instrument left of its
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

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

static GtkWidget *
create_test_flowbox ()
{
  /* create flowbox */
  GtkWidget * flowbox = gtk_flow_box_new ();
  GtkWidget * spinbutton = gtk_button_new_with_label ("test");
  gtk_widget_set_valign (GTK_WIDGET (spinbutton),
                         GTK_ALIGN_START);
  gtk_widget_set_halign (GTK_WIDGET (spinbutton),
                         GTK_ALIGN_START);
  gtk_flow_box_insert (GTK_FLOW_BOX (flowbox),
                       spinbutton,
                       -1);
  gtk_widget_set_valign (GTK_WIDGET (flowbox),
                         GTK_ALIGN_FILL);
  gtk_widget_set_halign (GTK_WIDGET (flowbox),
                         GTK_ALIGN_FILL);
  gtk_widget_set_vexpand (GTK_WIDGET (flowbox),
                          TRUE);
  GtkStyleContext *context =
          gtk_widget_get_style_context  (flowbox);
  gtk_style_context_add_class (context,
                               "timeline-instrument");
  return flowbox;
}

/**
 * Adds an instrument timeline view based on the given instrument
 * returns the new GtkPaned at the bottom
 * TODO: implement instrument struct
 * FIXME: should be a widget
 */
GtkWidget *
add_instrument (GtkWidget * container)
{
  /* create new gpaned */
  GtkWidget * new_gpaned =
    gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_wide_handle (GTK_PANED (new_gpaned),
                             USE_WIDE_HANDLE);
  gtk_widget_set_valign (new_gpaned,
                         GTK_ALIGN_FILL);

  /* add new instrument flowbox to new gpaned */
  gtk_paned_pack1 (GTK_PANED (new_gpaned),
                   create_test_flowbox (),
                   FALSE,
                   FALSE);

  /* add new last box to new gpaned */
  GtkWidget * last_box =
    gtk_box_new (GTK_ORIENTATION_VERTICAL,
                 0);
  gtk_paned_pack2 (GTK_PANED (new_gpaned),
                   last_box,
                   TRUE,
                   TRUE);

  /* put new gpaned where the box was in the parent */
  gtk_container_remove (GTK_CONTAINER (container),
                        gtk_paned_get_child2 (
                            GTK_PANED (container)));
  gtk_paned_pack2 (GTK_PANED (container),
                   new_gpaned,
                   TRUE,
                   FALSE);

  /* return the new gpaned */
  return new_gpaned;
}

/**
 * adds first instrument
 */
void
init_ins_timeline_view (GtkWidget * container)
{
  gtk_paned_pack1 (GTK_PANED (container),
                   create_test_flowbox (),
                   FALSE,
                   FALSE);

  gtk_paned_set_wide_handle (GTK_PANED (container),
                             USE_WIDE_HANDLE);

  GtkWidget * last_box =
    gtk_box_new (GTK_ORIENTATION_VERTICAL,
                 0);

  gtk_paned_pack2 (GTK_PANED (container),
                   create_test_flowbox (),
                   TRUE,
                   TRUE);
}
