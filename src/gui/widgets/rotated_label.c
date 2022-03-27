/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "gui/widgets/rotated_label.h"
#include "utils/gtk.h"
#include "utils/math.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  RotatedLabelWidget,
  rotated_label_widget,
  GTK_TYPE_WIDGET)

static void
queue_changes (
  RotatedLabelWidget * self)
{
  gtk_widget_queue_allocate (GTK_WIDGET (self));
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
rotated_label_widget_set_markup (
  RotatedLabelWidget * self,
  const char *         markup)
{
  gtk_label_set_markup (self->lbl, markup);
  queue_changes (self);
}

GtkLabel *
rotated_label_widget_get_label (
  RotatedLabelWidget * self)
{
  return self->lbl;
}

void
rotated_label_widget_setup (
  RotatedLabelWidget * self,
  float                angle)
{
  self->angle = angle;
  queue_changes (self);
}

RotatedLabelWidget *
rotated_label_widget_new (
  float angle)
{
  RotatedLabelWidget * self =
    g_object_new (
      ROTATED_LABEL_WIDGET_TYPE, NULL);

  rotated_label_widget_setup (self, angle);

  return self;
}

static void
on_measure (
  GtkWidget* widget,
  GtkOrientation orientation,
  int for_size,
  int* min,
  int* nat,
  int* min_baseline,
  int* nat_baseline)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (widget);
       child;
       child = gtk_widget_get_next_sibling (child)) {
    int child_min = 0;
    int child_nat = 0;
    int child_min_baseline = -1;
    int child_nat_baseline = -1;

    if (!gtk_widget_should_layout (child))
      continue;

    gtk_widget_measure (child, !orientation, for_size,
                        &child_min, &child_nat,
                        &child_min_baseline, &child_nat_baseline);

    *min = MAX (*min, child_min);
    *nat = MAX (*nat, child_nat);
  }

  *min_baseline = -1;
  *nat_baseline = -1;
}

static void
on_size_allocate (
  GtkWidget * widget,
  int         width,
  int         height,
  int         baseline)
{
  RotatedLabelWidget * self =
    Z_ROTATED_LABEL_WIDGET (widget);

  /* NOTE this only handles 0 or -90 degrees */
  GskTransform * transform =
    gsk_transform_rotate (NULL, self->angle);
  transform =
    gsk_transform_translate (
      transform,
      &GRAPHENE_POINT_INIT (- height, 0));
  gtk_widget_allocate (
    GTK_WIDGET (self->lbl),
    height, width, -1, transform);
}

static GtkSizeRequestMode
on_get_request_mode (
  GtkWidget * widget)
{
  return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
on_dispose (
  GObject * object)
{
  RotatedLabelWidget * self =
    Z_ROTATED_LABEL_WIDGET (object);
  gtk_widget_unparent (GTK_WIDGET (self->lbl));

  G_OBJECT_CLASS (
    rotated_label_widget_parent_class)->
      dispose (object);
}

static void
rotated_label_widget_init (
  RotatedLabelWidget * self)
{
  self->lbl = GTK_LABEL (gtk_label_new (""));
  gtk_widget_set_parent (
    GTK_WIDGET (self->lbl), GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "rotated-label-parent");
}

static void
rotated_label_widget_class_init (
  RotatedLabelWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  wklass->measure = on_measure;
  wklass->get_request_mode = on_get_request_mode;
  wklass->size_allocate = on_size_allocate;

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = on_dispose;
}
