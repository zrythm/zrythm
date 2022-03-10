/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline toolbar.
 */

#ifndef __GUI_WIDGETS_TIMELINE_TOOLBAR_H__
#define __GUI_WIDGETS_TIMELINE_TOOLBAR_H__

#include <gtk/gtk.h>

#define TIMELINE_TOOLBAR_WIDGET_TYPE \
  (timeline_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineToolbarWidget, timeline_toolbar_widget,
  Z, TIMELINE_TOOLBAR_WIDGET, GtkBox)

typedef struct _QuantizeMbWidget QuantizeMbWidget;
typedef struct _QuantizeBoxWidget QuantizeBoxWidget;
typedef struct _SnapBoxWidget SnapBoxWidget;
typedef struct _RangeActionButtonsWidget
  RangeActionButtonsWidget;
typedef struct _ZoomButtonsWidget ZoomButtonsWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_TOOLBAR \
  MW_TIMELINE_PANEL->timeline_toolbar

/**
 * The Timeline toolbar in the top.
 */
typedef struct _TimelineToolbarWidget
{
  GtkBox       parent_instance;
  SnapBoxWidget *  snap_box;
  QuantizeBoxWidget * quantize_box;
  GtkButton *  event_viewer_toggle;
  GtkToggleButton *  musical_mode_toggle;
  RangeActionButtonsWidget * range_action_buttons;
  ZoomButtonsWidget * zoom_buttons;
  GtkButton *  merge_btn;
} TimelineToolbarWidget;

void
timeline_toolbar_widget_refresh (
  TimelineToolbarWidget * self);

void
timeline_toolbar_widget_setup (
  TimelineToolbarWidget * self);

/**
 * @}
 */

#endif
