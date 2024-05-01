/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Ports expander widget.
 */

#ifndef __GUI_WIDGETS_PORTS_EXPANDER_H__
#define __GUI_WIDGETS_PORTS_EXPANDER_H__

#include "dsp/port.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

typedef struct _EditableLabelWidget EditableLabelWidget;
typedef struct Track                Track;
typedef struct Plugin               Plugin;

#define PORTS_EXPANDER_WIDGET_TYPE (ports_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortsExpanderWidget,
  ports_expander_widget,
  Z,
  PORTS_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Used for Track's.
 */
typedef enum PortsExpanderTrackPortType
{
  PE_TRACK_PORT_TYPE_CONTROLS,
  PE_TRACK_PORT_TYPE_SENDS,
  PE_TRACK_PORT_TYPE_STEREO_IN,
  PE_TRACK_PORT_TYPE_MIDI_IN,
  PE_TRACK_PORT_TYPE_MIDI_OUT,
} PortsExpanderTrackPortType;

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
typedef struct _PortsExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  ZPortFlow      flow;
  ZPortType      type;
  ZPortOwnerType owner_type;

  /** Plugin, in case of owner type Plugin. */
  Plugin * plugin;

  /** Track, in case of owner type Track. */
  Track * track;
} PortsExpanderWidget;

/**
 * Refreshes each field.
 */
void
ports_expander_widget_refresh (PortsExpanderWidget * self);

/**
 * Sets up the PortsExpanderWidget for a Plugin.
 */
void
ports_expander_widget_setup_plugin (
  PortsExpanderWidget * self,
  ZPortFlow             flow,
  ZPortType             type,
  Plugin *              pl);

/**
 * Sets up the PortsExpanderWidget for Track ports.
 *
 * @param type The type of ports to include.
 */
void
ports_expander_widget_setup_track (
  PortsExpanderWidget *      self,
  Track *                    tr,
  PortsExpanderTrackPortType type);

/**
 * @}
 */

#endif
