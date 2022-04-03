/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Inspector section for tracks.
 */

#ifndef __GUI_WIDGETS_INSPECTOR_TRACK_H__
#define __GUI_WIDGETS_INSPECTOR_TRACK_H__

#include <gtk/gtk.h>

#define INSPECTOR_TRACK_WIDGET_TYPE \
  (inspector_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorTrackWidget,
  inspector_track_widget,
  Z,
  INSPECTOR_TRACK_WIDGET,
  GtkBox)

typedef struct TracklistSelections
  TracklistSelections;
typedef struct _TrackPropertiesExpanderWidget
  TrackPropertiesExpanderWidget;
typedef struct _PortsExpanderWidget
  PortsExpanderWidget;
typedef struct _TrackInputExpanderWidget
  TrackInputExpanderWidget;
typedef struct _PluginStripExpanderWidget
  PluginStripExpanderWidget;
typedef struct _FaderControlsExpanderWidget
  FaderControlsExpanderWidget;
typedef struct _TextExpanderWidget TextExpanderWidget;
typedef struct _ColorAreaWidget    ColorAreaWidget;
typedef struct _ChannelSendsExpanderWidget
  ChannelSendsExpanderWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACK_INSPECTOR \
  MW_LEFT_DOCK_EDGE->track_inspector

/**
 * Inspector section for tracks.
 */
typedef struct _InspectorTrackWidget
{
  GtkBox                          parent_instance;
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
} InspectorTrackWidget;

/**
 * Shows the inspector page for the given tracklist
 * selection.
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the track page.
 */
NONNULL
void
inspector_track_widget_show_tracks (
  InspectorTrackWidget * self,
  TracklistSelections *  tls,
  bool                   set_notebook_page);

/**
 * Sets up the inspector track widget for the first
 * time.
 */
void
inspector_track_widget_setup (
  InspectorTrackWidget * self,
  TracklistSelections *  tls);

InspectorTrackWidget *
inspector_track_widget_new (void);

/**
 * Prepare for finalization.
 */
void
inspector_track_widget_tear_down (
  InspectorTrackWidget * self);

/**
 * @}
 */

#endif
