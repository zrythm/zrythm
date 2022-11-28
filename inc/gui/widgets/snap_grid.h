// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_SNAP_GRID_H__
#define __GUI_WIDGETS_SNAP_GRID_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define SNAP_GRID_WIDGET_TYPE (snap_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SnapGridWidget,
  snap_grid_widget,
  Z,
  SNAP_GRID_WIDGET,
  GtkWidget)

typedef struct _SnapGridPopoverWidget SnapGridPopoverWidget;
typedef struct SnapGrid               SnapGrid;

typedef struct _SnapGridWidget
{
  GtkWidget parent_instance;

  GtkMenuButton * menu_btn;

  AdwButtonContent * content;

  /** Popover. */
  SnapGridPopoverWidget * popover;

  /** Associated snap grid options. */
  SnapGrid * snap_grid;
} SnapGridWidget;

void
snap_grid_widget_setup (
  SnapGridWidget * self,
  SnapGrid *       snap_grid);

void
snap_grid_widget_refresh (SnapGridWidget * self);

#endif
