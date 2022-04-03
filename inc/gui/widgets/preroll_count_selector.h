/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_PREROLL_COUNT_SELECTOR_H__
#define __GUI_WIDGETS_PREROLL_COUNT_SELECTOR_H__

/**
 * \file
 *
 * Bounce step selector.
 */

#include <gtk/gtk.h>

#define PREROLL_COUNT_SELECTOR_WIDGET_TYPE \
  (preroll_count_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PrerollCountSelectorWidget,
  preroll_count_selector_widget,
  Z,
  PREROLL_COUNT_SELECTOR_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum PrerollCountSelectorType
{
  /** Countin before starting playback. */
  PREROLL_COUNT_SELECTOR_METRONOME_COUNTIN,

  /** Preroll to start at before the punch in
   * position during recording. */
  PREROLL_COUNT_SELECTOR_RECORD_PREROLL,
} PrerollCountSelectorType;

typedef struct _PrerollCountSelectorWidget
{
  GtkBox parent_instance;

  PrerollCountSelectorType type;

  GtkToggleButton * none_toggle;
  gulong            none_toggle_id;
  GtkToggleButton * one_bar_toggle;
  gulong            one_bar_toggle_id;
  GtkToggleButton * two_bars_toggle;
  gulong            two_bars_toggle_id;
  GtkToggleButton * four_bars_toggle;
  gulong            four_bars_toggle_id;

} PrerollCountSelectorWidget;

/**
 * Creates a PrerollCountSelectorWidget.
 */
PrerollCountSelectorWidget *
preroll_count_selector_widget_new (
  PrerollCountSelectorType type);

/**
 * @}
 */

#endif
