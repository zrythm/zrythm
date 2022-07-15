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
 * Dialog for binding MIDI CC to a port.
 */

#ifndef __GUI_WIDGETS_BIND_CC_DIALOG_H__
#define __GUI_WIDGETS_BIND_CC_DIALOG_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define BIND_CC_DIALOG_WIDGET_TYPE \
  (bind_cc_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BindCcDialogWidget,
  bind_cc_dialog_widget,
  Z,
  BIND_CC_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The bind_cc dialog.
 */
typedef struct _BindCcDialogWidget
{
  GtkDialog   parent_instance;
  GtkButton * cancel_btn;
  GtkButton * ok_btn;
  GtkLabel *  lbl;

  /** A MIDI event for the CC selected, or all 0's if
   * none selected. */
  midi_byte_t cc[3];

  Port * port;

  /** Whether to perform an undoable action for
   * binding on close. */
  bool perform_action;
} BindCcDialogWidget;

/**
 * Creates an bind_cc dialog widget and displays it.
 */
BindCcDialogWidget *
bind_cc_dialog_widget_new (Port * port, bool perform_action);

/**
 * @}
 */

#endif
