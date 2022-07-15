/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UTILS_RESOURCES_H__
#define __UTILS_RESOURCES_H__

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * \file
 *
 * Helpers for loading and using resources such as
 * icons.
 */

#define RESOURCES_PATH "/org/zrythm/Zrythm/app"
#define RESOURCES_TEMPLATE_PATH RESOURCES_PATH "/ui"
#define RESOURCES_ICON_PATH RESOURCES_PATH "/icons"
#define RESOURCES_GL_SHADERS_PATH RESOURCES_PATH "/gl/shaders"

typedef enum IconType
{
  ICON_TYPE_ZRYTHM,
  ICON_TYPE_GNOME_BUILDER,
  ICON_TYPE_BREEZE,
  ICON_TYPE_FORK_AWESOME,
} IconType;

/**
 * Creates a GtkImage of from the given information
 * and returns it as a GtkWidget.
 *
 * @return a GtkImage.
 */
GtkWidget *
resources_get_icon (IconType icon_type, const char * filename);

void
resources_set_image_icon (
  GtkImage *   img,
  IconType     icon_type,
  const char * path);

/**
 * Sets class template from resource.
 *
 * Filename is part after .../ui/
 */
void
resources_set_class_template (
  GtkWidgetClass * klass,
  const char *     filename);

void
resources_add_icon_to_button (
  GtkButton *  btn,
  IconType     icon_type,
  const char * path);

/**
 * Returns the bytes of the specified OpenGL shader
 * in RESOURCES_GL_SHADERS_PATH.
 *
 * Caller must free the bytes with g_bytes_unref ().
 *
 * @return bytes or NULL if error.
 */
GBytes *
resources_get_gl_shader_data (const char * path);

/**
 * @}
 */

#endif
