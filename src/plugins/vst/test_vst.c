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

#include "plugins/vst_plugin.h"

#include <gtk/gtk.h>
int
main (int    argc,
      char **argv)
{
  PluginDescriptor * descr =
    vst_plugin_create_descriptor_from_path (
      argv[1], 1);

  if (!descr)
    return -1;

  /* sleep to wait for crashes */
  g_usleep (10000);

  g_message (
    "zrythm_vst_check: Plugin %s passed",
    argv[1]);

  return 0;
}
