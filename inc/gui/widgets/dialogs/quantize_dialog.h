/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * Quantize dialog.
 */

#ifndef __GUI_WIDGETS_QUANTIZE_DIALOG_H__
#define __GUI_WIDGETS_QUANTIZE_DIALOG_H__

#include <gtk/gtk.h>

#define QUANTIZE_DIALOG_WIDGET_TYPE (quantize_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  QuantizeDialogWidget,
  quantize_dialog_widget,
  Z,
  QUANTIZE_DIALOG_WIDGET,
  GtkDialog)

typedef struct QuantizeOptions     QuantizeOptions;
typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _BarSliderWidget    BarSliderWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _QuantizeDialogWidget
{
  GtkDialog            parent_instance;
  GtkButton *          cancel_btn;
  GtkButton *          quantize_btn;
  GtkBox *             note_length_box;
  GtkBox *             note_type_box;
  DigitalMeterWidget * note_length;
  DigitalMeterWidget * note_type;
  GtkToggleButton *    adjust_start;
  GtkToggleButton *    adjust_end;
  GtkBox *             amount_box;
  GtkBox *             swing_box;
  GtkBox *             randomization_box;
  BarSliderWidget *    amount;
  BarSliderWidget *    swing;
  BarSliderWidget *    randomization;

  QuantizeOptions * opts;
} QuantizeDialogWidget;

/**
 * Creates an quantize dialog widget and displays it.
 */
QuantizeDialogWidget *
quantize_dialog_widget_new (QuantizeOptions * opts);

/**
 * @}
 */

#endif
