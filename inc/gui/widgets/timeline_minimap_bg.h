// SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline minimap bg.
 */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_BG_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_BG_H__

#include <gtk/gtk.h>

#define TIMELINE_MINIMAP_BG_WIDGET_TYPE (timeline_minimap_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineMinimapBgWidget,
  timeline_minimap_bg_widget,
  Z,
  TIMELINE_MINIMAP_BG_WIDGET,
  GtkWidget)

typedef struct _TimelineMinimapBgWidget
{
  GtkWidget parent_instance;
} TimelineMinimapBgWidget;

TimelineMinimapBgWidget *
timeline_minimap_bg_widget_new (void);

#endif
