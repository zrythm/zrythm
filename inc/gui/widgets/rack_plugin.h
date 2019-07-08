/*
 * gui/widgets/rack_plugin.h - A rack_plugin widget
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

#ifndef __GUI_WIDGETS_RACK_PLUGIN_H__
#define __GUI_WIDGETS_RACK_PLUGIN_H__

#include "gui/widgets/rack_plugin.h"

#include <gtk/gtk.h>

#define RACK_PLUGIN_WIDGET_TYPE \
  (rack_plugin_widget_get_type ())
G_DECLARE_FINAL_TYPE (RackPluginWidget,
                      rack_plugin_widget,
                      Z,
                      RACK_PLUGIN_WIDGET,
                      GtkGrid)

typedef struct _ControlWidget ControlWidget;
typedef struct Plugin Plugin;

typedef struct _RackPluginWidget
{
  GtkGrid                   parent_instance;
  GtkLabel *                name;
  GtkToggleButton *         power;
  GtkSearchEntry *          search_entry;
  GtkViewport *             in_viewport;
  GtkBox *                  in_box;
  ControlWidget *           in_controls[800];
  int                       num_in_controls;
  GtkViewport *             out_viewport;
  GtkBox *                  out_box;
  ControlWidget *           out_controls[800];
  int                       num_out_controls;
  GtkToggleButton *         automate;
  Plugin *                  plugin; ///< the plugin
} RackPluginWidget;

/**
 * Creates a rack_plugin widget using the given rack_plugin data.
 */
RackPluginWidget *
rack_plugin_widget_new (Plugin * plugin);

#endif


