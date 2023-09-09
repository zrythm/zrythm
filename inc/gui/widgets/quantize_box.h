/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __GUI_WIDGETS_QUANTIZE_BOX_H__
#define __GUI_WIDGETS_QUANTIZE_BOX_H__

#include <gtk/gtk.h>

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
typedef struct SnapGrid        SnapGrid;
typedef struct QuantizeOptions QuantizeOptions;

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
