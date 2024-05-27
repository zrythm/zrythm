// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Timeline panel.
 */

#ifndef __GUI_WIDGETS_TIMELINE_PANEL_H__
#define __GUI_WIDGETS_TIMELINE_PANEL_H__

#include "utils/types.h"

#include "gtk_wrapper.h"

#define TIMELINE_PANEL_WIDGET_TYPE (timeline_panel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelinePanelWidget,
  timeline_panel_widget,
  Z,
  TIMELINE_PANEL_WIDGET,
  GtkBox)

TYPEDEF_STRUCT_UNDERSCORED (RulerWidget);
TYPEDEF_STRUCT_UNDERSCORED (TracklistWidget);
TYPEDEF_STRUCT_UNDERSCORED (TracklistHeaderWidget);
TYPEDEF_STRUCT_UNDERSCORED (TimelineToolbarWidget);
TYPEDEF_STRUCT_UNDERSCORED (TimelineBotBoxWidget);
TYPEDEF_STRUCT_UNDERSCORED (ArrangerWrapperWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_PANEL (MW_MAIN_NOTEBOOK->timeline_panel)

#define MW_TRACKLIST_SCROLL (MW_TIMELINE_PANEL->tracklist_scroll)

typedef struct _TimelinePanelWidget
{
  GtkBox parent_instance;

  GtkPaned *              tracklist_timeline;
  GtkBox *                tracklist_top;
  TracklistHeaderWidget * tracklist_header;
  TracklistWidget *       tracklist;

  /** Box for the timelines and the ruler. */
  GtkBox * timelines_plus_ruler;

  /** Ruler. */
  RulerWidget * ruler;

  /** The paned dividing the pinned and unpinned
   * timelines. */
  GtkBox * timeline_divider_box;

  ArrangerWrapperWidget * timeline_wrapper;

  /** The main timeline. */
  ArrangerWidget * timeline;

  TimelineToolbarWidget * timeline_toolbar;

  /** The pinned timeline above the main one. */
  ArrangerWidget * pinned_timeline;

  /** Size group for keeping the whole ruler and
   * each timeline the same width. */
  GtkSizeGroup * timeline_ruler_h_size_group;
} TimelinePanelWidget;

void
timeline_panel_widget_setup (TimelinePanelWidget * self);

TimelinePanelWidget *
timeline_panel_widget_new (void);

/**
 * Prepare for finalization.
 */
void
timeline_panel_widget_tear_down (TimelinePanelWidget * self);

/**
 * @}
 */

#endif
