/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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

#ifndef __GUI_WIDGETS_FADER_BUTTONS_H__
#define __GUI_WIDGETS_FADER_BUTTONS_H__

#include <gtk/gtk.h>

#define FADER_BUTTONS_WIDGET_TYPE \
  (fader_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FaderButtonsWidget,
  fader_buttons_widget,
  Z, FADER_BUTTONS_WIDGET,
  GtkButtonBox)

typedef struct Track Track;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _FaderButtonsWidget
{
  GtkButtonBox   parent_instance;

  GtkToggleButton * mono_compat;
  GtkToggleButton * solo;
  GtkToggleButton * mute;
  GtkToggleButton * record;
  GtkToggleButton * listen;
  GtkButton *    e;

  /**
   * Signal handler IDs.
   */
  gulong         mono_compat_toggled_handler_id;
  gulong         record_toggled_handler_id;
  gulong         solo_toggled_handler_id;
  gulong         mute_toggled_handler_id;

  /** Owner track. */
  Track *        track;

} FaderButtonsWidget;

void
fader_buttons_widget_block_signal_handlers (
  FaderButtonsWidget * self);

void
fader_buttons_widget_unblock_signal_handlers (
  FaderButtonsWidget * self);

void
fader_buttons_widget_refresh (
  FaderButtonsWidget * self,
  Track *              track);

/**
 * @}
 */

#endif
