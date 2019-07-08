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

/** \file */

#ifndef __GUI_WIDGETS_PORTS_EXPANDER_H__
#define __GUI_WIDGETS_PORTS_EXPANDER_H__

#include "audio/port.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

#define PORTS_EXPANDER_WIDGET_TYPE \
  (ports_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortsExpanderWidget,
  ports_expander_widget,
  Z,
  PORTS_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);


typedef struct _EditableLabelWidget
  EditableLabelWidget;
typedef struct Track Track;
typedef struct Plugin Plugin;

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
typedef struct _PortsExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  PortFlow      flow;
  PortType      type;
  PortOwnerType owner_type;

  /** Plugin, in case of owner type Plugin. */
  Plugin *      plugin;

  /** Track, in case of owner type Track. */
  Track *       track;
} PortsExpanderWidget;

/**
 * Refreshes each field.
 */
void
ports_expander_widget_refresh (
  PortsExpanderWidget * self);

/**
 * Sets up the PortsExpanderWidget.
 */
void
ports_expander_widget_setup (
  PortsExpanderWidget * self,
  PortFlow      flow,
  PortType      type,
  PortOwnerType owner_type,
  Plugin *      pl,
  Track *       tr);

#endif
