/*
 * gui/widgets/connections.h - Connections widget
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

#ifndef __GUI_WIDGETS_CONNECTIONS_H__
#define __GUI_WIDGETS_CONNECTIONS_H__

#include <gtk/gtk.h>

#define CONNECTIONS_WIDGET_TYPE \
  (connections_widget_get_type ())
G_DECLARE_FINAL_TYPE (ConnectionsWidget,
                      connections_widget,
                      CONNECTIONS,
                      WIDGET,
                      GtkGrid)

#define MW_CONNECTIONS MW_BOT_DOCK_EDGE->connections

typedef struct AudioUnitWidget AudioUnitWidget;

typedef struct _ConnectionsWidget
{
  GtkGrid                  parent_instance;
  GtkDrawingArea *         bg;  ///< overlay background
  GtkComboBoxText *        select_src_cb;
  GtkComboBoxText *        select_dest_cb;
  GtkViewport *            src_au_viewport;
  AudioUnitWidget *        src_au;
  GtkViewport *            dest_au_viewport;
  AudioUnitWidget *        dest_au;
} ConnectionsWidget;

/**
 * Updates the connections widget.
 *
 * Updates the combo boxes.
 */
void
connections_widget_update (ConnectionsWidget * self);

#endif

