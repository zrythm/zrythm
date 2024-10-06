// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/channel.h"
#include "common/plugins/carla_native_plugin.h"

void
ChannelSend::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("slot", slot_), make_field ("amount", amount_),
    make_field ("enabled", enabled_), make_field ("isSidechain", is_sidechain_),
    make_field ("midiIn", midi_in_, true),
    make_field ("stereoIn", stereo_in_, true),
    make_field ("midiOut", midi_out_, true),
    make_field ("stereoOut", stereo_out_, true),
    make_field ("trackNameHash", track_name_hash_));
}

void
Fader::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("type", type_), make_field ("volume", volume_),
    make_field ("amp", amp_), make_field ("phase", phase_),
    make_field ("balance", balance_), make_field ("mute", mute_),
    make_field ("solo", solo_), make_field ("listen", listen_),
    make_field ("monoCompatEnabled", mono_compat_enabled_),
    make_field ("swapPhase", swap_phase_), make_field ("midiIn", midi_in_, true),
    make_field ("midiOut", midi_out_, true),
    make_field ("stereoIn", stereo_in_, true),
    make_field ("stereoOut", stereo_out_, true),
    make_field ("midiMode", midi_mode_),
    make_field ("passthrough", passthrough_));
}

void
Channel::define_fields (const Context &ctx)
{
  using T = ISerializable<Channel>;

  if (ctx.is_serializing ())
    {
      T::serialize_field<decltype (midi_fx_), zrythm::plugins::PluginPtrVariant> (
        "midiFx", midi_fx_, ctx);
      T::serialize_field<decltype (inserts_), zrythm::plugins::PluginPtrVariant> (
        "inserts", inserts_, ctx);
      T::serialize_field ("sends", sends_, ctx);
      T::serialize_field<
        decltype (instrument_), zrythm::plugins::PluginPtrVariant> (
        "instrument", instrument_, ctx, true);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);

      auto handle_plugin = [&] (yyjson_val * pl_obj, auto &plugin) {
        if (pl_obj == nullptr || yyjson_is_null (pl_obj))
          {
            plugin = nullptr;
          }
        else
          {
            auto          pl_it = yyjson_obj_iter_with (pl_obj);
            auto          setting_obj = yyjson_obj_iter_get (&pl_it, "setting");
            PluginSetting setting;
            setting.deserialize (Context (setting_obj, ctx));
            auto pl = zrythm::plugins::Plugin::create_unique_from_hosting_type (
              setting.hosting_type_);
            std::visit (
              [&] (auto &&p) {
                using PluginT = base_type<decltype (p)>;
                p->ISerializable<PluginT>::deserialize (Context (pl_obj, ctx));
              },
              convert_to_variant<zrythm::plugins::PluginPtrVariant> (pl.get ()));
            plugin = std::move (pl);
          }
      };

      auto handle_plugin_array = [&] (const auto &key, auto &plugins) {
        yyjson_val * arr = yyjson_obj_iter_get (&it, key);
        if (!arr)
          {
            throw ZrythmException ("No plugins array");
          }
        yyjson_arr_iter pl_arr_it = yyjson_arr_iter_with (arr);
        yyjson_val *    pl_obj = NULL;
        auto            size = yyjson_arr_size (arr);
        for (size_t i = 0; i < size; ++i)
          {
            pl_obj = yyjson_arr_iter_next (&pl_arr_it);
            handle_plugin (pl_obj, plugins[i]);
          }
      };

      handle_plugin_array ("midiFx", midi_fx_);
      handle_plugin_array ("inserts", inserts_);
      deserialize_field (it, "sends", sends_, ctx);

      auto instrument_obj = yyjson_obj_iter_get (&it, "instrument");
      handle_plugin (instrument_obj, instrument_);
    }

  T::serialize_fields (
    ctx, T::make_field ("prefader", prefader_), T::make_field ("fader", fader_),
    T::make_field ("midiOut", midi_out_, true),
    T::make_field ("stereoOut", stereo_out_, true),
    T::make_field ("hasOutput", has_output_),
    T::make_field ("outputNameHash", output_name_hash_),
    T::make_field ("trackPos", track_pos_));

  if (ctx.is_serializing ())
    {
      if (!all_midi_ins_)
        {
          T::serialize_field ("extMidiIns", ext_midi_ins_, ctx);
        }
      T::serialize_field ("allMidiIns", all_midi_ins_, ctx);
      if (!all_midi_channels_)
        {
          T::serialize_field ("midiChannels", midi_channels_, ctx);
        }
      T::serialize_field ("allMidiChannels", all_midi_channels_, ctx);
      if (!all_stereo_l_ins_)
        {
          T::serialize_field ("extStereoLIns", ext_stereo_l_ins_, ctx);
        }
      T::serialize_field ("allStereoLIns", all_stereo_l_ins_, ctx);
      if (!all_stereo_r_ins_)
        {
          T::serialize_field ("extStereoRIns", ext_stereo_r_ins_, ctx);
        }
      T::serialize_field ("allStereoRIns", all_stereo_r_ins_, ctx);
      T::serialize_field ("width", width_, ctx);
    }
  else
    {
      yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);
      T::deserialize_field (it, "extMidiIns", ext_midi_ins_, ctx, true);
      T::deserialize_field (it, "allMidiIns", all_midi_ins_, ctx);
      T::deserialize_field (it, "midiChannels", midi_channels_, ctx, true);
      T::deserialize_field (it, "allMidiChannels", all_midi_channels_, ctx);
      T::deserialize_field (it, "extStereoLIns", ext_stereo_l_ins_, ctx, true);
      T::deserialize_field (it, "allStereoLIns", all_stereo_l_ins_, ctx);
      T::deserialize_field (it, "extStereoRIns", ext_stereo_r_ins_, ctx, true);
      T::deserialize_field (it, "allStereoRIns", all_stereo_r_ins_, ctx);
      T::deserialize_field (it, "width", width_, ctx);
    }
}