// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Transport controls widget.
 */

#ifndef __GUI_WIDGETS_TRANSPORT_CONTROLS_H__
#define __GUI_WIDGETS_TRANSPORT_CONTROLS_H__

#include <gtk/gtk.h>

#define TRANSPORT_CONTROLS_WIDGET_TYPE \
  (transport_controls_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TransportControlsWidget,
  transport_controls_widget,
  Z,
  TRANSPORT_CONTROLS_WIDGET,
  GtkBox)

typedef struct _ButtonWithMenuWidget ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRANSPORT_CONTROLS MW_BOT_BAR->transport_controls

/**
 * Transport controls.
 */
typedef struct _TransportControlsWidget
{
  GtkBox                 parent_instance;
  GtkButton *            play;
  GtkButton *            stop;
  GtkButton *            backward;
  GtkButton *            forward;
  GtkToggleButton *      trans_record_btn;
  ButtonWithMenuWidget * trans_record;
  GtkToggleButton *      loop;
  GtkToggleButton *      return_to_cue_toggle;

  gulong rec_toggled_handler_id;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} TransportControlsWidget;

void
transport_controls_widget_refresh (
  TransportControlsWidget * self);

/**
 * @}
 */

#endif
