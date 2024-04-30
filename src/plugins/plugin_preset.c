/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "plugins/plugin_preset.h"
#include "utils/objects.h"

void
plugin_preset_identifier_init (PluginPresetIdentifier * id)
{
}

PluginBank *
plugin_bank_new (void)
{
  PluginBank * self = object_new (PluginBank);

  plugin_preset_identifier_init (&self->id);

  return self;
}

PluginPreset *
plugin_preset_new (void)
{
  PluginPreset * self = object_new (PluginPreset);

  plugin_preset_identifier_init (&self->id);

  return self;
}
