/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * The Zrythm GTK application.
 */

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include <gtk/gtk.h>

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (
  ZrythmApp, zrythm_app, ZRYTHM, APP,
  GtkApplication)

typedef struct Zrythm Zrythm;

/**
 * @addtogroup general
 *
 * @{
 */

/**
 * The global struct.
 *
 * Contains data that is only relevant to the GUI and
 * not to Zrythm.
 */
struct _ZrythmApp
{
  GtkApplication      parent;
};

/**
 * Global variable, should be available to all files.
 */
extern ZrythmApp * zrythm_app;

/**
 * Creates the Zrythm GApplication.
 *
 * This also initializes the Zrythm struct.
 */
ZrythmApp *
zrythm_app_new (void);

/**
 * @}
 */

#endif /* __ZRYTHM_APP_H__ */
