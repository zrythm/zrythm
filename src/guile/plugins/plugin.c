/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "plugins/plugin.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_plugin_get_in_port,
  "plugin-get-in-port",
  2,
  0,
  0,
  (SCM plugin, SCM port_idx),
  "Returns the input port of the plugin at the given index.")
#define FUNC_NAME s_
{
  Plugin * pl = (Plugin *) scm_to_pointer (plugin);

  return scm_from_pointer (pl->in_ports[scm_to_int (port_idx)], NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_plugin_get_out_port,
  "plugin-get-out-port",
  2,
  0,
  0,
  (SCM plugin, SCM port_idx),
  "Returns the output port of the plugin at the given index.")
#define FUNC_NAME s_
{
  Plugin * pl = (Plugin *) scm_to_pointer (plugin);

  return scm_from_pointer (pl->out_ports[scm_to_int (port_idx)], NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "plugins_plugin.x"
#endif
  scm_c_export ("plugin-get-in-port", "plugin-get-out-port", NULL);
}

void
guile_plugins_plugin_define_module (void)
{
  scm_c_define_module ("plugins plugin", init_module, NULL);
}
