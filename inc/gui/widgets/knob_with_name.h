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

#ifndef __GUI_WIDGETS_KNOB_WITH_NAME_H__
#define __GUI_WIDGETS_KNOB_WITH_NAME_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define KNOB_WITH_NAME_WIDGET_TYPE \
  (knob_with_name_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  KnobWithNameWidget,
  knob_with_name_widget,
  Z,
  KNOB_WITH_NAME_WIDGET,
  GtkBox)

typedef struct _KnobWidget KnobWidget;
typedef struct _EditableLabelWidget
  EditableLabelWidget;

/**
* A vertical box with a knob at the top and a name
* at the bottom.
*/
typedef struct _KnobWithNameWidget
{
  GtkBox parent_instance;

  /** The label to show below the knob. */
  //GtkLabel *     label;
  EditableLabelWidget * label;

  /** The knob. */
  KnobWidget * knob;
} KnobWithNameWidget;

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
  int                 spacing);

#endif
