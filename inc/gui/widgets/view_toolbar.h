// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 */

#ifndef __GUI_WIDGETS_VIEW_TOOLBAR_H__
#define __GUI_WIDGETS_VIEW_TOOLBAR_H__

#include <gtk/gtk.h>
#include <libpanel.h>

#define VIEW_TOOLBAR_WIDGET_TYPE \
  (view_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ViewToolbarWidget,
  view_toolbar_widget,
  Z,
  VIEW_TOOLBAR_WIDGET,
  GtkBox)

#define MW_VIEW_TOOLBAR MW_HEADER_NOTEBOOK->view_toolbar

typedef struct _ViewToolbarWidget
{
  GtkBox              parent_instance;
  GtkButton *         status_bar;
  GtkButton *         fullscreen;
  PanelToggleButton * left_panel;
  PanelToggleButton * bot_panel;
  PanelToggleButton * top_panel;
  PanelToggleButton * right_panel;
} ViewToolbarWidget;

#endif
