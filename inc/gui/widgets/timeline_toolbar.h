// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Timeline toolbar.
 */

#ifndef __GUI_WIDGETS_TIMELINE_TOOLBAR_H__
#define __GUI_WIDGETS_TIMELINE_TOOLBAR_H__

#include <gtk/gtk.h>

#define TIMELINE_TOOLBAR_WIDGET_TYPE (timeline_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineToolbarWidget,
  timeline_toolbar_widget,
  Z,
  TIMELINE_TOOLBAR_WIDGET,
  GtkBox)

typedef struct _QuantizeMbWidget         QuantizeMbWidget;
typedef struct _QuantizeBoxWidget        QuantizeBoxWidget;
typedef struct _SnapGridWidget           SnapGridWidget;
typedef struct _RangeActionButtonsWidget RangeActionButtonsWidget;
typedef struct _ZoomButtonsWidget        ZoomButtonsWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_TOOLBAR MW_TIMELINE_PANEL->timeline_toolbar

/**
 * The Timeline toolbar in the top.
 */
typedef struct _TimelineToolbarWidget
{
  GtkBox                     parent_instance;
  SnapGridWidget *           snap_grid;
  QuantizeBoxWidget *        quantize_box;
  GtkButton *                event_viewer_toggle;
  GtkToggleButton *          musical_mode_toggle;
  RangeActionButtonsWidget * range_action_buttons;
  ZoomButtonsWidget *        zoom_buttons;
  GtkButton *                merge_btn;
} TimelineToolbarWidget;

void
timeline_toolbar_widget_refresh (TimelineToolbarWidget * self);

void
timeline_toolbar_widget_setup (TimelineToolbarWidget * self);

/**
 * @}
 */

#endif
