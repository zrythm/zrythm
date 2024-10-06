/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Track properties box.
 */

#ifndef __GUI_WIDGETS_TRACK_PROPERTIES_EXPANDER_H__
#define __GUI_WIDGETS_TRACK_PROPERTIES_EXPANDER_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/two_col_expander_box.h"

#define TRACK_PROPERTIES_EXPANDER_WIDGET_TYPE \
  (track_properties_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackPropertiesExpanderWidget,
  track_properties_expander_widget,
  Z,
  TRACK_PROPERTIES_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);

typedef struct _EditableLabelWidget EditableLabelWidget;
class Track;
typedef struct _RouteTargetSelectorWidget RouteTargetSelectorWidget;
typedef struct _ChannelSlotWidget         ChannelSlotWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _TrackPropertiesExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  /**
   * Editable label for displaying the name.
   *
   * This should take up the whole row.
   */
  EditableLabelWidget * name;

  RouteTargetSelectorWidget * direct_out;

  /** Instrument slot, for instrument tracks. */
  ChannelSlotWidget * instrument_slot;

  GtkLabel * instrument_label;

  /* TODO midi inputs, etc. See Instrument Track
   * Inspector from cubase manual. */

  /** Track the TrackPropertiesExpanderWidget is
   * associated with. */
  Track * track;
} TrackPropertiesExpanderWidget;

/**
 * Refreshes each field.
 */
void
track_properties_expander_widget_refresh (
  TrackPropertiesExpanderWidget * self,
  Track *                         track);

/**
 * Sets up the TrackPropertiesExpanderWidget.
 */
void
track_properties_expander_widget_setup (
  TrackPropertiesExpanderWidget * self,
  Track *                         track);

/**
 * @}
 */

#endif
