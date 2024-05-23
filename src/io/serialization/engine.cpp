// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "io/serialization/channel.h"
#include "io/serialization/engine.h"
#include "io/serialization/extra.h"
#include "io/serialization/port.h"
#include "utils/objects.h"

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
  yyjson_mut_obj_add_uint (doc, transport_obj, "position", transport->position);
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
  yyjson_mut_obj_add_int (doc, clip_obj, "bitDepth", (int64_t) clip->bit_depth);
  yyjson_mut_obj_add_bool (doc, clip_obj, "useFlac", clip->use_flac);
  yyjson_mut_obj_add_int (
    doc, clip_obj, "samplerate", (int64_t) clip->samplerate);
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
      AudioClip * clip = pool->clips[i];
      if (clip)
        {
          yyjson_mut_val * clip_obj = yyjson_mut_arr_add_obj (doc, clips_arr);
          audio_clip_serialize_to_json (doc, clip_obj, clip, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, clips_arr);
        }
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
    doc, engine_obj, "transportType", (int64_t) engine->transport_type);
  yyjson_mut_obj_add_uint (doc, engine_obj, "sampleRate", engine->sample_rate);
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

bool
transport_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * transport_obj,
  Transport *  transport,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (transport_obj);
  transport->total_bars =
    yyjson_get_int (yyjson_obj_iter_get (&it, "totalBars"));
  yyjson_val * playhead_pos_obj = yyjson_obj_iter_get (&it, "playheadPos");
  position_deserialize_from_json (
    doc, playhead_pos_obj, &transport->playhead_pos, error);
  yyjson_val * cue_pos_obj = yyjson_obj_iter_get (&it, "cuePos");
  position_deserialize_from_json (doc, cue_pos_obj, &transport->cue_pos, error);
  yyjson_val * loop_start_pos_obj = yyjson_obj_iter_get (&it, "loopStartPos");
  position_deserialize_from_json (
    doc, loop_start_pos_obj, &transport->loop_start_pos, error);
  yyjson_val * loop_end_pos_obj = yyjson_obj_iter_get (&it, "loopEndPos");
  position_deserialize_from_json (
    doc, loop_end_pos_obj, &transport->loop_end_pos, error);
  yyjson_val * punch_in_pos_obj = yyjson_obj_iter_get (&it, "punchInPos");
  position_deserialize_from_json (
    doc, punch_in_pos_obj, &transport->punch_in_pos, error);
  yyjson_val * punch_out_pos_obj = yyjson_obj_iter_get (&it, "punchOutPos");
  position_deserialize_from_json (
    doc, punch_out_pos_obj, &transport->punch_out_pos, error);
  yyjson_val * range1_obj = yyjson_obj_iter_get (&it, "range1");
  position_deserialize_from_json (doc, range1_obj, &transport->range_1, error);
  yyjson_val * range2_obj = yyjson_obj_iter_get (&it, "range2");
  position_deserialize_from_json (doc, range2_obj, &transport->range_2, error);
  transport->has_range = yyjson_get_bool (yyjson_obj_iter_get (&it, "hasRange"));
  transport->position = yyjson_get_uint (yyjson_obj_iter_get (&it, "position"));
  yyjson_val * roll_obj = yyjson_obj_iter_get (&it, "roll");
  transport->roll = object_new (Port);
  port_deserialize_from_json (doc, roll_obj, transport->roll, error);
  yyjson_val * stop_obj = yyjson_obj_iter_get (&it, "stop");
  transport->stop = object_new (Port);
  port_deserialize_from_json (doc, stop_obj, transport->stop, error);
  yyjson_val * backward_obj = yyjson_obj_iter_get (&it, "backward");
  transport->backward = object_new (Port);
  port_deserialize_from_json (doc, backward_obj, transport->backward, error);
  yyjson_val * forward_obj = yyjson_obj_iter_get (&it, "forward");
  transport->forward = object_new (Port);
  port_deserialize_from_json (doc, forward_obj, transport->forward, error);
  yyjson_val * loop_toggle_obj = yyjson_obj_iter_get (&it, "loopToggle");
  transport->loop_toggle = object_new (Port);
  port_deserialize_from_json (
    doc, loop_toggle_obj, transport->loop_toggle, error);
  yyjson_val * rec_toggle_obj = yyjson_obj_iter_get (&it, "recToggle");
  transport->rec_toggle = object_new (Port);
  port_deserialize_from_json (doc, rec_toggle_obj, transport->rec_toggle, error);
  return true;
}

static bool
audio_clip_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * clip_obj,
  AudioClip *  clip,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (clip_obj);
  clip->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val * file_hash_obj = yyjson_obj_iter_get (&it, "fileHash");
  if (file_hash_obj)
    {
      clip->file_hash = g_strdup (yyjson_get_str (file_hash_obj));
    }
  clip->bpm = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "bpm"));
  clip->bit_depth =
    (BitDepth) yyjson_get_real (yyjson_obj_iter_get (&it, "bitDepth"));
  clip->use_flac = yyjson_get_bool (yyjson_obj_iter_get (&it, "useFlac"));
  clip->samplerate = yyjson_get_int (yyjson_obj_iter_get (&it, "samplerate"));
  clip->pool_id = yyjson_get_int (yyjson_obj_iter_get (&it, "poolId"));
  return true;
}

static bool
audio_pool_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * pool_obj,
  AudioPool *  pool,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pool_obj);
  yyjson_val *    clips_arr = yyjson_obj_iter_get (&it, "clips");
  pool->clips_size = yyjson_arr_size (clips_arr);
  if (pool->clips_size > 0)
    {
      pool->clips = object_new_n (pool->clips_size, AudioClip *);
      yyjson_arr_iter clips_it = yyjson_arr_iter_with (clips_arr);
      yyjson_val *    clip_obj = NULL;
      while ((clip_obj = yyjson_arr_iter_next (&clips_it)))
        {
          if (yyjson_is_null (clip_obj))
            {
              pool->num_clips++;
              continue;
            }

          AudioClip * clip = object_new (AudioClip);
          pool->clips[pool->num_clips++] = clip;
          audio_clip_deserialize_from_json (doc, clip_obj, clip, error);
        }
    }
  return true;
}

static bool
control_room_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  cr_obj,
  ControlRoom * cr,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (cr_obj);
  yyjson_val *    monitor_fader_obj = yyjson_obj_iter_get (&it, "monitorFader");
  cr->monitor_fader = object_new (Fader);
  fader_deserialize_from_json (doc, monitor_fader_obj, cr->monitor_fader, error);
  return true;
}

static bool
sample_processor_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sp_obj,
  SampleProcessor * sp,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sp_obj);
  yyjson_val *    fader_obj = yyjson_obj_iter_get (&it, "fader");
  sp->fader = object_new (Fader);
  fader_deserialize_from_json (doc, fader_obj, sp->fader, error);
  return true;
}

static bool
hardware_processor_deserialize_from_json (
  yyjson_doc *        doc,
  yyjson_val *        hp_obj,
  HardwareProcessor * hp,
  GError **           error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (hp_obj);
  hp->is_input = yyjson_get_bool (yyjson_obj_iter_get (&it, "isInput"));
  yyjson_val * ext_audio_ports_arr = yyjson_obj_iter_get (&it, "extAudioPorts");
  hp->ext_audio_ports_size = yyjson_arr_size (ext_audio_ports_arr);
  if (hp->ext_audio_ports_size > 0)
    {
      hp->ext_audio_ports = object_new_n (hp->ext_audio_ports_size, ExtPort *);
      yyjson_arr_iter ext_audio_ports_it =
        yyjson_arr_iter_with (ext_audio_ports_arr);
      yyjson_val * ext_audio_port_obj = NULL;
      while ((ext_audio_port_obj = yyjson_arr_iter_next (&ext_audio_ports_it)))
        {
          ExtPort * ext_audio_port = object_new (ExtPort);
          hp->ext_audio_ports[hp->num_ext_audio_ports++] = ext_audio_port;
          ext_port_deserialize_from_json (
            doc, ext_audio_port_obj, ext_audio_port, error);
        }
    }
  yyjson_val * ext_midi_ports_arr = yyjson_obj_iter_get (&it, "extMidiPorts");
  hp->ext_midi_ports_size = yyjson_arr_size (ext_midi_ports_arr);
  if (hp->ext_midi_ports_size > 0)
    {
      hp->ext_midi_ports = object_new_n (hp->ext_midi_ports_size, ExtPort *);
      yyjson_arr_iter ext_midi_ports_it =
        yyjson_arr_iter_with (ext_midi_ports_arr);
      yyjson_val * ext_midi_port_obj = NULL;
      while ((ext_midi_port_obj = yyjson_arr_iter_next (&ext_midi_ports_it)))
        {
          ExtPort * ext_midi_port = object_new (ExtPort);
          hp->ext_midi_ports[hp->num_ext_midi_ports++] = ext_midi_port;
          ext_port_deserialize_from_json (
            doc, ext_midi_port_obj, ext_midi_port, error);
        }
    }
  yyjson_val * audio_ports_arr = yyjson_obj_iter_get (&it, "audioPorts");
  if (hp->ext_audio_ports_size > 0)
    {
      hp->audio_ports = object_new_n (hp->ext_audio_ports_size, Port *);
      yyjson_arr_iter audio_ports_it = yyjson_arr_iter_with (audio_ports_arr);
      yyjson_val *    audio_port_obj = NULL;
      while ((audio_port_obj = yyjson_arr_iter_next (&audio_ports_it)))
        {
          Port * audio_port = object_new (Port);
          hp->audio_ports[hp->num_audio_ports++] = audio_port;
          port_deserialize_from_json (doc, audio_port_obj, audio_port, error);
        }
    }
  yyjson_val * midi_ports_arr = yyjson_obj_iter_get (&it, "midiPorts");
  if (hp->ext_midi_ports_size > 0)
    {
      hp->midi_ports = object_new_n (hp->ext_midi_ports_size, Port *);
      yyjson_arr_iter midi_ports_it = yyjson_arr_iter_with (midi_ports_arr);
      yyjson_val *    midi_port_obj = NULL;
      while ((midi_port_obj = yyjson_arr_iter_next (&midi_ports_it)))
        {
          Port * midi_port = object_new (Port);
          hp->midi_ports[hp->num_midi_ports++] = midi_port;
          port_deserialize_from_json (doc, midi_port_obj, midi_port, error);
        }
    }
  return true;
}

bool
audio_engine_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  engine_obj,
  AudioEngine * engine,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (engine_obj);
  engine->transport_type = (AudioEngineJackTransportType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "transportType"));
  engine->sample_rate =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "sampleRate"));
  engine->frames_per_tick =
    yyjson_get_real (yyjson_obj_iter_get (&it, "framesPerTick"));
  yyjson_val * monitor_out_obj = yyjson_obj_iter_get (&it, "monitorOut");
  engine->monitor_out = object_new (StereoPorts);
  stereo_ports_deserialize_from_json (
    doc, monitor_out_obj, engine->monitor_out, error);
  yyjson_val * midi_editor_manual_press_obj =
    yyjson_obj_iter_get (&it, "midiEditorManualPress");
  engine->midi_editor_manual_press = object_new (Port);
  port_deserialize_from_json (
    doc, midi_editor_manual_press_obj, engine->midi_editor_manual_press, error);
  yyjson_val * midi_in_obj = yyjson_obj_iter_get (&it, "midiIn");
  engine->midi_in = object_new (Port);
  port_deserialize_from_json (doc, midi_in_obj, engine->midi_in, error);
  yyjson_val * transport_obj = yyjson_obj_iter_get (&it, "transport");
  engine->transport = object_new (Transport);
  transport_deserialize_from_json (doc, transport_obj, engine->transport, error);
  yyjson_val * pool_obj = yyjson_obj_iter_get (&it, "pool");
  engine->pool = object_new (AudioPool);
  audio_pool_deserialize_from_json (doc, pool_obj, engine->pool, error);
  yyjson_val * control_room_obj = yyjson_obj_iter_get (&it, "controlRoom");
  engine->control_room = object_new (ControlRoom);
  control_room_deserialize_from_json (
    doc, control_room_obj, engine->control_room, error);
  yyjson_val * sample_processor_obj =
    yyjson_obj_iter_get (&it, "sampleProcessor");
  engine->sample_processor = object_new (SampleProcessor);
  sample_processor_deserialize_from_json (
    doc, sample_processor_obj, engine->sample_processor, error);
  yyjson_val * hw_in_processor_obj = yyjson_obj_iter_get (&it, "hwInProcessor");
  engine->hw_in_processor = object_new (HardwareProcessor);
  hardware_processor_deserialize_from_json (
    doc, hw_in_processor_obj, engine->hw_in_processor, error);
  yyjson_val * hw_out_processor_obj =
    yyjson_obj_iter_get (&it, "hwOutProcessor");
  engine->hw_out_processor = object_new (HardwareProcessor);
  hardware_processor_deserialize_from_json (
    doc, hw_out_processor_obj, engine->hw_out_processor, error);
  return true;
}
