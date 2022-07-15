/* * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_TWO_COL_EXPANDER_BOX_H__
#define __GUI_WIDGETS_TWO_COL_EXPANDER_BOX_H__

#include "gui/widgets/expander_box.h"

#include <gtk/gtk.h>

#define TWO_COL_EXPANDER_BOX_WIDGET_TYPE \
  (two_col_expander_box_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  TwoColExpanderBoxWidget,
  two_col_expander_box_widget,
  Z,
  TWO_COL_EXPANDER_BOX_WIDGET,
  ExpanderBoxWidget)

#define TWO_COL_EXPANDER_BOX_DEFAULT_HORIZONTAL_SPACING 4
#define TWO_COL_EXPANDER_BOX_DEFAULT_VERTICAL_SPACING 0

/**
 * A two column expander for the simple case that the
 * contents are two columns with fixed spacing.
 *
 * Used in the inspector.
 */
typedef struct
{
  /**
   * The scrolled window holding the content.
   */
  GtkScrolledWindow * scroll;

  /**
   * This is an additional box to what
   * ExpanderBoxWidget does that will hold a bunch
   * of pairs (e.g. key-value) stacked vertically.
   */
  GtkBox * content;

  /**
   * The spacing to use in each horizontal box.
   */
  int horizontal_spacing;

  /**
   * The spacing to use between stacked boxes.
   */
  int vertical_spacing;

  /** Max width of content. */
  int max_width;

  /** Max height of content. */
  int max_height;

  /** Show scrollbars when max size is reached. */
  int show_scroll;

} TwoColExpanderBoxWidgetPrivate;

typedef struct _TwoColExpanderBoxWidgetClass
{
  ExpanderBoxWidgetClass parent_class;
} TwoColExpanderBoxWidgetClass;

/**
 * Gets the private.
 */
TwoColExpanderBoxWidgetPrivate *
two_col_expander_box_widget_get_private (
  TwoColExpanderBoxWidget * self);

/**
 * Sets the horizontal spacing.
 */
void
two_col_expander_box_widget_set_horizontal_spacing (
  TwoColExpanderBoxWidget * self,
  int                       horizontal_spacing);

/**
 * Sets the max size.
 */
void
two_col_expander_box_widget_set_min_max_size (
  TwoColExpanderBoxWidget * self,
  const int                 min_w,
  const int                 min_h,
  const int                 max_w,
  const int                 max_h);

/**
 * Sets whether to show scrollbars or not.
 */
void
two_col_expander_box_widget_set_scroll_policy (
  TwoColExpanderBoxWidget * self,
  GtkPolicyType             hscrollbar_policy,
  GtkPolicyType             vscrollbar_policy);

/**
 * Adds the two widgets in a horizontal box with the
 * given spacing.
 */
void
two_col_expander_box_widget_add_pair (
  TwoColExpanderBoxWidget * self,
  GtkWidget *               widget1,
  GtkWidget *               widget2);

/**
 * Adds a single widget taking up the full horizontal
 * space.
 */
void
two_col_expander_box_widget_add_single (
  TwoColExpanderBoxWidget * self,
  GtkWidget *               widget);

/**
 * Removes and destroys the children widgets.
 */
void
two_col_expander_box_widget_remove_children (
  TwoColExpanderBoxWidget * self);

/**
 * Gets the content box.
 */
GtkBox *
two_col_expander_box_widget_get_content_box (
  TwoColExpanderBoxWidget * self);

#endif
