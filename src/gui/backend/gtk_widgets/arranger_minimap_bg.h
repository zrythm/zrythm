// SPDX-FileCopyrightText: © 2019, 2022-2023 Alexandros Theodotou
// <alex@zrythm.org> SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Arranger minimap bg.
 */

#ifndef __GUI_WIDGETS_ARRANGER_MINIMAP_BG_H__
#define __GUI_WIDGETS_ARRANGER_MINIMAP_BG_H__

TYPEDEF_STRUCT_UNDERSCORED (ArrangerMinimapWidget);

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define ARRANGER_MINIMAP_BG_WIDGET_TYPE (arranger_minimap_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerMinimapBgWidget,
  arranger_minimap_bg_widget,
  Z,
  ARRANGER_MINIMAP_BG_WIDGET,
  GtkWidget)

typedef struct _ArrangerMinimapBgWidget
{
  GtkWidget parent_instance;

  ArrangerMinimapWidget * owner;
} ArrangerMinimapBgWidget;

ArrangerMinimapBgWidget *
arranger_minimap_bg_widget_new (ArrangerMinimapWidget * owner);

#endif
