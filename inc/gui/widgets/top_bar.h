// SPDX-FileCopyrightText: © 2018-2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TOP_BAR_H__
#define __GUI_WIDGETS_TOP_BAR_H__

#include <gtk/gtk.h>

#define TOP_BAR_WIDGET_TYPE (top_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TopBarWidget,
  top_bar_widget,
  Z,
  TOP_BAR_WIDGET,
  GtkBox)

#define TOP_BAR MW->top_bar

typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _TransportControlsWidget TransportControlsWidget;
typedef struct _CpuWidget             CpuWidget;
typedef struct _MidiActivityBarWidget MidiActivityBarWidget;

typedef struct _TopBarWidget
{
  GtkBox   parent_instance;
  GtkBox * top_bar_left;
} TopBarWidget;

void
top_bar_widget_refresh (TopBarWidget * self);

#endif
