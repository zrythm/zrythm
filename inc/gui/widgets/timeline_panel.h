/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/dzl/dzl-dock-bin.h"
#include "gui/widgets/timeline_selection_info.h"

#include <gtk/gtk.h>

#define TIMELINE_PANEL_WIDGET_TYPE \
  (timeline_panel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelinePanelWidget,
  timeline_panel_widget,
  Z, TIMELINE_PANEL_WIDGET,
  GtkBox)

typedef struct _RulerWidget RulerWidget;
typedef struct _TracklistWidget TracklistWidget;
typedef struct _SnapGridWidget SnapGridWidget;
typedef struct _TimelineArrangerWidget
  TimelineArrangerWidget;
typedef struct _TimelineRulerWidget
  TimelineRulerWidget;
typedef struct _LeftDockEdgeWidget LeftDockEdgeWidget;
typedef struct _RightDockEdgeWidget
  RightDockEdgeWidget;
typedef struct _BotDockEdgeWidget BotDockEdgeWidget;
typedef struct _TimelinePanelBotBoxWidget
  TimelinePanelBotBoxWidget;
typedef struct _TracklistHeaderWidget
  TracklistHeaderWidget;
typedef struct _PinnedTracklistWidget
  PinnedTracklistWidget;
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
  MW_CENTER_DOCK->timeline_panel

typedef struct _TimelinePanelWidget
{
  GtkBox                  parent_instance;

  /** The paned containing the
   * PinnedTracklistWidget and the
   * TracklistWidget. */
  GtkPaned *              tracklist_paned;

  GtkPaned *              tracklist_timeline;
  GtkBox *                tracklist_top;
  GtkScrolledWindow *     tracklist_scroll;
  GtkViewport *           tracklist_viewport;
  PinnedTracklistWidget * pinned_tracklist;
  TracklistHeaderWidget * tracklist_header;
  TracklistWidget *       tracklist;

  /** Box for the timelines and the ruler. */
  GtkBox *                timelines_plus_ruler;

  /** Scroll for ruler holding the viewport. */
  GtkScrolledWindow *     ruler_scroll;

  /** Viewport for ruler holding the ruler. */
  GtkViewport *           ruler_viewport;

  /** Ruler. */
  TimelineRulerWidget *   ruler;

  /** The paned dividing the pinned and unpinned
   * timelines. */
  GtkPaned *              timeline_divider_pane;

  GtkScrolledWindow *     timeline_scroll;
  GtkViewport *           timeline_viewport;

  /** The main timeline. */
  TimelineArrangerWidget * timeline;

  TimelineToolbarWidget * timeline_toolbar;

  GtkScrolledWindow *     pinned_timeline_scroll;
  GtkViewport *           pinned_timeline_viewport;

  /** The pinned timeline above the main one. */
  TimelineArrangerWidget * pinned_timeline;

  TimelineBotBoxWidget *  bot_box;
} TimelinePanelWidget;

void
timeline_panel_widget_setup (
  TimelinePanelWidget * self);

/**
 * @}
 */

#endif
