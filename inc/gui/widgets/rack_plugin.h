/*
 * gui/widgets/rack_plugin.h - A rack_plugin widget
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

#ifndef __GUI_WIDGETS_RACK_PLUGIN_H__
#define __GUI_WIDGETS_RACK_PLUGIN_H__

#include "gui/widgets/rack_plugin.h"

#include <gtk/gtk.h>

#define RACK_PLUGIN_WIDGET_TYPE                  (rack_plugin_widget_get_type ())
#define RACK_PLUGIN_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RACK_PLUGIN_WIDGET_TYPE, RackPluginWidget))
#define RACK_PLUGIN_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), RACK_PLUGIN_WIDGET, RackPluginWidgetClass))
#define IS_RACK_PLUGIN_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RACK_PLUGIN_WIDGET_TYPE))
#define IS_RACK_PLUGIN_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), RACK_PLUGIN_WIDGET_TYPE))
#define RACK_PLUGIN_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), RACK_PLUGIN_WIDGET_TYPE, RackPluginWidgetClass))

typedef struct ControlWidget ControlWidget;
typedef struct Plugin Plugin;

typedef struct RackPluginWidget
{
  GtkGrid                   parent_instance;
  GtkLabel *                name;
  GtkButton *               power_button;
  GtkSearchEntry *          search_entry;
  GtkViewport *             in_viewport;
  GtkBox *                  in_box;
  ControlWidget *           in_controls[800];
  int                       num_in_controls;
  GtkViewport *             out_viewport;
  GtkBox *                  out_box;
  ControlWidget *           out_controls[800];
  int                       num_out_controls;
  GtkToggleButton *         toggle;
  Plugin *                  plugin; ///< the plugin
} RackPluginWidget;

typedef struct RackPluginWidgetClass
{
  GtkGridClass       parent_class;
} RackPluginWidgetClass;


/**
 * Creates a rack_plugin widget using the given rack_plugin data.
 */
RackPluginWidget *
rack_plugin_widget_new (Plugin * plugin);

#endif


