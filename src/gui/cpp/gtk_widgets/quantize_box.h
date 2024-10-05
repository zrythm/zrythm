/*
 * SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 */

#ifndef __GUI_WIDGETS_QUANTIZE_BOX_H__
#define __GUI_WIDGETS_QUANTIZE_BOX_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define QUANTIZE_BOX_WIDGET_TYPE (quantize_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  QuantizeBoxWidget,
  quantize_box_widget,
  Z,
  QUANTIZE_BOX_WIDGET,
  GtkBox)

#define MW_QUANTIZE_BOX MW_TIMELINE_TOOLBAR->quantize_box

typedef struct _SnapGridWidget SnapGridWidget;
class SnapGrid;
class QuantizeOptions;

typedef struct _QuantizeBoxWidget
{
  GtkBox            parent_instance;
  GtkButton *       quick_quantize_btn;
  GtkButton *       quantize_opts_btn;
  QuantizeOptions * q_opts;
} QuantizeBoxWidget;

/**
 * Sets up the QuantizeBoxWidget.
 */
void
quantize_box_widget_setup (QuantizeBoxWidget * self, QuantizeOptions * q_opts);

/**
 * @}
 */

#endif
