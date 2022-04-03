/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Rotated label.
 */

#ifndef __GUI_WIDGETS_ROTATED_LABEL_H__
#define __GUI_WIDGETS_ROTATED_LABEL_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define ROTATED_LABEL_WIDGET_TYPE \
  (rotated_label_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RotatedLabelWidget,
  rotated_label_widget,
  Z,
  ROTATED_LABEL_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Rotated label.
 */
typedef struct _RotatedLabelWidget
{
  GtkWidget parent_instance;

  GtkLabel * lbl;

  float angle;
} RotatedLabelWidget;

void
rotated_label_widget_set_markup (
  RotatedLabelWidget * self,
  const char *         markup);

GtkLabel *
rotated_label_widget_get_label (
  RotatedLabelWidget * self);

void
rotated_label_widget_setup (
  RotatedLabelWidget * self,
  float                angle);

RotatedLabelWidget *
rotated_label_widget_new (float angle);

/**
 * @}
 */

#endif
