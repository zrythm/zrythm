// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/channel.h"
#include "gui/dsp/carla_native_plugin.h"

void
Fader::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("type", type_), make_field ("volume", volume_),
    make_field ("amp", amp_id_), make_field ("phase", phase_),
    make_field ("balance", balance_id_), make_field ("mute", mute_id_),
    make_field ("solo", solo_id_), make_field ("listen", listen_id_),
    make_field ("monoCompatEnabled", mono_compat_enabled_id_),
    make_field ("swapPhase", swap_phase_id_),
    make_field ("midiIn", midi_in_id_, true),
    make_field ("midiOut", midi_out_id_, true),
    make_field ("stereoInL", stereo_in_left_id_, true),
    make_field ("stereoInR", stereo_in_right_id_, true),
    make_field ("stereoOutL", stereo_out_left_id_, true),
    make_field ("stereoOutR", stereo_out_right_id_, true),
    make_field ("midiMode", midi_mode_),
    make_field ("passthrough", passthrough_));
}

void
zrythm::gui::Channel::define_fields (const Context &ctx)
{
  using T = ISerializable<Channel>;

  T::serialize_fields (
    ctx, T::make_field ("midiFx", midi_fx_),
    T::make_field ("inserts", inserts_), T::make_field ("sends", sends_),
    T::make_field ("instrument", instrument_, true),
    T::make_field ("prefader", prefader_), T::make_field ("fader", fader_),
    T::make_field ("midiOut", midi_out_id_, true),
    T::make_field ("stereoOutL", stereo_out_left_id_, true),
    T::make_field ("stereoOutR", stereo_out_right_id_, true),
    T::make_field ("outputUuid", output_track_uuid_),
    T::make_field ("trackPos", track_uuid_));

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
