/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_DONATE_DIALOG_H__
#define __GUI_WIDGETS_DONATE_DIALOG_H__

#include <gtk/gtk.h>

#define DONATE_DIALOG_WIDGET_TYPE \
  (donate_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (DonateDialogWidget,
                      donate_dialog_widget,
                      Z,
                      DONATE_DIALOG_WIDGET,
                      GtkDialog)

typedef struct _DonateDialogWidget
{
  GtkDialog            parent_instance;
  GtkButton *          bitcoin;
  GtkButton *          paypal;
  GtkButton *          liberapay;
  GtkLabel *           link;
} DonateDialogWidget;

/**
 * Creates an donate dialog widget and displays it.
 */
DonateDialogWidget *
donate_dialog_widget_new ();

#endif
