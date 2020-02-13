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

#include <gtk/gtk.h>

#define EDITABLE_LABEL_WIDGET_TYPE \
  (editable_label_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EditableLabelWidget,
  editable_label_widget,
  Z, EDITABLE_LABEL_WIDGET,
  GtkEventBox)

/**
 * Getter prototype.
 */
typedef const char * (*EditableLabelWidgetTextGetter) (
  void * object);

/**
 * Setter prototype.
 */
typedef void (*EditableLabelWidgetTextSetter) (
  void *       object,
  const char * text);

/**
 * A label that shows a popover when clicked.
 */
typedef struct _EditableLabelWidget
{
  GtkEventBox     parent_instance;

  /** The label. */
  GtkLabel *      label;

  GtkPopover *      popover;
  GtkEntry *        entry;

  /** Getter. */
  EditableLabelWidgetTextGetter getter;

  /** Setter. */
  EditableLabelWidgetTextSetter setter;

  /** Object to call get/set with. */
  void *            object;

  /** Whether this is a temporary widget for just
   * showing the popover. */
  int               is_temp;

  /** Multipress for the label. */
  GtkGestureMultiPress * mp;
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
 */
void
editable_label_widget_show_popover_for_widget (
  GtkWidget * parent,
  void *      object,
  EditableLabelWidgetTextGetter getter,
  EditableLabelWidgetTextSetter setter);

/**
 * Sets up an existing EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 */
void
editable_label_widget_setup (
  EditableLabelWidget * self,
  void *                object,
  EditableLabelWidgetTextGetter getter,
  EditableLabelWidgetTextSetter setter);

/**
 * Returns a new instance of EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param width Label width in chars.
 */
EditableLabelWidget *
editable_label_widget_new (
  void *                object,
  EditableLabelWidgetTextGetter getter,
  EditableLabelWidgetTextSetter setter,
  int                    width);

#endif
