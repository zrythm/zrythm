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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#include "plugins/plugin_manager.h"
#include "project.h"
#endif

SCM_DEFINE (
  s_plugin_manager_find_plugin_from_uri,
  "plugin-manager-find-plugin-from-uri", 1, 0, 0,
  (SCM uri),
  "Returns the PluginDescriptor matching the given URI.")
{
  const PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (
      PLUGIN_MANAGER,
      scm_to_locale_string (uri));

  return
    scm_from_pointer ((void *) descr, NULL);
}

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#include "plugins_plugin_manager.x"
#endif

  scm_c_export (
    "plugin-manager-find-plugin-from-uri", NULL);
}

void
guile_plugins_plugin_manager_define_module (void)
{
  scm_c_define_module (
    "plugins plugin-manager", init_module, NULL);
}
