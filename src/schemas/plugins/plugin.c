// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin.h"
#include "utils/objects.h"
#include "schemas/plugins/plugin.h"

Plugin *
plugin_upgrade_from_v1 (
  Plugin_v1 * old)
{
  if (!old)
    return NULL;

  Plugin * self = object_new (Plugin);

  self->schema_version = PLUGIN_SCHEMA_VERSION;
  PluginIdentifier * id = plugin_identifier_upgrade_from_v1 (&old->id);
  self->id = *id;
  self->setting = plugin_setting_upgrade_from_v1 (old->setting);
  self->num_in_ports = old->num_in_ports;
  self->in_ports = g_malloc_n ((size_t) self->num_in_ports, sizeof (Port *));
  for (int i = 0; i < old->num_in_ports; i++)
    {
      self->in_ports[i] = port_upgrade_from_v1 (old->in_ports[i]);
    }
  self->num_out_ports = old->num_out_ports;
  self->out_ports = g_malloc_n ((size_t) self->num_out_ports, sizeof (Port *));
  for (int i = 0; i < old->num_out_ports; i++)
    {
      self->out_ports[i] = port_upgrade_from_v1 (old->out_ports[i]);
    }
  self->num_banks = old->num_banks;
  self->banks = g_malloc_n ((size_t) self->num_banks, sizeof (PluginPresetIdentifier *));
  for (int i = 0; i < old->num_banks; i++)
    {
      self->banks[i] = plugin_bank_upgrade_from_v1 (old->banks[i]);
    }
  PluginPresetIdentifier * selected_bank = plugin_preset_identifier_upgrade_from_v1 (&old->selected_bank);
  self->selected_bank = *selected_bank;
  PluginPresetIdentifier * selected_preset = plugin_preset_identifier_upgrade_from_v1 (&old->selected_preset);
  self->selected_preset = *selected_preset;
  self->visible = old->visible;
  self->state_dir = old->state_dir;

  return self;
}
