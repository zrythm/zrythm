// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Home toolbar.
 */

#ifndef __GUI_WIDGETS_HOME_TOOLBAR_H__
#define __GUI_WIDGETS_HOME_TOOLBAR_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define HOME_TOOLBAR_WIDGET_TYPE \
  (home_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  HomeToolbarWidget,
  home_toolbar_widget,
  Z,
  HOME_TOOLBAR_WIDGET,
  GtkBox)

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_HOME_TOOLBAR MW_HEADER->home_toolbar

/**
 * The Home toolbar in the top.
 */
typedef struct _HomeToolbarWidget
{
  GtkBox           parent_instance;
  AdwSplitButton * undo_btn;
  AdwSplitButton * redo_btn;
  GtkButton *      cut;
  GtkButton *      copy;
  GtkButton *      paste;
  GtkButton *      duplicate;
  GtkButton * delete;
  GtkButton *     nudge_left;
  GtkButton *     nudge_right;
  GtkButton *     clear_selection;
  GtkButton *     select_all;
  GtkButton *     loop_selection;
  ToolboxWidget * toolbox;
} HomeToolbarWidget;

void
home_toolbar_widget_refresh_undo_redo_buttons (
  HomeToolbarWidget * self);

void
home_toolbar_widget_setup (
  HomeToolbarWidget * self);

/**
 * @}
 */

#endif
