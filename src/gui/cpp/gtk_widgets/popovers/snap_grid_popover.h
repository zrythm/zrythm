// SPDX-FileCopyrightText: © 2019-2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Snap grid popover.
 */

#ifndef __GUI_WIDGETS_SNAP_GRID_POPOVER_H__
#define __GUI_WIDGETS_SNAP_GRID_POPOVER_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/libadwaita_wrapper.h"

typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _SnapGridWidget     SnapGridWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SNAP_GRID_POPOVER_WIDGET_TYPE (snap_grid_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SnapGridPopoverWidget,
  snap_grid_popover_widget,
  Z,
  SNAP_GRID_POPOVER_WIDGET,
  GtkPopover)

/**
 * Snap grid popover.
 */
typedef struct _SnapGridPopoverWidget
{
  GtkPopover parent_instance;

  /** Owner button. */
  SnapGridWidget * owner;

  AdwPreferencesPage * pref_page;

  /* --- snap --- */
  AdwPreferencesGroup * snap_position_group;
  GtkSwitch *           snap_to_grid;
  GtkSwitch *           adaptive_snap;
  AdwActionRow *        adaptive_snap_row;
  AdwComboRow *         snap_length;
  AdwComboRow *         snap_type;
  GtkSwitch *           keep_offset;
  AdwActionRow *        keep_offset_row;
  GtkSwitch *           snap_to_events;
  AdwActionRow *        snap_to_events_row;

  /* --- object lengths --- */
  AdwPreferencesGroup * object_length_group;
  AdwComboRow *         object_length_type;
  AdwComboRow *         object_length;
  AdwComboRow *         object_length_type_custom;

} SnapGridPopoverWidget;

/**
 * Creates the popover.
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner);

/**
 * @}
 */

#endif
