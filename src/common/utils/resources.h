// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_RESOURCES_H__
#define __UTILS_RESOURCES_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * @file
 *
 * Helpers for loading and using resources such as icons.
 */

#define RESOURCES_PATH_TOP "/org/zrythm/Zrythm"
#define RESOURCES_PATH RESOURCES_PATH_TOP "/app"
#define RESOURCES_TEMPLATE_PATH RESOURCES_PATH "/ui"
#define RESOURCES_GL_SHADERS_PATH RESOURCES_PATH "/gl/shaders"

/**
 * Sets class template from resource.
 *
 * Filename is part after .../ui/
 */
void
resources_set_class_template (GtkWidgetClass * klass, const char * filename);

/**
 * Returns the bytes of the specified OpenGL shader in RESOURCES_GL_SHADERS_PATH.
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
