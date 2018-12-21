/*
 * utils/resources.h - resource utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __UTILS_RESOURCES_H__
#define __UTILS_RESOURCES_H__

#include <gtk/gtk.h>

#define RESOURCE_PATH "/org/zrythm/"
#define TEMPLATE_PATH "ui/"
#define ICON_PATH "icons/"

GtkWidget *
resources_get_icon (const char * filename); ///< the path after .../icons/

/**
 * Sets class template from resource.
 *
 * Filename is part after .../ui/
 */
void
resources_set_class_template (GtkWidgetClass * klass,
                              const char * filename);

void
resources_add_icon_to_button (GtkButton * btn,
                              const char * path);

#endif
