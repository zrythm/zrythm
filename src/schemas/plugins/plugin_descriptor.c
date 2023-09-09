// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor.h"
#include "utils/objects.h"

#include "schemas/plugins/plugin_descriptor.h"

PluginDescriptor *
plugin_descriptor_upgrade_from_v1 (PluginDescriptor_v1 * old)
{
  if (!old)
    return NULL;

  PluginDescriptor * self = object_new (PluginDescriptor);

  self->schema_version = PLUGIN_DESCRIPTOR_SCHEMA_VERSION;
  self->author = old->author;
  self->name = old->name;
  self->website = old->website;
  self->category = (ZPluginCategory) old->category;
  self->category_str = old->category_str;
  self->num_audio_ins = old->num_audio_ins;
  self->num_audio_outs = old->num_audio_outs;
  self->num_midi_ins = old->num_midi_ins;
  self->num_midi_outs = old->num_midi_outs;
  self->num_cv_ins = old->num_cv_ins;
  self->num_cv_outs = old->num_cv_outs;
  self->num_ctrl_ins = old->num_ctrl_ins;
  self->num_ctrl_outs = old->num_ctrl_outs;
  self->unique_id = old->unique_id;
  self->arch = (PluginArchitecture) old->arch;
  self->protocol = (ZPluginProtocol) old->protocol;
  self->path = old->path;
  self->uri = old->uri;
  self->min_bridge_mode = (CarlaBridgeMode) old->min_bridge_mode;
  self->has_custom_ui = old->has_custom_ui;
  self->ghash = old->ghash;

  return self;
}
