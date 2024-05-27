/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Channel slot.
 */

#ifndef __GUI_WIDGETS_FADER_CONTROLS_GRID_H__
#define __GUI_WIDGETS_FADER_CONTROLS_GRID_H__

#include "gtk_wrapper.h"

#define FADER_CONTROLS_GRID_WIDGET_TYPE (fader_controls_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsGridWidget,
  fader_controls_grid_widget,
  Z,
  FADER_CONTROLS_GRID_WIDGET,
  GtkGrid)

typedef struct Track                 Track;
typedef struct _FaderWidget          FaderWidget;
typedef struct _MeterWidget          MeterWidget;
typedef struct _BalanceControlWidget BalanceControlWidget;
typedef struct _FaderButtonsWidget   FaderButtonsWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _FaderControlsGridWidget
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

} FaderControlsGridWidget;

void
fader_controls_grid_widget_setup (FaderControlsGridWidget * self, Track * track);

/**
 * Prepare for finalization.
 */
void
fader_controls_grid_widget_tear_down (FaderControlsGridWidget * self);

FaderControlsGridWidget *
fader_controls_grid_widget_new (void);

/**
 * @}
 */

#endif
