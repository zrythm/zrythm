/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline panel.
 */

#ifndef __GUI_WIDGETS_TIMELINE_PANEL_H__
#define __GUI_WIDGETS_TIMELINE_PANEL_H__

#include <gtk/gtk.h>

#define TIMELINE_PANEL_WIDGET_TYPE \
  (timeline_panel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelinePanelWidget,
  timeline_panel_widget,
  Z,
  TIMELINE_PANEL_WIDGET,
  GtkBox)

typedef struct _RulerWidget     RulerWidget;
typedef struct _TracklistWidget TracklistWidget;
typedef struct _TimelinePanelBotBoxWidget
  TimelinePanelBotBoxWidget;
typedef struct _TracklistHeaderWidget
  TracklistHeaderWidget;
typedef struct _TimelineToolbarWidget
  TimelineToolbarWidget;
typedef struct _TimelineBotBoxWidget
  TimelineBotBoxWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_PANEL \
  (MW_MAIN_NOTEBOOK->timeline_panel)

#define MW_TRACKLIST_SCROLL \
  (MW_TIMELINE_PANEL->tracklist_scroll)

typedef struct _TimelinePanelWidget
{
  GtkBox parent_instance;

  GtkPaned *              tracklist_timeline;
  GtkBox *                tracklist_top;
  TracklistHeaderWidget * tracklist_header;
  TracklistWidget *       tracklist;

  /** Box for the timelines and the ruler. */
  GtkBox * timelines_plus_ruler;

  /** Scroll for ruler holding the viewport. */
  GtkScrolledWindow * ruler_scroll;

  /** Viewport for ruler holding the ruler. */
  GtkViewport * ruler_viewport;

  /** Ruler. */
  RulerWidget * ruler;

  /** The paned dividing the pinned and unpinned
   * timelines. */
  GtkBox * timeline_divider_box;

  GtkScrolledWindow * timeline_scroll;
  GtkViewport *       timeline_viewport;

  /** The main timeline. */
  ArrangerWidget * timeline;

  TimelineToolbarWidget * timeline_toolbar;

  GtkScrolledWindow * pinned_timeline_scroll;
  GtkViewport *       pinned_timeline_viewport;

  /** The pinned timeline above the main one. */
  ArrangerWidget * pinned_timeline;

  TimelineBotBoxWidget * bot_box;

  /** Size group for keeping the whole ruler and
   * each timeline the same width. */
  GtkSizeGroup * timeline_ruler_h_size_group;
} TimelinePanelWidget;

void
timeline_panel_widget_setup (
  TimelinePanelWidget * self);

TimelinePanelWidget *
timeline_panel_widget_new (void);

/**
 * Prepare for finalization.
 */
void
timeline_panel_widget_tear_down (
  TimelinePanelWidget * self);

/**
 * @}
 */

#endif
