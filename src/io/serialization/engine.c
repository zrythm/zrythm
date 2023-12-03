// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "io/serialization/channel.h"
#include "io/serialization/engine.h"
#include "io/serialization/extra.h"
#include "io/serialization/port.h"

typedef enum
{
  Z_IO_SERIALIZATION_ENGINE_ERROR_FAILED,
} ZIOSerializationEngineError;

#define Z_IO_SERIALIZATION_ENGINE_ERROR z_io_serialization_engine_error_quark ()
GQuark
z_io_serialization_engine_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - engine - error - quark, z_io_serialization_engine_error)

bool
transport_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  transport_obj,
  const Transport * transport,
  GError **         error)
{
  yyjson_mut_obj_add_int (
    doc, transport_obj, "totalBars", transport->total_bars);
  yyjson_mut_val * playhead_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "playheadPos");
  position_serialize_to_json (
    doc, playhead_pos_obj, &transport->playhead_pos, error);
  yyjson_mut_val * cue_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "cuePos");
  position_serialize_to_json (doc, cue_pos_obj, &transport->cue_pos, error);
  yyjson_mut_val * loop_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopStartPos");
  position_serialize_to_json (
    doc, loop_start_pos_obj, &transport->loop_start_pos, error);
  yyjson_mut_val * loop_end_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopEndPos");
  position_serialize_to_json (
    doc, loop_end_pos_obj, &transport->loop_end_pos, error);
  yyjson_mut_val * punch_in_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "punchInPos");
  position_serialize_to_json (
    doc, punch_in_pos_obj, &transport->punch_in_pos, error);
  yyjson_mut_val * punch_out_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "punchOutPos");
  position_serialize_to_json (
    doc, punch_out_pos_obj, &transport->punch_out_pos, error);
  yyjson_mut_val * range_1_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "range1");
  position_serialize_to_json (doc, range_1_obj, &transport->range_1, error);
  yyjson_mut_val * range_2_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "range2");
  position_serialize_to_json (doc, range_2_obj, &transport->range_2, error);
  yyjson_mut_obj_add_bool (doc, transport_obj, "hasRange", transport->has_range);
  yyjson_mut_obj_add_int (doc, transport_obj, "position", transport->position);
  yyjson_mut_val * roll_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "roll");
  port_serialize_to_json (doc, roll_obj, transport->roll, error);
  yyjson_mut_val * stop_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "stop");
  port_serialize_to_json (doc, stop_obj, transport->stop, error);
  yyjson_mut_val * backward_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "backward");
  port_serialize_to_json (doc, backward_obj, transport->backward, error);
  yyjson_mut_val * forward_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "forward");
  port_serialize_to_json (doc, forward_obj, transport->forward, error);
  yyjson_mut_val * loop_toggle_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopToggle");
  port_serialize_to_json (doc, loop_toggle_obj, transport->loop_toggle, error);
  yyjson_mut_val * rec_toggle_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "recToggle");
  port_serialize_to_json (doc, rec_toggle_obj, transport->rec_toggle, error);
  return true;
}

static bool
audio_clip_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  clip_obj,
  const AudioClip * clip,
  GError **         error)
{
  yyjson_mut_obj_add_str (doc, clip_obj, "name", clip->name);
  if (clip->file_hash)
    {
      yyjson_mut_obj_add_str (doc, clip_obj, "fileHash", clip->file_hash);
    }
  yyjson_mut_obj_add_real (doc, clip_obj, "bpm", clip->bpm);
  yyjson_mut_obj_add_int (doc, clip_obj, "bitDepth", clip->bit_depth);
  yyjson_mut_obj_add_bool (doc, clip_obj, "useFlac", clip->use_flac);
  yyjson_mut_obj_add_int (doc, clip_obj, "samplerate", clip->samplerate);
  yyjson_mut_obj_add_int (doc, clip_obj, "poolId", clip->pool_id);
  return true;
}

static bool
audio_pool_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  pool_obj,
  const AudioPool * pool,
  GError **         error)
{
  yyjson_mut_val * clips_arr = yyjson_mut_obj_add_arr (doc, pool_obj, "clips");
  for (int i = 0; i < pool->num_clips; i++)
    {
      AudioClip *      clip = pool->clips[i];
      yyjson_mut_val * clip_obj = yyjson_mut_arr_add_obj (doc, clips_arr);
      audio_clip_serialize_to_json (doc, clip_obj, clip, error);
    }
  return true;
}

static bool
control_room_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    cr_obj,
  const ControlRoom * cr,
  GError **           error)
{
  yyjson_mut_val * monitor_fader_obj =
    yyjson_mut_obj_add_obj (doc, cr_obj, "monitorFader");
  fader_serialize_to_json (doc, monitor_fader_obj, cr->monitor_fader, error);
  return true;
}

static bool
sample_processor_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sp_obj,
  const SampleProcessor * sp,
  GError **               error)
{
  yyjson_mut_val * fader_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "fader");
  fader_serialize_to_json (doc, fader_obj, sp->fader, error);
  return true;
}

static bool
hardware_processor_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          hp_obj,
  const HardwareProcessor * hp,
  GError **                 error)
{
  yyjson_mut_obj_add_bool (doc, hp_obj, "isInput", hp->is_input);
  if (hp->ext_audio_ports)
    {
      yyjson_mut_val * ext_audio_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "extAudioPorts");
      for (int i = 0; i < hp->num_ext_audio_ports; i++)
        {
          ExtPort *        port = hp->ext_audio_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, ext_audio_ports_arr);
          ext_port_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->ext_midi_ports)
    {
      yyjson_mut_val * ext_midi_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "extMidiPorts");
      for (int i = 0; i < hp->num_ext_midi_ports; i++)
        {
          ExtPort *        port = hp->ext_midi_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, ext_midi_ports_arr);
          ext_port_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->audio_ports)
    {
      yyjson_mut_val * audio_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "audioPorts");
      for (int i = 0; i < hp->num_audio_ports; i++)
        {
          Port *           port = hp->audio_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, audio_ports_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->midi_ports)
    {
      yyjson_mut_val * midi_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "midiPorts");
      for (int i = 0; i < hp->num_midi_ports; i++)
        {
          Port *           port = hp->midi_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, midi_ports_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
    }
  return true;
}

bool
audio_engine_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    engine_obj,
  const AudioEngine * engine,
  GError **           error)
{
  yyjson_mut_obj_add_int (
    doc, engine_obj, "transportType", engine->transport_type);
  yyjson_mut_obj_add_int (doc, engine_obj, "sampleRate", engine->sample_rate);
  yyjson_mut_obj_add_real (
    doc, engine_obj, "framesPerTick", engine->frames_per_tick);
  yyjson_mut_val * monitor_out_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "monitorOut");
  stereo_ports_serialize_to_json (
    doc, monitor_out_obj, engine->monitor_out, error);
  yyjson_mut_val * midi_editor_manual_press_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "midiEditorManualPress");
  port_serialize_to_json (
    doc, midi_editor_manual_press_obj, engine->midi_editor_manual_press, error);
  yyjson_mut_val * midi_in_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "midiIn");
  port_serialize_to_json (doc, midi_in_obj, engine->midi_in, error);
  yyjson_mut_val * transport_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "transport");
  transport_serialize_to_json (doc, transport_obj, engine->transport, error);
  yyjson_mut_val * pool_obj = yyjson_mut_obj_add_obj (doc, engine_obj, "pool");
  audio_pool_serialize_to_json (doc, pool_obj, engine->pool, error);
  yyjson_mut_val * control_room_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "controlRoom");
  control_room_serialize_to_json (
    doc, control_room_obj, engine->control_room, error);
  yyjson_mut_val * sample_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "sampleProcessor");
  sample_processor_serialize_to_json (
    doc, sample_processor_obj, engine->sample_processor, error);
  yyjson_mut_val * hw_in_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "hwInProcessor");
  hardware_processor_serialize_to_json (
    doc, hw_in_processor_obj, engine->hw_in_processor, error);
  yyjson_mut_val * hw_out_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "hwOutProcessor");
  hardware_processor_serialize_to_json (
    doc, hw_out_processor_obj, engine->hw_out_processor, error);
  return true;
}
