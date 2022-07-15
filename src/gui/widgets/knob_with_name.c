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

#include "gui/widgets/editable_label.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  KnobWithNameWidget,
  knob_with_name_widget,
  GTK_TYPE_BOX)

/**
 * Returns a new instance.
 *
 * @param label_before Whether to show the label
 *   before the knob.
 */
KnobWithNameWidget *
knob_with_name_widget_new (
  void *              obj,
  GenericStringGetter name_getter,
  GenericStringSetter name_setter,
  KnobWidget *        knob,
  GtkOrientation      orientation,
  bool                label_before,
  int                 spacing)
{
  KnobWithNameWidget * self = g_object_new (
    KNOB_WITH_NAME_WIDGET_TYPE, "orientation", orientation,
    "spacing", 2, NULL);

  EditableLabelWidget * label = editable_label_widget_new (
    obj, name_getter, name_setter, -1);

  if (label_before)
    {
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (label));
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (knob));
    }
  else
    {
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (knob));
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (label));
    }

  return self;
}

static void
knob_with_name_widget_class_init (
  KnobWithNameWidgetClass * _klass)
{
}

static void
knob_with_name_widget_init (KnobWithNameWidget * self)
{
}
