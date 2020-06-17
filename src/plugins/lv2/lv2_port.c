/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_port.h"
#include "plugins/plugin_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

/**
 * Returns the Lv2Port corresponding to the given
 * symbol.
 *
 * TODO: Build an index to make this faster,
 * currently O(n) which may be
 * a problem when restoring the state of plugins
 * with many ports.
 */
Lv2Port*
lv2_port_get_by_symbol (
  Lv2Plugin* plugin,
  const char* sym)
{
  for (int i = 0; i < plugin->num_ports; ++i)
    {
      Lv2Port* const port =
        &plugin->ports[i];
      const char * port_sym =
        lv2_port_get_symbol_as_string (
          plugin, port);

      if (!strcmp (port_sym, sym))
        return port;
  }

  return NULL;
}


/**
 * Function to get a port's value from its string
 * symbol.
 *
 * Used when saving the state.
 * This function MUST set size and type
 * appropriately.
 */
const void *
lv2_port_get_value_from_symbol (
  const char * port_sym,
  void       * user_data,
  uint32_t   * size,
  uint32_t   * type)
{
  Lv2Plugin * lv2_plugin = (Lv2Plugin *) user_data;

  Lv2Port * port =
    lv2_port_get_by_symbol (
      lv2_plugin, port_sym);
  *size = 0;
  *type = 0;

  if (port)
    {
      *size = sizeof (float);
      *type = PM_URIDS.atom_Float;
      g_return_val_if_fail (port->port, NULL);
      return (const void *) &port->port->control;
    }

  return NULL;
}

/**
 * Gets the symbol as a string.
 */
const char *
lv2_port_get_symbol_as_string (
  Lv2Plugin * plugin,
  Lv2Port *   port)
{
  const LilvNode* sym =
    lilv_port_get_symbol (
      plugin->lilv_plugin, port->lilv_port);
  return lilv_node_as_string (sym);
}
