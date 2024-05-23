// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/resources.h"

void
resources_set_class_template (GtkWidgetClass * klass, const char * filename)
{
  char path[500];
  sprintf (path, "%s/%s", RESOURCES_TEMPLATE_PATH, filename);
  gtk_widget_class_set_template_from_resource (klass, path);
}

GBytes *
resources_get_gl_shader_data (const char * path)
{
  GError * err = NULL;
  char *   str = g_strdup_printf ("%s/%s", RESOURCES_GL_SHADERS_PATH, path);
  GBytes * data = g_resources_lookup_data (str, (GResourceLookupFlags) 0, &err);

  if (err)
    {
      g_critical (
        "Failed to load gl shader data at <%s>: %s", path, err->message);
    }

  return data;
}
