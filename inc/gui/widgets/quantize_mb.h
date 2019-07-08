/*
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Quantize menu button.
 */

#ifndef __GUI_WIDGETS_QUANTIZE_MB_H__
#define __GUI_WIDGETS_QUANTIZE_MB_H__

#include <gtk/gtk.h>

#define QUANTIZE_MB_WIDGET_TYPE \
  (quantize_mb_widget_get_type ())
G_DECLARE_FINAL_TYPE (QuantizeMbWidget,
                      quantize_mb_widget,
                      Z,
                      QUANTIZE_MB_WIDGET,
                      GtkMenuButton)

typedef struct _QuantizeMbPopoverWidget QuantizeMbPopoverWidget;
typedef struct Quantize Quantize;

typedef struct _QuantizeMbWidget
{
  GtkMenuButton           parent_instance;
  GtkBox *                box; ///< the box
  GtkImage *              img; ///< img to show next to the label
  GtkLabel *              label; ///< label to show
  QuantizeMbPopoverWidget * popover; ///< the popover to show
  GtkBox *                content; ///< popover content holder
  Quantize *              quantize;
} QuantizeMbWidget;

void
quantize_mb_widget_refresh (
  QuantizeMbWidget * self);

void
quantize_mb_widget_setup (QuantizeMbWidget * self,
                        Quantize * quantize);

#endif
