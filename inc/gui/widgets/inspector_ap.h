/*
 * gui/widgets/inspector_ap.h - A inspector_ap widget
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

#ifndef __GUI_WIDGETS_INSPECTOR_AP_H__
#define __GUI_WIDGETS_INSPECTOR_AP_H__

#include <gtk/gtk.h>

#define INSPECTOR_AP_WIDGET_TYPE                  (inspector_ap_widget_get_type ())
#define INSPECTOR_AP_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), INSPECTOR_AP_WIDGET_TYPE, InspectorApWidget))
#define INSPECTOR_AP_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), INSPECTOR_AP_WIDGET, InspectorApWidgetClass))
#define IS_INSPECTOR_AP_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INSPECTOR_AP_WIDGET_TYPE))
#define IS_INSPECTOR_AP_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), INSPECTOR_AP_WIDGET_TYPE))
#define INSPECTOR_AP_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), INSPECTOR_AP_WIDGET_TYPE, InspectorApWidgetClass))

typedef struct AutomationPoint AutomationPoint;

typedef struct InspectorApWidget
{
  GtkGrid             parent_instance;
  GtkLabel *          header;
  GtkBox *            position_box;
  GtkBox *            length_box;
  GtkColorButton *    color;
  GtkToggleButton *   muted_toggle;
} InspectorApWidget;

typedef struct InspectorApWidgetClass
{
  GtkGridClass       parent_class;
} InspectorApWidgetClass;

/**
 * Creates the inspector_ap widget.
 *
 * Only once per project.
 */
InspectorApWidget *
inspector_ap_widget_new ();

void
inspector_ap_widget_show_aps (InspectorApWidget * self,
                              AutomationPoint **               aps,
                              int                     num_aps);

#endif




