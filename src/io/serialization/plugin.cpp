// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/control_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/collections.h"
#include "plugins/plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_identifier.h"

#include <yyjson.h>

void
PluginIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("slotType", slot_type_),
    make_field ("trackNameHash", track_name_hash_), make_field ("slot", slot_));
}

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
  using T = ISerializable<Plugin>;

  T::serialize_fields (
    ctx, T::make_field ("id", id_), T::make_field ("setting", setting_));

  if (ctx.is_serializing ())
    {
      T::make_field<decltype (in_ports_), PortPtrVariant> ("inPorts", in_ports_);
      T::make_field<decltype (out_ports_), PortPtrVariant> (
        "outPorts", out_ports_);
    }
  else
    {
      auto it = yyjson_obj_iter_with (ctx.obj_);
      auto handle_ports = [&] (const std::string &key, auto &ports) {
        yyjson_val * in_ports_arr = yyjson_obj_iter_get (&it, key.c_str ());
        size_t       in_ports_len = yyjson_arr_size (in_ports_arr);
        ports.clear ();
        ports.reserve (in_ports_len);
        for (size_t i = 0; i < in_ports_len; ++i)
          {
            yyjson_val * port_val = yyjson_arr_get (in_ports_arr, i);
            if (yyjson_is_obj (port_val))
              {
                auto elem_it = yyjson_obj_iter_with (port_val);
                auto port_id_obj = yyjson_obj_iter_get (&elem_it, "id");
                if (!yyjson_is_obj (port_id_obj))
                  {
                    throw ZrythmException ("Invalid port id");
                  }
                PortIdentifier port_id;
                Context        port_id_ctx (port_id_obj, ctx);
                port_id.deserialize (port_id_ctx);
                auto    port = Port::create_unique_from_type (port_id.type_);
                Context port_ctx (port_val, ctx);
                std::visit (
                  [&] (auto &&p) {
                    p->ISerializable<base_type<decltype (p)>>::deserialize (
                      port_ctx);
                  },
                  convert_to_variant<PortPtrVariant> (port.get ()));
              }
          }
      };
      handle_ports ("inPorts", in_ports_);
      handle_ports ("outPorts", out_ports_);
    }

  T::serialize_fields (
    ctx, T::make_field ("banks", banks_),
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
  ISerializable<CarlaNativePlugin>::call_all_base_define_fields<Plugin> (ctx);
}