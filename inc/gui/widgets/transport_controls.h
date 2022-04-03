/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRANSPORT_CONTROLS \
  MW_BOT_BAR->transport_controls

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
