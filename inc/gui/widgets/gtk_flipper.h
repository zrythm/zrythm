// SPDX-FileCopyrightText: © 2021 Benjamin Otte
// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2021 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __GTK_FLIPPER_H__
#define __GTK_FLIPPER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_FLIPPER (gtk_flipper_get_type ())

G_DECLARE_FINAL_TYPE (
  GtkFlipper,
  gtk_flipper,
  GTK,
  FLIPPER,
  GtkWidget)

GtkWidget *
gtk_flipper_new (GtkWidget * child);

GtkWidget *
gtk_flipper_get_child (GtkFlipper * self);
void
gtk_flipper_set_child (
  GtkFlipper * self,
  GtkWidget *  child);
gboolean
gtk_flipper_get_flip_horizontal (GtkFlipper * self);
void
gtk_flipper_set_flip_horizontal (
  GtkFlipper * self,
  gboolean     flip_horizontal);
gboolean
gtk_flipper_get_flip_vertical (GtkFlipper * self);
void
gtk_flipper_set_flip_vertical (
  GtkFlipper * self,
  gboolean     flip_vertical);
gboolean
gtk_flipper_get_rotate (GtkFlipper * self);
void
gtk_flipper_set_rotate (
  GtkFlipper * self,
  gboolean     rotate);

G_END_DECLS

#endif /* __GTK_FLIPPER_H__ */
