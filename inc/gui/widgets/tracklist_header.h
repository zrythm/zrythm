// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Composite widget above the tracklist.
 */

#ifndef __GUI_WIDGETS_TRACKLIST_HEADER_H__
#define __GUI_WIDGETS_TRACKLIST_HEADER_H__

#include <gtk/gtk.h>

#define TRACKLIST_HEADER_WIDGET_TYPE \
  (tracklist_header_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TracklistHeaderWidget,
  tracklist_header_widget,
  Z,
  TRACKLIST_HEADER_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACKLIST_HEADER \
  (MW_TIMELINE_PANEL->tracklist_header)

typedef struct _TracklistHeaderWidget
{
  GtkWidget  parent_instance;
  GtkLabel * track_count_lbl;

  GtkMenuButton * filter_menu_btn;
} TracklistHeaderWidget;

void
tracklist_header_widget_refresh_track_count (
  TracklistHeaderWidget * self);

void
tracklist_header_widget_setup (TracklistHeaderWidget * self);

/**
 * @}
 */

#endif
