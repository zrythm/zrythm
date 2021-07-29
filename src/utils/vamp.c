/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/vamp.h"

#include <gtk/gtk.h>

#include <vamp/vamp.h>
#include <vamp-hostsdk/host-c.h>

/**
 * Prints detected vamp plugins.
 */
void
vamp_print_all (void)
{
  int num_vamp_libs = vhGetLibraryCount ();
  g_message (
    "loading %d vamp libraries...", num_vamp_libs);
  for (int i = 0; i < num_vamp_libs; i++)
    {
      const char * lib_name = vhGetLibraryName (i);
      vhLibrary lib = vhLoadLibrary (i);
      int num_plugins = vhGetPluginCount (lib);
      g_message (
        "[%d-%s: %d plugins]",
        i, lib_name, num_plugins);
      for (int j = 0; j < num_plugins; j++)
        {
          const VampPluginDescriptor * descr =
            vhGetPluginDescriptor (lib, j);
          g_message (
            "\n[plugin %d: %s]\n"
            "name: %s\n"
            "description: %s\n"
            "maker: %s\n"
            "plugin version: %d\n"
            "copyright: %s\n"
            "param count: %u\n"
            "program count: %u\n"
            "vamp API version: %d",
            j, descr->identifier,
            descr->name,
            descr->description,
            descr->maker,
            descr->pluginVersion,
            descr->copyright,
            descr->parameterCount,
            descr->programCount,
            descr->vampApiVersion);
        }
      vhUnloadLibrary (lib);
    }
}
