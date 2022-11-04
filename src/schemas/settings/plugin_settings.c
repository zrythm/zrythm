// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "settings/plugin_settings.h"
#include "utils/objects.h"

#include "schemas/settings/plugin_settings.h"

PluginSetting *
plugin_setting_upgrade_from_v1 (PluginSetting_v1 * old)
{
  if (!old)
    return NULL;

  PluginSetting * self = object_new (PluginSetting);

#define UPDATE(name) self->name = old->name

  self->schema_version = PLUGIN_SETTING_SCHEMA_VERSION;
  self->descr = plugin_descriptor_upgrade_from_v1 (old->descr);
  self->open_with_carla = old->open_with_carla;
  self->force_generic_ui = old->force_generic_ui;
  self->bridge_mode = (CarlaBridgeMode) old->bridge_mode;
  self->ui_uri = old->ui_uri;
  UPDATE (last_instantiated_time);
  UPDATE (num_instantiations);

  return self;
}
