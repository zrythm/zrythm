// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 */

#ifndef __GUI_WIDGETS_EDITABLE_LABEL_H__
#define __GUI_WIDGETS_EDITABLE_LABEL_H__

#include "utils/types.h"

#include "gtk_wrapper.h"

#define EDITABLE_LABEL_WIDGET_TYPE (editable_label_widget_get_type ())
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

  guint select_region_source_id;
} EditableLabelWidget;

/**
 * Shows the popover.
 */
void
editable_label_widget_show_popover (EditableLabelWidget * self);

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
