// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_FADER_CONTROLS_GRID_H__
#define __GUI_WIDGETS_FADER_CONTROLS_GRID_H__

#include "common/dsp/channel_track.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define FADER_CONTROLS_GRID_WIDGET_TYPE (fader_controls_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsGridWidget,
  fader_controls_grid_widget,
  Z,
  FADER_CONTROLS_GRID_WIDGET,
  GtkGrid)

class Track;
TYPEDEF_STRUCT_UNDERSCORED (FaderWidget);
TYPEDEF_STRUCT_UNDERSCORED (MeterWidget);
TYPEDEF_STRUCT_UNDERSCORED (BalanceControlWidget);
TYPEDEF_STRUCT_UNDERSCORED (FaderButtonsWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

using FaderControlsGridWidget = struct _FaderControlsGridWidget
{
  GtkGrid parent_instance;

  GtkBox *               meters_box;
  MeterWidget *          meter_l;
  MeterWidget *          meter_r;
  GtkBox *               balance_box;
  BalanceControlWidget * balance_control;
  FaderWidget *          fader;

  Track * track;

  FaderButtonsWidget * fader_buttons;

  GtkLabel * meter_readings;

  double meter_reading_val;

  /** Last MIDI event trigger time, for MIDI
   * output. */
  gint64 last_midi_trigger_time;

  guint tick_cb;
};

void
fader_controls_grid_widget_setup (
  FaderControlsGridWidget * self,
  ChannelTrack *            track);

/**
 * Prepare for finalization.
 */
void
fader_controls_grid_widget_tear_down (FaderControlsGridWidget * self);

FaderControlsGridWidget *
fader_controls_grid_widget_new ();

/**
 * @}
 */

#endif
