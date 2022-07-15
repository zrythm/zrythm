/*
 * gui/accelerators.h - accel
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __GUI_ACCEL_H__
#define __GUI_ACCEL_H__

#include <gtk/gtk.h>

void
accel_install_action_accelerator (
  const char * primary,
  const char * secondary,
  const char * action_name);

/**
 * Install accelerator for an action.
 */
#define accel_install_primary_action_accelerator( \
  primary, action_name) \
  accel_install_action_accelerator ( \
    primary, NULL, action_name)

/**
 * Returns the primary accelerator for the given
 * action.
 */
char *
accel_get_primary_accel_for_action (const char * action_name);

#if 0
void
accel_set_accel_label_from_action (
  GtkAccelLabel * accel_label,
  const char *    action_name);
#endif

#endif
