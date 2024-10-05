/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Ports expander widget.
 */

#ifndef __GUI_WIDGETS_PORTS_EXPANDER_H__
#define __GUI_WIDGETS_PORTS_EXPANDER_H__

#include "dsp/port.h"
#include "gui/cpp/gtk_widgets/two_col_expander_box.h"

#include "gtk_wrapper.h"

typedef struct _EditableLabelWidget EditableLabelWidget;
class Track;
class Plugin;

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
enum class PortsExpanderTrackPortType
{
  PE_TRACK_PORT_TYPE_CONTROLS,
  PE_TRACK_PORT_TYPE_SENDS,
  PE_TRACK_PORT_TYPE_STEREO_IN,
  PE_TRACK_PORT_TYPE_MIDI_IN,
  PE_TRACK_PORT_TYPE_MIDI_OUT,
};

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
typedef struct _PortsExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  PortFlow                  flow;
  PortType                  type;
  PortIdentifier::OwnerType owner_type;

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
  PortFlow              flow,
  PortType              type,
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
