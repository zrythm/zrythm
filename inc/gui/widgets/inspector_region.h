/*
 * Copyright (C) 2019 Alexandros Theodotou
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

/**
 * \file
 * Widget showing region info in the inspector.
 */

#ifndef __GUI_WIDGETS_INSPECTOR_REGION_H__
#define __GUI_WIDGETS_INSPECTOR_REGION_H__

#include <gtk/gtk.h>

#define INSPECTOR_REGION_WIDGET_TYPE \
  (inspector_region_widget_get_type ())
G_DECLARE_FINAL_TYPE (InspectorRegionWidget,
                      inspector_region_widget,
                      Z,
                      INSPECTOR_REGION_WIDGET,
                      GtkGrid)

typedef struct Region Region;

typedef struct _InspectorRegionWidget
{
  GtkGrid             parent_instance;
  GtkLabel *          header;
  GtkEntry *          name;
  GtkBox *            position_box;
  GtkBox *            length_box;
  GtkColorButton *    color;
  GtkToggleButton *   mute_toggle;
} InspectorRegionWidget;

/**
 * Creates the inspector_region widget.
 *
 * Only once per project.
 */
InspectorRegionWidget *
inspector_region_widget_new ();

void
inspector_region_widget_show_regions (
  InspectorRegionWidget * self,
  Region **               regions,
  int                     num_regions);

#endif
