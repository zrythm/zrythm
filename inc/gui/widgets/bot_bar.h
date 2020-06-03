/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Bottomest bar.
 */

#ifndef __GUI_WIDGETS_BOT_BAR_H__
#define __GUI_WIDGETS_BOT_BAR_H__

#include <gtk/gtk.h>

typedef struct _DigitalMeterWidget
  DigitalMeterWidget;
typedef struct _TransportControlsWidget
  TransportControlsWidget;
typedef struct _CpuWidget CpuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define BOT_BAR_WIDGET_TYPE \
  (bot_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BotBarWidget, bot_bar_widget,
  Z, BOT_BAR_WIDGET, GtkBox)

#define MW_BOT_BAR MW->bot_bar
#define MW_STATUS_BAR MW_BOT_BAR->status_bar
#define MW_DIGITAL_TRANSPORT \
  MW_BOT_BAR->digital_transport
#define MW_DIGITAL_BPM \
  MW_BOT_BAR->digital_bpm

typedef struct _BotBarWidget
{
  GtkBox                parent_instance;
  GtkStatusbar *        status_bar;

  /** New label replacing the original status
   * bar label. */
  GtkLabel *            label;

  /** Status bar context id. */
  guint                 context_id;

  GtkBox *                  digital_meters;
  DigitalMeterWidget *      digital_bpm;

  /**
   * Overlay for showing things on top of the
   * playhead positionmeter.
   */
  GtkOverlay *              playhead_overlay;

  /**
   * The playhead digital meter.
   */
  DigitalMeterWidget *      digital_transport;

  /** Jack timebase master image. */
  GtkWidget *               master_img;

  /** Jack client master image. */
  GtkWidget *               client_img;

  /**
   * Menuitems in context menu of digital transport.
   */
  GtkCheckMenuItem *        bbt_display_check;
  GtkCheckMenuItem *        time_display_check;

  GtkCheckMenuItem *        timebase_master_check;
  GtkCheckMenuItem *        transport_client_check;
  GtkCheckMenuItem *        no_jack_transport_check;

  GtkBox *                  playhead_box;

  DigitalMeterWidget *      digital_timesig;
  TransportControlsWidget * transport_controls;
  CpuWidget *               cpu_load;

  /** Color in hex to use in pango markups. */
  char                      hex_color[8];
} BotBarWidget;

void
bot_bar_widget_refresh (BotBarWidget * self);

/**
 * Updates the content of the status bar.
 */
void
bot_bar_widget_update_status (
  BotBarWidget * self);

/**
 * Sets up the bot bar.
 */
void
bot_bar_widget_setup (
  BotBarWidget * self);

/**
 * @}
 */

#endif
