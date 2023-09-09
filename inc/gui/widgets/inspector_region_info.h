/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_EXPANDER_BOX_H__
#define __GUI_WIDGETS_EXPANDER_BOX_H__

#include <gtk/gtk.h>

#define INSPECTOR_REGION_INFO_WIDGET_TYPE \
  (inspetor_region_info_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorRegionInfoWidget,
  inspector_region_info_widget,
  Z,
  INSPECTOR_REGION_INFO_WIDGET,
  ExpanderBoxWidget)

typedef struct _ExpanderBoxWidget
{
  GtkBox parent_instance;

  GtkButton *   button;
  GtkLabel *    btn_label;
  GtkImage *    btn_img;
  GtkRevealer * revealer;
  GtkBox *      content;
} ExpanderBoxWidget;

/**
 * Sets the label to show.
 */
void
expander_box_widget_set_label (ExpanderBoxWidget * self, const char * label);

/**
 * Sets the icon name to show.
 */
void
expander_box_widget_set_icon_name (
  ExpanderBoxWidget * self,
  const char *        icon_name);

#endif
