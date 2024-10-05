// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_KNOB_WITH_NAME_H__
#define __GUI_WIDGETS_KNOB_WITH_NAME_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

#define KNOB_WITH_NAME_WIDGET_TYPE (knob_with_name_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  KnobWithNameWidget,
  knob_with_name_widget,
  Z,
  KNOB_WITH_NAME_WIDGET,
  GtkBox)

typedef struct _KnobWidget          KnobWidget;
typedef struct _EditableLabelWidget EditableLabelWidget;

/**
 * A vertical box with a knob at the top and a name
 * at the bottom.
 */
typedef struct _KnobWithNameWidget
{
  GtkBox parent_instance;

  /** The label to show below the knob. */
  // GtkLabel *     label;
  EditableLabelWidget * label;

  /** The knob. */
  KnobWidget * knob;

  /** Right click menu. */
  GtkPopoverMenu * popover_menu;
} KnobWithNameWidget;

/**
 * Returns a new instance.
 *
 * @param label_before Whether to show the label
 *   before the knob.
 */
KnobWithNameWidget *
knob_with_name_widget_new (
  void *              obj,
  GenericStringGetter name_getter,
  GenericStringSetter name_setter,
  KnobWidget *        knob,
  GtkOrientation      orientation,
  bool                label_before,
  int                 spacing);

#endif
