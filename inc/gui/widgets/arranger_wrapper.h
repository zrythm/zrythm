// clang-format off
// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Wrapper over arranger widget
 */

#ifndef __GUI_WIDGETS_ARRANGER_WRAPPER_H__
#define __GUI_WIDGETS_ARRANGER_WRAPPER_H__

#include "gui/widgets/arranger.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

#define ARRANGER_WRAPPER_WIDGET_TYPE (arranger_wrapper_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerWrapperWidget,
  arranger_wrapper_widget,
  Z,
  ARRANGER_WRAPPER_WIDGET,
  GtkWidget)

TYPEDEF_STRUCT_UNDERSCORED (ArrangerMinimapWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Wraps the arranger widget in a box with scrollbars.
 */
typedef struct _ArrangerWrapperWidget
{
  GtkWidget parent_instance;

  /** The right scrollbar will be overlayed on the arranger. */
  GtkOverlay * overlay;

  GtkScrollbar *          right_scrollbar;
  ArrangerWidget *        child;
  ArrangerMinimapWidget * minimap;

} ArrangerWrapperWidget;

void
arranger_wrapper_widget_setup (
  ArrangerWrapperWidget * self,
  ArrangerWidgetType      type,
  SnapGrid *              snap_grid);

/**
 * @}
 */

#endif
