/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Timeline ruler derived from base ruler.
 */

#ifndef __GUI_WIDGETS_EDITABLE_LABEL_H__
#define __GUI_WIDGETS_EDITABLE_LABEL_H__

#include <gtk/gtk.h>

#define EDITABLE_LABEL_WIDGET_TYPE \
  (editable_label_widget_get_type ())
G_DECLARE_FINAL_TYPE (EditableLabelWidget,
                      editable_label_widget,
                      Z,
                      EDITABLE_LABEL_WIDGET,
                      GtkEventBox)

/**
 * Type of editable label.
 */
typedef enum EditableLabelType
{
  EDITABLE_LABEL_TYPE_CHANNEL,
} EditableLabelType;

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

  /** Type of editable label. */
  EditableLabelType type;

  /** Owner widget, determined by type. */
  GtkWidget * owner;

  /** Multipress for the label. */
  GtkGestureMultiPress * mp;
} EditableLabelWidget;

/**
 * Sets up an existing EditableLabelWidget.
 */
void
editable_label_widget_setup (
  EditableLabelWidget * self,
  GtkWidget *           owner,
  EditableLabelType     type);

/**
 * Returns a new instance of EditableLabelWidget.
 */
EditableLabelWidget *
editable_label_widget_new (
  GtkWidget *       owner,
  EditableLabelType type);

#endif
