/*
 * gui/widgets/rack.c - Manages plugins
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

#include "gui/widgets/rack.h"
#include "gui/widgets/rack_row.h"
#include "utils/gtk.h"

G_DEFINE_TYPE (RackWidget,
               rack_widget,
               GTK_TYPE_SCROLLED_WINDOW)

static void
rack_widget_init (RackWidget * self)
{
  self->main_box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_valign (GTK_WIDGET (self->main_box), GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->main_box));

  /* pack the automators row */
  self->rack_rows[0] = rack_row_widget_new (RRW_TYPE_AUTOMATORS,
                                            NULL);
  gtk_box_pack_start (GTK_BOX (self->main_box),
                      GTK_WIDGET (self->rack_rows[0]),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      0);
  self->rack_rows[1] = rack_row_widget_new (RRW_TYPE_AUTOMATORS,
                                            NULL);
  self->num_rack_rows = 2;
  gtk_box_pack_start (GTK_BOX (self->main_box),
                      GTK_WIDGET (self->rack_rows[1]),
                      Z_GTK_EXPAND,
                      Z_GTK_NO_FILL,
                      0);
  self->rack_rows[1] = rack_row_widget_new (RRW_TYPE_AUTOMATORS,
                                            NULL);
  self->num_rack_rows = 2;
  gtk_box_pack_start (GTK_BOX (self->main_box),
                      GTK_WIDGET (self->rack_rows[1]),
                      Z_GTK_EXPAND,
                      Z_GTK_NO_FILL,
                      0);


  /* pack the master channel row */

  /* pack dummy box */
  self->dummy_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_pack_end (GTK_BOX (self->main_box),
                    GTK_WIDGET (self->dummy_box),
                    Z_GTK_EXPAND,
                    Z_GTK_FILL,
                    0);
}

static void
rack_widget_class_init (RackWidgetClass * klass)
{
}

