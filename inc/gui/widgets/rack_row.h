/*
 * gui/widgets/rack_row.h - A rack_row widget
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

#ifndef __GUI_WIDGETS_RACK_ROW_H__
#define __GUI_WIDGETS_RACK_ROW_H__

#include "audio/channel.h"

#include <gtk/gtk.h>

#define RACK_ROW_WIDGET_TYPE \
  (rack_row_widget_get_type ())
G_DECLARE_FINAL_TYPE (RackRowWidget,
                      rack_row_widget,
                      Z,
                      RACK_ROW_WIDGET,
                      GtkBox)

typedef enum RRWType
{
  RRW_TYPE_AUTOMATORS,
  RRW_TYPE_CHANNEL
} RRWType;

typedef struct _AutomatorWidget AutomatorWidget;
typedef struct _RackPluginWidget RackPluginWidget;

typedef struct _RackRowWidget
{
  GtkBox                  parent_instance;
  GtkExpander *           expander;
  GtkLabel *              label;
  GtkBox *                box;
  RRWType                 type;
  AutomatorWidget *       automators[40];
  int                     num_automators;
  RackPluginWidget *      rack_plugins[STRIP_SIZE];
  Channel *               channel; ///< channel, if type channel
} RackRowWidget;

/**
 * Creates a rack_row widget using the given rack_row data.
 */
RackRowWidget *
rack_row_widget_new (RRWType              type,
                     Channel *            channel); ///< channel, if type channel

#endif
