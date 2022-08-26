// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Multiple selection widget.
 */

#ifndef __GUI_WIDGETS_MULTI_SELECTION_H__
#define __GUI_WIDGETS_MULTI_SELECTION_H__

#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

#define MULTI_SELECTION_WIDGET_TYPE \
  (multi_selection_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MultiSelectionWidget,
  multi_selection_widget,
  Z,
  MULTI_SELECTION_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef void (*MultiSelectionChangedCallback) (
  void * object,
  int    selection);

/**
 * A menu button that allows selecting active
 * hardware ports.
 */
typedef struct _MultiSelectionWidget
{
  GtkWidget parent_instance;

  /** Array of items as string labels. */
  GArray * item_strings;

  /** Array of selected items (subset of
   * MultiSelectionWidget.item_strings). */
  GArray * selections;

  /** Object to call callbacks with. */
  void * obj;

  MultiSelectionChangedCallback sel_changed_cb;

  GtkFlowBox * flow_box;

} MultiSelectionWidget;

void
multi_selection_widget_setup (
  MultiSelectionWidget *        self,
  const char **                 strings,
  const cyaml_strval_t *        cyaml_strings,
  const int                     num_items,
  MultiSelectionChangedCallback sel_changed_cb,
  const int *                   selections,
  const int                     num_selections,
  void *                        object);

MultiSelectionWidget *
multi_selection_widget_new (void);

/**
 * @}
 */

#endif
