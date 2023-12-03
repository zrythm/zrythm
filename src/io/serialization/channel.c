// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel.h"
#include "io/serialization/channel.h"
#include "io/serialization/extra.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"

typedef enum
{
  Z_IO_SERIALIZATION_CHANNEL_ERROR_FAILED,
} ZIOSerializationChannelError;

#define Z_IO_SERIALIZATION_CHANNEL_ERROR \
  z_io_serialization_channel_error_quark ()
GQuark
z_io_serialization_channel_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - channel - error - quark, z_io_serialization_channel_error)

bool
channel_send_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    cs_obj,
  const ChannelSend * cs,
  GError **           error)
{
  yyjson_mut_obj_add_int (doc, cs_obj, "slot", cs->slot);
  yyjson_mut_val * amount_obj = yyjson_mut_obj_add_obj (doc, cs_obj, "amount");
  port_serialize_to_json (doc, amount_obj, cs->amount, error);
  yyjson_mut_val * enabled_obj = yyjson_mut_obj_add_obj (doc, cs_obj, "enabled");
  port_serialize_to_json (doc, enabled_obj, cs->enabled, error);
  yyjson_mut_obj_add_bool (doc, cs_obj, "isSidechain", cs->is_sidechain);
  if (cs->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "midiIn");
      port_serialize_to_json (doc, midi_in_obj, cs->midi_in, error);
    }
  if (cs->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "stereoIn");
      stereo_ports_serialize_to_json (doc, stereo_in_obj, cs->stereo_in, error);
    }
  if (cs->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "midiOut");
      port_serialize_to_json (doc, midi_out_obj, cs->midi_out, error);
    }
  if (cs->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "stereoOut");
      stereo_ports_serialize_to_json (
        doc, stereo_out_obj, cs->stereo_out, error);
    }
  yyjson_mut_obj_add_uint (doc, cs_obj, "trackNameHash", cs->track_name_hash);
  return true;
}

bool
fader_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * f_obj,
  const Fader *    f,
  GError **        error)
{
  yyjson_mut_obj_add_int (doc, f_obj, "type", f->type);
  yyjson_mut_obj_add_real (doc, f_obj, "volume", f->volume);
  yyjson_mut_val * amp_obj = yyjson_mut_obj_add_obj (doc, f_obj, "amp");
  port_serialize_to_json (doc, amp_obj, f->amp, error);
  yyjson_mut_obj_add_real (doc, f_obj, "phase", f->phase);
  yyjson_mut_val * balance_obj = yyjson_mut_obj_add_obj (doc, f_obj, "balance");
  port_serialize_to_json (doc, balance_obj, f->balance, error);
  yyjson_mut_val * mute_obj = yyjson_mut_obj_add_obj (doc, f_obj, "mute");
  port_serialize_to_json (doc, mute_obj, f->mute, error);
  yyjson_mut_val * solo_obj = yyjson_mut_obj_add_obj (doc, f_obj, "solo");
  port_serialize_to_json (doc, solo_obj, f->solo, error);
  yyjson_mut_val * listen_obj = yyjson_mut_obj_add_obj (doc, f_obj, "listen");
  port_serialize_to_json (doc, listen_obj, f->listen, error);
  yyjson_mut_val * mono_compat_enabled_obj =
    yyjson_mut_obj_add_obj (doc, f_obj, "monoCompatEnabled");
  port_serialize_to_json (
    doc, mono_compat_enabled_obj, f->mono_compat_enabled, error);
  yyjson_mut_val * swap_phase_obj =
    yyjson_mut_obj_add_obj (doc, f_obj, "swapPhase");
  port_serialize_to_json (doc, swap_phase_obj, f->swap_phase, error);
  if (f->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "midiIn");
      port_serialize_to_json (doc, midi_in_obj, f->midi_in, error);
    }
  if (f->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "midiOut");
      port_serialize_to_json (doc, midi_out_obj, f->midi_out, error);
    }
  if (f->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "stereoIn");
      stereo_ports_serialize_to_json (doc, stereo_in_obj, f->stereo_in, error);
    }
  if (f->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "stereoOut");
      stereo_ports_serialize_to_json (doc, stereo_out_obj, f->stereo_out, error);
    }
  yyjson_mut_obj_add_int (doc, f_obj, "midiMode", f->midi_mode);
  yyjson_mut_obj_add_bool (doc, f_obj, "passthrough", f->passthrough);
  return true;
}

bool
channel_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * ch_obj,
  const Channel *  ch,
  GError **        error)
{
  yyjson_mut_val * midi_fx_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "midiFx");
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * pl = ch->midi_fx[i];
      if (pl)
        {
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, midi_fx_arr);
          plugin_serialize_to_json (doc, pl_obj, pl, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, midi_fx_arr);
        }
    }
  yyjson_mut_val * inserts_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "inserts");
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * pl = ch->inserts[i];
      if (pl)
        {
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, inserts_arr);
          plugin_serialize_to_json (doc, pl_obj, pl, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, inserts_arr);
        }
    }
  yyjson_mut_val * sends_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "sends");
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend *    cs = ch->sends[i];
      yyjson_mut_val * cs_obj = yyjson_mut_arr_add_obj (doc, sends_arr);
      channel_send_serialize_to_json (doc, cs_obj, cs, error);
    }
  if (ch->instrument)
    {
      yyjson_mut_val * pl_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "instrument");
      plugin_serialize_to_json (doc, pl_obj, ch->instrument, error);
    }
  yyjson_mut_val * prefader_obj =
    yyjson_mut_obj_add_obj (doc, ch_obj, "prefader");
  fader_serialize_to_json (doc, prefader_obj, ch->prefader, error);
  yyjson_mut_val * fader_obj = yyjson_mut_obj_add_obj (doc, ch_obj, "fader");
  fader_serialize_to_json (doc, fader_obj, ch->fader, error);
  if (ch->midi_out)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "midiOut");
      port_serialize_to_json (doc, port_obj, ch->midi_out, error);
    }
  if (ch->stereo_out)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "stereoOut");
      stereo_ports_serialize_to_json (doc, port_obj, ch->stereo_out, error);
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "hasOutput", ch->has_output);
  yyjson_mut_obj_add_uint (doc, ch_obj, "outputNameHash", ch->output_name_hash);
  yyjson_mut_obj_add_int (doc, ch_obj, "trackPos", ch->track_pos);
  if (!ch->all_midi_ins)
    {
      yyjson_mut_val * ext_midi_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extMidiIns");
      for (int i = 0; i < ch->num_ext_midi_ins; i++)
        {
          ExtPort *        ep = ch->ext_midi_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_midi_ins_arr);
          ext_port_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allMidiIns", ch->all_midi_ins);
  if (!ch->all_midi_channels)
    {
      yyjson_mut_val * midi_channels_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "midiChannels");
      for (int i = 0; i < 16; i++)
        {
          yyjson_mut_arr_add_bool (doc, midi_channels_arr, ch->midi_channels[i]);
        }
    }
  yyjson_mut_obj_add_bool (
    doc, ch_obj, "allMidiChannels", ch->all_midi_channels);
  if (!ch->all_stereo_l_ins)
    {
      yyjson_mut_val * ext_stereo_l_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extStereoLIns");
      for (int i = 0; i < ch->num_ext_stereo_l_ins; i++)
        {
          ExtPort *        ep = ch->ext_stereo_l_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_stereo_l_ins_arr);
          ext_port_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allStereoLIns", ch->all_stereo_l_ins);
  if (!ch->all_stereo_r_ins)
    {
      yyjson_mut_val * ext_stereo_r_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extStereoRIns");
      for (int i = 0; i < ch->num_ext_stereo_r_ins; i++)
        {
          ExtPort *        ep = ch->ext_stereo_r_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_stereo_r_ins_arr);
          ext_port_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allStereoRIns", ch->all_stereo_r_ins);
  yyjson_mut_obj_add_int (doc, ch_obj, "width", ch->width);
  return true;
}
