/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * A foldable GtkNotebook.
 */

#ifndef __GUI_WIDGETS_FOLDABLE_NOTEBOOK_H__
#define __GUI_WIDGETS_FOLDABLE_NOTEBOOK_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define FOLDABLE_NOTEBOOK_WIDGET_TYPE \
  (foldable_notebook_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FoldableNotebookWidget,
  foldable_notebook_widget,
  Z, FOLDABLE_NOTEBOOK_WIDGET,
  GtkBox);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_BOT_FOLDABLE_NOTEBOOK \
  MW_BOT_DOCK_EDGE->bot_notebook

/**
 * A GtkNotebook that shows or hides itself when the
 * same page tab is clicked.
 *
 * It assumes that each page is wrapped in a GtkBox.
 * The reason is that Gtk automatically hides the
 * tab widget too if you hide the main child of a
 * page, so we hide the box's child instead.
 */
typedef struct _FoldableNotebookWidget
{
  GtkBox              parent_instance;

  GtkNotebook *       notebook;

  GtkGestureClick *   mp;

  /** Paned associated with this notebook to set it
   * to max/min when hiding. */
  GtkPaned *          paned;

  /** Notebook position in the paned. */
  GtkPositionType     pos_in_paned;

  /** Previous paned position before setting it to
   * max/min. */
  //int               prev_paned_pos;

  /** Revealer position before hiding. */
  int                 prev_pos;

  /** Whether to add text to the tab labels
   * (otherwise just icons). */
  bool                with_text;

  /**
   * Current tab during a press action.
   *
   * Used to check if the same tab is active on
   * release in order to hide the tab.
   */
  GtkWidget *         tab_during_press;

  guint               switch_page_handler_id;
} FoldableNotebookWidget;

/**
 * Creates a FoldableNotebookWidget.
 */
FoldableNotebookWidget *
foldable_notebook_widget_new (
  GtkPositionType          pos_in_paned,
  bool                     with_text);

/**
 * Get the widget at the given page.
 */
GtkWidget *
foldable_notebook_widget_get_widget_at_page (
  FoldableNotebookWidget * self,
  int                      page);

/**
 * Get the widget currently visible.
 *
 * FIXME this is not working with nested containers
 * like scrolled windows. this whole widget needs
 * a rewrite.
 */
GtkWidget *
foldable_notebook_widget_get_current_widget (
  FoldableNotebookWidget * self);

/**
 * Sets the folded space visible or not.
 */
void
foldable_notebook_widget_set_visibility (
  FoldableNotebookWidget * self,
  bool                     new_visibility);

/**
 * Returns if the content of the foldable notebook
 * is visible.
 */
int
foldable_notebook_widget_is_content_visible (
  FoldableNotebookWidget * self);

/**
 * Gets the internal notebook.
 */
GtkNotebook *
foldable_notebook_widget_get_notebook (
  FoldableNotebookWidget * self);

void
foldable_notebook_widget_set_current_page (
  FoldableNotebookWidget * self,
  int                      page_num,
  bool                     block_signals);

int
foldable_notebook_widget_get_current_page (
  FoldableNotebookWidget * self);

void
foldable_notebook_widget_add_page (
  FoldableNotebookWidget * self,
  GtkWidget *              child,
  const char *             tab_icon_name,
  const char *             tab_label,
  const char *             tooltip);

/**
 * Combines the above.
 */
void
foldable_notebook_widget_toggle_visibility (
  FoldableNotebookWidget * self);

/**
 * Sets up an existing FoldableNotebookWidget.
 */
void
foldable_notebook_widget_setup (
  FoldableNotebookWidget * self,
  GtkPaned *               paned,
  GtkPositionType          pos_in_paned,
  bool                     with_text);

/**
 * @}
 */

#endif
