// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TRACK_FILTER_POPOVER_H__
#define __GUI_WIDGETS_TRACK_FILTER_POPOVER_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/item_factory.h"

#define TRACK_FILTER_POPOVER_WIDGET_TYPE \
  (track_filter_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackFilterPopoverWidget,
  track_filter_popover_widget,
  Z,
  TRACK_FILTER_POPOVER_WIDGET,
  GtkPopover)

using TrackFilterPopoverWidget = struct _TrackFilterPopoverWidget
{
  GtkPopover parent_instance;

  GtkCustomFilter * custom_filter;
  GtkColumnView *   track_col_view;

  GListStore * track_list_store;

  ItemFactoryPtrVector item_factories;
};

/**
 * Creates the popover.
 */
TrackFilterPopoverWidget *
track_filter_popover_widget_new ();

#endif
