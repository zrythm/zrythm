// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Arranger minimap selection.
 */

#ifndef __GUI_WIDGETS_ARRANGER_MINIMAP_SELECTION_H__
#define __GUI_WIDGETS_ARRANGER_MINIMAP_SELECTION_H__

#include "common/utils/ui.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

TYPEDEF_STRUCT_UNDERSCORED (ArrangerMinimapWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define ARRANGER_MINIMAP_SELECTION_WIDGET_TYPE \
  (arranger_minimap_selection_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerMinimapSelectionWidget,
  arranger_minimap_selection_widget,
  Z,
  ARRANGER_MINIMAP_SELECTION_WIDGET,
  GtkWidget)

typedef struct _ArrangerMinimapSelectionWidget
{
  GtkWidget parent_instance;

  UiCursorState cursor;

  /** Pointer back to parent. */
  ArrangerMinimapWidget * parent;
} ArrangerMinimapSelectionWidget;

ArrangerMinimapSelectionWidget *
arranger_minimap_selection_widget_new (ArrangerMinimapWidget * parent);

/**
 * @}
 */

#endif
