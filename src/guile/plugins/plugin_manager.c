/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "plugins/plugin_manager.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_plugin_manager_find_plugin_from_uri,
  "plugin-manager-find-plugin-from-uri",
  2,
  0,
  0,
  (SCM plugin_manager, SCM uri),
  "Returns the PluginDescriptor matching the given URI.")
#define FUNC_NAME s_
{
  PluginManager * pm = (PluginManager *) scm_to_pointer (plugin_manager);

  g_return_val_if_fail (pm, SCM_BOOL_F);

  const PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (pm, scm_to_locale_string (uri));

  if (descr)
    {
      return scm_from_pointer ((void *) descr, NULL);
    }
  else
    {
      return SCM_BOOL_F;
    }
}
#undef FUNC_NAME

SCM_DEFINE (
  s_plugin_manager_scan_plugins,
  "plugin-manager-scan-plugins",
  1,
  0,
  0,
  (SCM plugin_manager),
  "Scans the system for plugins.")
#define FUNC_NAME s_
{
  PluginManager * pm = (PluginManager *) scm_to_pointer (plugin_manager);

  plugin_manager_scan_plugins (pm, 1.0, NULL);

  return SCM_BOOL_T;
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "plugins_plugin_manager.x"
#endif

  scm_c_export (
    "plugin-manager-find-plugin-from-uri", "plugin-manager-scan-plugins", NULL);
}

void
guile_plugins_plugin_manager_define_module (void)
{
  scm_c_define_module ("plugins plugin-manager", init_module, NULL);
}
