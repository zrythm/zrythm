/*
 * gui/widgets/inspector_region.h - A inspector_region widget
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

#ifndef __GUI_WIDGETS_INSPECTOR_REGION_H__
#define __GUI_WIDGETS_INSPECTOR_REGION_H__

#include <gtk/gtk.h>

#define INSPECTOR_REGION_WIDGET_TYPE                  (inspector_region_widget_get_type ())
#define INSPECTOR_REGION_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), INSPECTOR_REGION_WIDGET_TYPE, InspectorRegionWidget))
#define INSPECTOR_REGION_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), INSPECTOR_REGION_WIDGET, InspectorRegionWidgetClass))
#define IS_INSPECTOR_REGION_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INSPECTOR_REGION_WIDGET_TYPE))
#define IS_INSPECTOR_REGION_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), INSPECTOR_REGION_WIDGET_TYPE))
#define INSPECTOR_REGION_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), INSPECTOR_REGION_WIDGET_TYPE, InspectorRegionWidgetClass))

typedef struct Region Region;

typedef struct InspectorRegionWidget
{
  GtkGrid             parent_instance;
  GtkLabel *          header;
  GtkBox *            position_box;
  GtkBox *            length_box;
  GtkColorButton *    color;
  GtkToggleButton *   mute_toggle;
} InspectorRegionWidget;

typedef struct InspectorRegionWidgetClass
{
  GtkGridClass       parent_class;
} InspectorRegionWidgetClass;

/**
 * Creates the inspector_region widget.
 *
 * Only once per project.
 */
InspectorRegionWidget *
inspector_region_widget_new ();

void
inspector_region_widget_show_regions (InspectorRegionWidget * self,
                                      Region **               regions,
                                      int                     num_regions);

#endif


