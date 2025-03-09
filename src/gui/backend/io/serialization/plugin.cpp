// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_collections.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/plugin_descriptor.h"

#include <yyjson.h>

using namespace zrythm::gui::old_dsp::plugins;

void
PluginDescriptor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("author", author_, true), make_field ("name", name_, true),
    make_field ("website", website_, true), make_field ("category", category_),
    make_field ("categoryStr", category_str_, true),
    make_field ("numAudioIns", num_audio_ins_),
    make_field ("numAudioOuts", num_audio_outs_),
    make_field ("numMidiIns", num_midi_ins_),
    make_field ("numMidiOuts", num_midi_outs_),
    make_field ("numCtrlIns", num_ctrl_ins_),
    make_field ("numCtrlOuts", num_ctrl_outs_),
    make_field ("numCvIns", num_cv_ins_),
    make_field ("numCvOuts", num_cv_outs_), make_field ("uniqueId", unique_id_),
    make_field ("architecture", arch_), make_field ("protocol", protocol_),
    make_field ("path", path_, true), make_field ("uri", uri_, true),
    make_field ("minBridgeMode", min_bridge_mode_),
    make_field ("hasCustomUI", has_custom_ui_), make_field ("ghash", ghash_),
    make_field ("sha1", sha1_, true));
}

void
PluginSetting::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("desriptor", descr_),
    make_field ("openWithCarla", open_with_carla_),
    make_field ("forceGenericUI", force_generic_ui_),
    make_field ("bridgeMode", bridge_mode_));
}

void
PluginSettings::define_fields (const Context &ctx)
{
  using T = ISerializable<PluginSettings>;
  T::serialize_fields (ctx, make_field ("settings", settings_));
}

void
Plugin::PresetIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("index", idx_), make_field ("bankIndex", bank_idx_),
    make_field ("pluginId", plugin_id_));
}

void
Plugin::Preset::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("uri", uri_, true),
    make_field ("carlaProgram", carla_program_), make_field ("id", id_));
}

void
Plugin::Bank::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("presets", presets_),
    make_field ("uri", uri_, true), make_field ("id", id_));
}

void
Plugin::define_base_fields (const Context &ctx)
{
  using T = ISerializable<zrythm::gui::old_dsp::plugins::Plugin>;

  T::serialize_fields (
    ctx, T::make_field ("id", id_), T::make_field ("setting", setting_),
    T::make_field ("inPorts", in_ports_),
    T::make_field ("outPorts", out_ports_), T::make_field ("banks", banks_),
    T::make_field ("selectedBank", selected_bank_),
    T::make_field ("selectedPreset", selected_preset_),
    T::make_field ("visible", visible_),
    T::make_field ("stateDir", state_dir_, true));
}

void
PluginCollection::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_),
    make_field ("description", description_, true),
    make_field ("descriptors", descriptors_));
}

void
PluginCollections::define_fields (const Context &ctx)
{
  serialize_fields (ctx, make_field ("collections", collections_));
}

void
CarlaNativePlugin::define_fields (const Context &ctx)
{
  ISerializable<CarlaNativePlugin>::call_all_base_define_fields<
    zrythm::gui::old_dsp::plugins::Plugin> (ctx);
}
