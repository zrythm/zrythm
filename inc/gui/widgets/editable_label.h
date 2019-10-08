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
  const char * (*getter)(void *);

  /** Setter. */
  void (*setter)(void *, const char *);

  /** Object to call get/set with. */
  void *            object;

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
 * Sets up an existing EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 */
void
_editable_label_widget_setup (
  EditableLabelWidget * self,
  void *                object,
  const char * (*get_val)(void *),
  void (*set_val)(void *, const char *));

#define \
editable_label_widget_setup( \
  self,object,getter,setter) \
  _editable_label_widget_setup ( \
    self, \
    (void *) object, \
    (const char * (*) (void *)) getter, \
    (void (*) (void *, const char *)) setter);

/**
 * Returns a new instance of EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param width Label width in chars.
 */
EditableLabelWidget *
_editable_label_widget_new (
  void *                object,
  const char * (*get_val)(void *),
  void (*set_val)(void *, const char *),
  int                    width);

#define \
editable_label_widget_new(object,getter,setter,lbl) \
  _editable_label_widget_new ( \
    (void *) object, \
    (const char * (*) (void *)) getter, \
    (void (*) (void *, const char *)) setter, \
    lbl)

#endif
