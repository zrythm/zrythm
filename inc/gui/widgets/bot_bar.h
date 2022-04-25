// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Bottomest bar.
 */

#ifndef __GUI_WIDGETS_BOT_BAR_H__
#define __GUI_WIDGETS_BOT_BAR_H__

#include <gtk/gtk.h>
#include <libpanel.h>

typedef struct _DigitalMeterWidget
  DigitalMeterWidget;
typedef struct _TransportControlsWidget
                          TransportControlsWidget;
typedef struct _CpuWidget CpuWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define BOT_BAR_WIDGET_TYPE \
  (bot_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BotBarWidget,
  bot_bar_widget,
  Z,
  BOT_BAR_WIDGET,
  GtkWidget)

#define MW_BOT_BAR MW->bot_bar
#define MW_STATUS_BAR MW_BOT_BAR->status_bar
#define MW_DIGITAL_TRANSPORT \
  MW_BOT_BAR->digital_transport
#define MW_DIGITAL_BPM MW_BOT_BAR->digital_bpm
#define MW_DIGITAL_TIME_SIG \
  MW_BOT_BAR->digital_timesig

/**
 * Bot bar.
 */
typedef struct _BotBarWidget
{
  GtkWidget parent_instance;

  GtkCenterBox * center_box;
  GtkStatusbar * status_bar;

  /** New label replacing the original status
   * bar label. */
  GtkLabel * label;

  /** Status bar context id. */
  guint context_id;

  GtkBox *             digital_meters;
  DigitalMeterWidget * digital_bpm;

  /**
   * Overlay for showing things on top of the
   * playhead positionmeter.
   */
  GtkOverlay * playhead_overlay;

  ButtonWithMenuWidget * metronome;
  GtkToggleButton *      metronome_btn;

  /**
   * The playhead digital meter.
   */
  DigitalMeterWidget * digital_transport;

  /** Jack timebase master image. */
  GtkWidget * master_img;

  /** Jack client master image. */
  GtkWidget * client_img;

  GtkBox * playhead_box;

  DigitalMeterWidget *      digital_timesig;
  TransportControlsWidget * transport_controls;
  CpuWidget *               cpu_load;

  PanelDockSwitcher * bot_dock_switcher;

  /** Color in hex to use in pango markups. */
  char hex_color[8];
  char green_hex[8];
  char red_hex[8];

} BotBarWidget;

void
bot_bar_widget_refresh (BotBarWidget * self);

/**
 * Updates the content of the status bar.
 */
void
bot_bar_widget_update_status (BotBarWidget * self);

/**
 * Sets up the bot bar.
 */
void
bot_bar_widget_setup (BotBarWidget * self);

/**
 * @}
 */

#endif
