// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Inspector section for tracks.
 */

#ifndef __GUI_WIDGETS_INSPECTOR_TRACK_H__
#define __GUI_WIDGETS_INSPECTOR_TRACK_H__

#include "common/utils/types.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define INSPECTOR_TRACK_WIDGET_TYPE (inspector_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorTrackWidget,
  inspector_track_widget,
  Z,
  INSPECTOR_TRACK_WIDGET,
  GtkWidget)

class TracklistSelections;
TYPEDEF_STRUCT_UNDERSCORED (TrackPropertiesExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (PortsExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (TrackInputExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (PluginStripExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (FaderControlsExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (TextExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (ColorAreaWidget);
TYPEDEF_STRUCT_UNDERSCORED (ChannelSendsExpanderWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACK_INSPECTOR MW_LEFT_DOCK_EDGE->track_inspector

/**
 * Inspector section for tracks.
 */
using InspectorTrackWidget = struct _InspectorTrackWidget
{
  GtkWidget                       parent_instance;
  TrackPropertiesExpanderWidget * track_info;

  TrackInputExpanderWidget * inputs;

  ChannelSendsExpanderWidget * sends;

  PortsExpanderWidget * outputs;

  /** Pan, fader, etc. */
  PortsExpanderWidget * controls;

  PluginStripExpanderWidget * inserts;

  PluginStripExpanderWidget * midi_fx;

  FaderControlsExpanderWidget * fader;
  TextExpanderWidget *          comment;

  ColorAreaWidget * color;
};

/**
 * Shows the inspector page for the given tracklist
 * selection.
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the track page.
 */
ATTR_NONNULL void
inspector_track_widget_show_tracks (
  InspectorTrackWidget *      self,
  SimpleTracklistSelections * tls,
  bool                        set_notebook_page);

/**
 * Sets up the inspector track widget for the first time.
 */
void
inspector_track_widget_setup (
  InspectorTrackWidget *      self,
  SimpleTracklistSelections * tls);

InspectorTrackWidget *
inspector_track_widget_new (void);

/**
 * Prepare for finalization.
 */
void
inspector_track_widget_tear_down (InspectorTrackWidget * self);

/**
 * @}
 */

#endif
