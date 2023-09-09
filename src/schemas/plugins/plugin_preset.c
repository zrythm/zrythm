// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_preset.h"
#include "utils/objects.h"

#include "schemas/plugins/plugin_preset.h"

PluginPresetIdentifier *
plugin_preset_identifier_upgrade_from_v1 (PluginPresetIdentifier_v1 * old)
{
  if (!old)
    return NULL;

  PluginPresetIdentifier * self = object_new (PluginPresetIdentifier);
  self->schema_version = PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION;
  self->idx = old->idx;
  self->bank_idx = old->bank_idx;
  PluginIdentifier * plugin_id =
    plugin_identifier_upgrade_from_v1 (&old->plugin_id);
  self->plugin_id = *plugin_id;

  return self;
}

PluginPreset *
plugin_preset_upgrade_from_v1 (PluginPreset_v1 * old)
{
  if (!old)
    return NULL;

  PluginPreset * self = object_new (PluginPreset);
  self->schema_version = PLUGIN_PRESET_SCHEMA_VERSION;
  self->name = old->name;
  self->uri = old->uri;
  self->carla_program = old->carla_program;
  PluginPresetIdentifier * id =
    plugin_preset_identifier_upgrade_from_v1 (&old->id);
  self->id = *id;

  return self;
}

PluginBank *
plugin_bank_upgrade_from_v1 (PluginBank_v1 * old)
{
  if (!old)
    return NULL;

  PluginBank * self = object_new (PluginBank);
  self->schema_version = PLUGIN_BANK_SCHEMA_VERSION;
  self->num_presets = old->num_presets;
  self->presets_size = old->presets_size;
  self->presets = g_malloc_n (self->presets_size, sizeof (PluginPreset *));
  for (int i = 0; i < self->num_presets; i++)
    {
      self->presets[i] = plugin_preset_upgrade_from_v1 (old->presets[i]);
    }
  self->uri = old->uri;
  self->name = old->name;
  PluginPresetIdentifier * id =
    plugin_preset_identifier_upgrade_from_v1 (&old->id);
  self->id = *id;

  return self;
}
