/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Channel slot.
 */

#ifndef __GUI_WIDGETS_FADER_CONTROLS_GRID_H__
#define __GUI_WIDGETS_FADER_CONTROLS_GRID_H__

#include <gtk/gtk.h>

#define FADER_CONTROLS_GRID_WIDGET_TYPE \
  (fader_controls_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderControlsGridWidget,
  fader_controls_grid_widget,
  Z,
  FADER_CONTROLS_GRID_WIDGET,
  GtkGrid)

typedef struct Track        Track;
typedef struct _FaderWidget FaderWidget;
typedef struct _MeterWidget MeterWidget;
typedef struct _BalanceControlWidget
  BalanceControlWidget;
typedef struct _FaderButtonsWidget
  FaderButtonsWidget;

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
fader_controls_grid_widget_setup (
  FaderControlsGridWidget * self,
  Track *                   track);

/**
 * Prepare for finalization.
 */
void
fader_controls_grid_widget_tear_down (
  FaderControlsGridWidget * self);

FaderControlsGridWidget *
fader_controls_grid_widget_new (void);

/**
 * @}
 */

#endif
