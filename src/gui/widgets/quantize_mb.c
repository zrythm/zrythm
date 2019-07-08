/*
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "audio/quantize.h"
#include "audio/snap_grid.h"
#include "gui/widgets/quantize_mb.h"
#include "gui/widgets/quantize_mb_popover.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (QuantizeMbWidget,
               quantize_mb_widget,
               GTK_TYPE_MENU_BUTTON)

static void
on_clicked (GtkButton * button,
            gpointer  user_data)
{
  QuantizeMbWidget * self =
    Z_QUANTIZE_MB_WIDGET (user_data);
  gtk_widget_show_all (GTK_WIDGET (self->popover));
}

static void
set_label (QuantizeMbWidget * self)
{
  char * string =
    snap_grid_stringize (
      self->quantize->note_length,
      self->quantize->note_type);
  gtk_label_set_text (self->label, string);
  g_free (string);
}

void
quantize_mb_widget_refresh (
  QuantizeMbWidget * self)
{
  set_label (self);
}

void
quantize_mb_widget_setup (QuantizeMbWidget * self,
                        Quantize * quantize)
{
  self->quantize = quantize;
  self->popover = quantize_mb_popover_widget_new (self);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self),
                               GTK_WIDGET (self->popover));

  set_label (self);
}

static void
quantize_mb_widget_class_init (QuantizeMbWidgetClass * klass)
{
}

static void
quantize_mb_widget_init (QuantizeMbWidget * self)
{
  self->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->img = GTK_IMAGE (
    resources_get_icon (ICON_TYPE_GNOME_BUILDER,
                        "completion-snippet-symbolic-light.svg"));
  self->label = GTK_LABEL (gtk_label_new ("Quantize"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->box),
                               "Quantize options");
  gtk_box_pack_start (self->box,
                      GTK_WIDGET (self->img),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      1);
  gtk_box_pack_end (self->box,
                    GTK_WIDGET (self->label),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->box));
  g_signal_connect (G_OBJECT (self),
                    "clicked",
                    G_CALLBACK (on_clicked),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));
}
