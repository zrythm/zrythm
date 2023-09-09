/*
 * gui/widgets/inspector_ap.h - A inspector_ap widget
 *
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

#ifndef __GUI_WIDGETS_INSPECTOR_AP_H__
#define __GUI_WIDGETS_INSPECTOR_AP_H__

#include <gtk/gtk.h>

#define INSPECTOR_AP_WIDGET_TYPE (inspector_ap_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorApWidget,
  inspector_ap_widget,
  Z,
  INSPECTOR_AP_WIDGET,
  GtkGrid)

typedef struct AutomationPoint AutomationPoint;

typedef struct _InspectorApWidget
{
  GtkGrid           parent_instance;
  GtkLabel *        header;
  GtkBox *          position_box;
  GtkBox *          length_box;
  GtkColorButton *  color;
  GtkToggleButton * mute_toggle;
} InspectorApWidget;

void
inspector_ap_widget_show_aps (
  InspectorApWidget * self,
  AutomationPoint **  aps,
  int                 num_aps);

#endif
