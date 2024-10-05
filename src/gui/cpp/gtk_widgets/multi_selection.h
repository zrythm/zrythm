// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Multiple selection widget.
 */

#ifndef __GUI_WIDGETS_MULTI_SELECTION_H__
#define __GUI_WIDGETS_MULTI_SELECTION_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"
#include "common/utils/yaml.h"

#define MULTI_SELECTION_WIDGET_TYPE (multi_selection_widget_get_type ())
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

/**
 * Callback to call when any selection changes.
 */
typedef void (*MultiSelectionChangedCallback) (
  MultiSelectionWidget * multi_selection,
  const GArray *         selection_indices,
  void *                 object);

/**
 * A menu button that allows selecting active
 * hardware ports.
 */
typedef struct _MultiSelectionWidget
{
  GtkWidget parent_instance;

  /** Array of items as string labels. */
  GArray * item_strings;

  /** Array of selected item indices from
   * MultiSelectionWidget.item_strings. */
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
  const int                     num_items,
  MultiSelectionChangedCallback sel_changed_cb,
  const guint *                 selections,
  const int                     num_selections,
  void *                        object);

MultiSelectionWidget *
multi_selection_widget_new (void);

/**
 * @}
 */

#endif
