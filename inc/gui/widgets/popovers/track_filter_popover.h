// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TRACK_FILTER_POPOVER_H__
#define __GUI_WIDGETS_TRACK_FILTER_POPOVER_H__

#include "gtk_wrapper.h"

#define TRACK_FILTER_POPOVER_WIDGET_TYPE \
  (track_filter_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackFilterPopoverWidget,
  track_filter_popover_widget,
  Z,
  TRACK_FILTER_POPOVER_WIDGET,
  GtkPopover)

typedef struct _TrackFilterPopoverWidget
{
  GtkPopover parent_instance;

  GtkCustomFilter * custom_filter;
  GtkColumnView *   track_col_view;

  GListStore * track_list_store;

  GPtrArray * item_factories;
} TrackFilterPopoverWidget;

/**
 * Creates the popover.
 */
TrackFilterPopoverWidget *
track_filter_popover_widget_new (void);

#endif
