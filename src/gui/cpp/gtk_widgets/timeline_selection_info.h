/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Timeline selection info.
 */

#ifndef __GUI_WIDGETS_TIMELINE_SELECTION_INFO_H__
#define __GUI_WIDGETS_TIMELINE_SELECTION_INFO_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/region.h"

#include "common/dsp/region.h"
#include "common/utils/ui.h"

#define TIMELINE_SELECTION_INFO_WIDGET_TYPE \
  (timeline_selection_info_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineSelectionInfoWidget,
  timeline_selection_info_widget,
  Z,
  TIMELINE_SELECTION_INFO_WIDGET,
  GtkStack);

#define MW_TS_INFO MW_TIMELINE_PANEL->selection_info

typedef struct _SelectionInfoWidget SelectionInfoWidget;
class TimelineSelections;

/**
 * A widget for showing info about the current
 * TimelineSelections.
 */
typedef struct _TimelineSelectionInfoWidget
{
  GtkStack              parent_instance;
  GtkLabel *            no_selection_label;
  SelectionInfoWidget * selection_info;
} TimelineSelectionInfoWidget;

/**
 * Populates the SelectionInfoWidget based on the
 * leftmost object selected.
 */
void
timeline_selection_info_widget_refresh (
  TimelineSelectionInfoWidget * self,
  TimelineSelections *          ts);

#endif
