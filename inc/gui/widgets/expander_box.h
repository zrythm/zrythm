// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou
// <alex@zrythm.org> SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#define EXPANDER_BOX_WIDGET_TYPE (expander_box_widget_get_type ())
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
expander_box_widget_get_private (ExpanderBoxWidget * self);

/**
 * Sets the label to show.
 */
void
expander_box_widget_set_label (ExpanderBoxWidget * self, const char * label);

/**
 * Sets the icon name to show.
 */
void
expander_box_widget_set_icon_name (
  ExpanderBoxWidget * self,
  const char *        icon_name);

void
expander_box_widget_add_content (ExpanderBoxWidget * self, GtkWidget * content);

/**
 * Reveals or hides the expander box's contents.
 */
void
expander_box_widget_set_reveal (ExpanderBoxWidget * self, int reveal);

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
expander_box_widget_set_vexpand (ExpanderBoxWidget * self, bool expand);

ExpanderBoxWidget *
expander_box_widget_new (
  const char *   label,
  const char *   icon_name,
  GtkOrientation orientation);

/**
 * @}
 */

#endif
