/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/expander_box.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ExpanderBoxWidget,
               expander_box_widget,
               GTK_TYPE_BOX)

void
on_clicked (GtkButton *button,
            ExpanderBoxWidget * self)
{
  gtk_revealer_set_reveal_child (
    self->revealer,
    !gtk_revealer_get_reveal_child (self->revealer));
}

static void
expander_box_widget_class_init (
  ExpanderBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "expander_box.ui");
  gtk_widget_class_set_css_name (
    klass, "expander-box");

  gtk_widget_class_bind_template_child (
    klass,
    ExpanderBoxWidget,
    button);
  gtk_widget_class_bind_template_child (
    klass,
    ExpanderBoxWidget,
    revealer);
  gtk_widget_class_bind_template_child (
    klass,
    ExpanderBoxWidget,
    content);
}

static void
expander_box_widget_init (ExpanderBoxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->btn_label =
    GTK_LABEL (gtk_label_new ("Label"));
  self->btn_img =
    GTK_IMAGE (
      gtk_image_new_from_icon_name (
        "z-plugins", GTK_ICON_SIZE_BUTTON));
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                 2);
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->btn_label));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (
      gtk_separator_new (GTK_ORIENTATION_VERTICAL)));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->btn_img));
  gtk_container_add (
    GTK_CONTAINER (self->button),
    GTK_WIDGET (box));

  gtk_widget_show_all (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->button), "clicked",
    G_CALLBACK (on_clicked), self);
}
