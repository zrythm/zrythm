/*
 * Copyright (C) 2018-2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_TOP_BAR_H__
#define __GUI_WIDGETS_TOP_BAR_H__

#include <gtk/gtk.h>

#define TOP_BAR_WIDGET_TYPE \
  (top_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TopBarWidget,
  top_bar_widget,
  Z,
  TOP_BAR_WIDGET,
  GtkBox)

#define TOP_BAR MW->top_bar

typedef struct _DigitalMeterWidget
  DigitalMeterWidget;
typedef struct _TransportControlsWidget
                          TransportControlsWidget;
typedef struct _CpuWidget CpuWidget;
typedef struct _MidiActivityBarWidget
  MidiActivityBarWidget;
typedef struct _LiveWaveformWidget
  LiveWaveformWidget;

typedef struct _TopBarWidget
{
  GtkBox   parent_instance;
  GtkBox * top_bar_left;
} TopBarWidget;

void
top_bar_widget_refresh (TopBarWidget * self);

#endif
