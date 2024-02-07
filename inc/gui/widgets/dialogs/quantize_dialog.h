/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
