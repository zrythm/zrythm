// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TRACKLIST_PREFERENCES_POPOVER_H__
#define __GUI_WIDGETS_TRACKLIST_PREFERENCES_POPOVER_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define TRACKLIST_PREFERENCES_POPOVER_WIDGET_TYPE \
  (tracklist_preferences_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TracklistPreferencesPopoverWidget,
  tracklist_preferences_popover_widget,
  Z,
  TRACKLIST_PREFERENCES_POPOVER_WIDGET,
  GtkPopover)

typedef struct _TracklistPreferencesPopoverWidget
{
  GtkPopover parent_instance;
} TracklistPreferencesPopoverWidget;

/**
 * Creates the popover.
 */
TracklistPreferencesPopoverWidget *
tracklist_preferences_popover_widget_new (void);

#endif
