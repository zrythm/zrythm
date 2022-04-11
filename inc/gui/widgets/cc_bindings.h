/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * CC Bindings matrix.
 */

#ifndef __GUI_WIDGETS_CC_BINDINGS_H__
#define __GUI_WIDGETS_CC_BINDINGS_H__

#include <gtk/gtk.h>

typedef struct _CcBindingsTreeWidget
  CcBindingsTreeWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CC_BINDINGS_WIDGET_TYPE \
  (cc_bindings_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CcBindingsWidget,
  cc_bindings_widget,
  Z,
  CC_BINDINGS_WIDGET,
  GtkBox)

/**
 * Left dock widget.
 */
typedef struct _CcBindingsWidget
{
  GtkBox parent_instance;

  CcBindingsTreeWidget * bindings_tree;
} CcBindingsWidget;

CcBindingsWidget *
cc_bindings_widget_new (void);

void
cc_bindings_widget_refresh (
  CcBindingsWidget * self);

/**
 * @}
 */

#endif
