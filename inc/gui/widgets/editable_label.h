/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 */

#ifndef __GUI_WIDGETS_EDITABLE_LABEL_H__
#define __GUI_WIDGETS_EDITABLE_LABEL_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define EDITABLE_LABEL_WIDGET_TYPE \
  (editable_label_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EditableLabelWidget,
  editable_label_widget,
  Z,
  EDITABLE_LABEL_WIDGET,
  GtkWidget)

/**
 * A label that shows a popover when clicked.
 */
typedef struct _EditableLabelWidget
{
  GtkWidget parent_instance;

  /** The label. */
  GtkLabel * label;

  /** Popover owned by another widget. */
  GtkPopover * foreign_popover;

  GtkPopover * popover;
  GtkEntry *   entry;

  /** Getter. */
  GenericStringGetter getter;

  /** Setter. */
  GenericStringSetter setter;

  /** Object to call get/set with. */
  void * object;

  /** Whether this is a temporary widget for just
   * showing the popover. */
  int is_temp;

  /** Multipress for the label. */
  GtkGestureClick * mp;
} EditableLabelWidget;

/**
 * Shows the popover.
 */
void
editable_label_widget_show_popover (
  EditableLabelWidget * self);

/**
 * Shows a popover without the need of an editable
 * label.
 *
 * @param popover A pre-created popover that is a
 *   child of @ref parent.
 */
void
editable_label_widget_show_popover_for_widget (
  GtkWidget *         parent,
  GtkPopover *        popover,
  void *              object,
  GenericStringGetter getter,
  GenericStringSetter setter);

/**
 * Sets up an existing EditableLabelWidget.
 *
 * @param getter Getter function.
 * @param setter Setter function.
 * @param object Object to call get/set with.
 */
void
editable_label_widget_setup (
  EditableLabelWidget * self,
  void *                object,
  GenericStringGetter   getter,
  GenericStringSetter   setter);

/**
 * Returns a new instance of EditableLabelWidget.
 *
 * @param getter Getter function.
 * @param setter Setter function.
 * @param object Object to call get/set with.
 * @param width Label width in chars.
 */
EditableLabelWidget *
editable_label_widget_new (
  void *              object,
  GenericStringGetter getter,
  GenericStringSetter setter,
  int                 width);

#endif
