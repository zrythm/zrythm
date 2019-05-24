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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_EXPANDER_BOX_H__
#define __GUI_WIDGETS_EXPANDER_BOX_H__

#include "utils/resources.h"

#include <gtk/gtk.h>

#define EXPANDER_BOX_WIDGET_TYPE \
  (expander_box_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  ExpanderBoxWidget,
  expander_box_widget,
  Z,
  EXPANDER_BOX_WIDGET,
  GtkBox)

/**
 * An expander box is a base widget with a button that
 * when clicked expands the contents.
 */
typedef struct
{
  GtkButton *    button;
  GtkBox *       btn_box;
  GtkLabel *     btn_label;
  GtkImage *     btn_img;
  GtkRevealer *  revealer;
  GtkBox *       content;

  /** Horizontal or vertical. */
  GtkOrientation orientation;

} ExpanderBoxWidgetPrivate;

typedef struct _ExpanderBoxWidgetClass
{
  GtkBoxClass         parent_class;
} ExpanderBoxWidgetClass;

/**
 * Gets the private.
 */
ExpanderBoxWidgetPrivate *
expander_box_widget_get_private (
  ExpanderBoxWidget * self);

/**
 * Sets the label to show.
 */
void
expander_box_widget_set_label (
  ExpanderBoxWidget * self,
  const char *        label);

/**
 * Sets the icon name to show.
 */
void
expander_box_widget_set_icon_name (
  ExpanderBoxWidget * self,
  const char *        icon_name);

/**
 * Sets the icon resource to show.
 */
static inline void
expander_box_widget_set_icon_resource (
  ExpanderBoxWidget * self,
  IconType            icon_type,
  const char *        path)
{
  ExpanderBoxWidgetPrivate * prv =
     expander_box_widget_get_private (self);

  resources_set_image_icon (
    prv->btn_img,
    icon_type,
    path);
}

static inline void
expander_box_widget_add_content (
  ExpanderBoxWidget * self,
  GtkWidget *         content)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);
  gtk_container_add (
    GTK_CONTAINER (prv->content),
    content);
}

static inline void
expander_box_widget_set_orientation (
  ExpanderBoxWidget * self,
  GtkOrientation      orientation)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);

  /* set the main orientation */
  prv->orientation = orientation;
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self),
    orientation);

  /* set the orientation of the box inside the
   * expander button */
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (prv->btn_box),
    orientation == GTK_ORIENTATION_HORIZONTAL ?
    GTK_ORIENTATION_VERTICAL :
    GTK_ORIENTATION_HORIZONTAL);

  /* set the label angle */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_label_set_angle (
      prv->btn_label, 90.0);
  else
    gtk_label_set_angle (
      prv->btn_label, 0.0);
}

#endif
