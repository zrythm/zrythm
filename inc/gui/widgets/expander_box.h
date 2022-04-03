/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Expander box.
 */

#ifndef __GUI_WIDGETS_EXPANDER_BOX_H__
#define __GUI_WIDGETS_EXPANDER_BOX_H__

#include <stdbool.h>

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

typedef struct _GtkFlipper GtkFlipper;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Reveal callback prototype.
 */
typedef void (*ExpanderBoxRevealFunc) (
  ExpanderBoxWidget * expander_box,
  bool                revealed,
  void *              user_data);

/**
 * An expander box is a base widget with a button that
 * when clicked expands the contents.
 */
typedef struct
{
  GtkButton *   button;
  GtkBox *      btn_box;
  GtkLabel *    btn_label;
  GtkFlipper *  btn_label_flipper;
  GtkImage *    btn_img;
  GtkRevealer * revealer;
  GtkBox *      content;

  /** Horizontal or vertical. */
  GtkOrientation orientation;

  ExpanderBoxRevealFunc reveal_cb;

  void * user_data;

} ExpanderBoxWidgetPrivate;

typedef struct _ExpanderBoxWidgetClass
{
  GtkBoxClass parent_class;
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
    prv->btn_img, icon_type, path);
}

void
expander_box_widget_add_content (
  ExpanderBoxWidget * self,
  GtkWidget *         content);

/**
 * Reveals or hides the expander box's contents.
 */
void
expander_box_widget_set_reveal (
  ExpanderBoxWidget * self,
  int                 reveal);

void
expander_box_widget_set_reveal_callback (
  ExpanderBoxWidget *   self,
  ExpanderBoxRevealFunc cb,
  void *                user_data);

void
expander_box_widget_set_orientation (
  ExpanderBoxWidget * self,
  GtkOrientation      orientation);

void
expander_box_widget_set_vexpand (
  ExpanderBoxWidget * self,
  bool                expand);

ExpanderBoxWidget *
expander_box_widget_new (
  const char *   label,
  const char *   icon_name,
  GtkOrientation orientation);

/**
 * @}
 */

#endif
