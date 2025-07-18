// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/file_audio_source.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "engine/device_io/engine.h"
#include "engine/session/control_room.h"
#include "engine/session/midi_mapping.h"
#include "engine/session/recording_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/audio_region.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/fader.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/dsp.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"

#include <fmt/format.h>

namespace zrythm::structure::tracks
{

TrackProcessor::TrackProcessor (
  ProcessableTrack         &tr,
  ProcessorBaseDependencies dependencies)
    : dsp::ProcessorBase (
        dependencies,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("{} Processor", tr.get_name ()))),
      port_registry_ (dependencies.port_registry_),
      param_registry_ (dependencies.param_registry_), track_ (&tr)
{
  switch (tr.get_input_signal_type ())
    {
    case PortType::Event:
      init_midi_port (false);
      init_midi_port (true);

      /* set up piano roll port */
      if (tr.has_piano_roll () || tr.is_chord ())
        {
          add_input_port (port_registry_.create_object<dsp::MidiPort> (
            u8"TP Piano Roll", PortFlow::Input));
          auto * piano_roll = &get_piano_roll_port ();
          piano_roll->set_full_designation_provider (this);
          piano_roll->set_symbol (u8"track_processor_piano_roll");
          if (!tr.is_chord ())
            {
              init_midi_cc_ports ();
            }
        }
      break;
    case PortType::Audio:
      init_stereo_out_ports (false);
      init_stereo_out_ports (true);
      if (tr.get_type () == Track::Type::Audio)
        {
          mono_id_ = param_registry_.create_object<dsp::ProcessorParameter> (
            port_registry_,
            dsp::ProcessorParameter::UniqueId (
              u8"track_processor_mono_toggle"

              ),
            dsp::ParameterRange::make_toggle (false), u8"TP Mono Toggle");
          input_gain_id_ = param_registry_.create_object<dsp::ProcessorParameter> (
            port_registry_,
            dsp::ProcessorParameter::UniqueId (
              u8"track_processor_input_gain"

              ),
            dsp::ParameterRange (
              dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
            u8"TP Input Gain");
        }
      break;
    default:
      break;
    }

  if (tr.get_type () == Track::Type::Audio)
    {
      output_gain_id_ = param_registry_.create_object<dsp::ProcessorParameter> (
        port_registry_,
        dsp::ProcessorParameter::UniqueId (
          u8"track_processor_output_gain"

          ),
        dsp::ParameterRange (
          dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
        u8"TP Output Gain");

      monitor_audio_id_ = param_registry_.create_object<dsp::ProcessorParameter> (
        port_registry_,
        dsp::ProcessorParameter::UniqueId (
          u8"track_processor_monitor_audio"

          ),
        dsp::ParameterRange::make_toggle (false), u8"Monitor audio");
    }

  init_common ();
}

void
TrackProcessor::init_loaded (
  ProcessableTrack *               track,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  track_ = track;

  std::vector<dsp::Port *> ports;

// TODO
#if 0
  for (auto port : ports)
    {
      port->set_full_designation_provider (this);
    }
#endif

  init_common ();
}

void
TrackProcessor::init_common ()
{
  if (midi_cc_ids_)
    {
      cc_mappings_ =
        std::make_unique<engine::session::MidiMappings> (param_registry_);

      for (const auto i : std::views::iota (0, 16))
        {
          for (const auto j : std::views::iota (0, 128))
            {
              auto cc_port_id = (*midi_cc_ids_)[i * 128 + j];
              auto cc_port =
                cc_port_id.get_object_as<dsp::ProcessorParameter> ();

              /* set caches */
              // cc_port->midi_channel_ = i + 1;
              // cc_port->midi_cc_no_ = j;

              /* set model bytes for CC:
               * [0] = ctrl change + channel
               * [1] = controller
               * [2] (unused) = control */
              std::array<midi_byte_t, 3> buf{};
              buf[0] =
                (midi_byte_t) (utils::midi::MIDI_CH1_CTRL_CHANGE | (midi_byte_t) i);
              buf[1] = (midi_byte_t) j;
              buf[2] = 0;

              /* bind */
              cc_mappings_->bind_track (
                buf, { cc_port->get_uuid (), param_registry_ }, false);
            } /* endforeach 0..127 */

// TODO
#if 0
          auto pb_id = pitch_bend_ids_->at (i);
          auto kp_id = poly_key_pressure_ids_->at (i);
          auto cp_id = channel_pressure_ids_->at (i);
          auto pb = std::get<ControlPort *> (pb_id.get_object ());
          auto kp = std::get<ControlPort *> (kp_id.get_object ());
          auto cp = std::get<ControlPort *> (cp_id.get_object ());

          /* set caches */
          pb->midi_channel_ = i + 1;
          kp->midi_channel_ = i + 1;
          cp->midi_channel_ = i + 1;
#endif

        } /* endforeach 0..15 */

      updated_midi_automatable_ports_ =
        std::make_unique<MPMCQueue<dsp::ProcessorParameter *>> (128 * 16);
    }
}

void
TrackProcessor::init_midi_port (bool in)
{
  if (in)
    {
      add_input_port (port_registry_.create_object<dsp::MidiPort> (
        u8"TP MIDI in", dsp::PortFlow::Input));
      auto * midi_in = &get_midi_in_port ();
      midi_in->set_full_designation_provider (this);
      midi_in->set_symbol (u8"track_processor_midi_in");
    }
  else
    {
      add_output_port (port_registry_.create_object<dsp::MidiPort> (
        u8"TP MIDI out", dsp::PortFlow::Output));
      auto * midi_out = &get_midi_out_port ();
      midi_out->set_full_designation_provider (this);
      midi_out->set_symbol (u8"track_processor_midi_out");
    }
}

template <size_t... Is>
std::array<dsp::ProcessorParameterUuidReference, sizeof...(Is)>
make_port_uuid_reference_array (
  dsp::ProcessorParameterRegistry &param_registry,
  std::index_sequence<Is...>)
{
  return {
    ((void) Is, dsp::ProcessorParameterUuidReference (param_registry))...
  };
}

template <size_t N>
std::array<dsp::ProcessorParameterUuidReference, N>
make_port_uuid_reference_array (dsp::ProcessorParameterRegistry &param_registry)
{
  return make_port_uuid_reference_array (
    param_registry, std::make_index_sequence<N> ());
}

void
TrackProcessor::init_midi_cc_ports ()
{
  midi_cc_ids_ = std::make_unique<decltype (midi_cc_ids_)::element_type> (
    make_port_uuid_reference_array<2048> (param_registry_));
  pitch_bend_ids_ = std::make_unique<decltype (pitch_bend_ids_)::element_type> (
    make_port_uuid_reference_array<16> (param_registry_));
  poly_key_pressure_ids_ =
    std::make_unique<decltype (poly_key_pressure_ids_)::element_type> (
      make_port_uuid_reference_array<16> (param_registry_));
  channel_pressure_ids_ =
    std::make_unique<decltype (channel_pressure_ids_)::element_type> (
      make_port_uuid_reference_array<16> (param_registry_));

  for (const auto i : std::views::iota (0, 16))
    {
      /* starting from 1 */
      int channel = i + 1;

      for (const auto j : std::views::iota (0, 128))
        {
          (*midi_cc_ids_)[(i * 128) + j] = param_registry_.create_object<
            dsp::ProcessorParameter> (
            port_registry_,
            dsp::ProcessorParameter::UniqueId (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("midi_controller_ch{}_{}", channel, j + 1))),
            dsp::ParameterRange (
              dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format (
                "Ch{} {}", channel, utils::midi::midi_get_controller_name (j))));
        }

      (*pitch_bend_ids_)[i] = param_registry_.create_object<
        dsp::ProcessorParameter> (
        port_registry_,
        dsp::ProcessorParameter::UniqueId (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("ch{}_pitch_bend", i + 1))),
        dsp::ParameterRange (
          dsp::ParameterRange::Type::Integer, -8192.f, 8191.f, 0.f, 0.f),
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Ch{} Pitch bend", i + 1)));

      (*poly_key_pressure_ids_)[i] = param_registry_.create_object<
        dsp::ProcessorParameter> (
        port_registry_,
        dsp::ProcessorParameter::UniqueId (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("ch{}_poly_key_pressure", i + 1))),
        dsp::ParameterRange (
          dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Ch{} Poly key pressure", i + 1)));

      (*channel_pressure_ids_)[i] = param_registry_.create_object<
        dsp::ProcessorParameter> (
        port_registry_,
        dsp::ProcessorParameter::UniqueId (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("ch{}_channel_pressure", i + 1))),
        dsp::ParameterRange (
          dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Ch{} Channel pressure", i + 1)));
    }
}

void
TrackProcessor::init_stereo_out_ports (bool in)
{
  auto stereo_ports = dsp::StereoPorts::create_stereo_ports (
    port_registry_, in, in ? u8"TP Stereo in" : u8"TP Stereo out",
    utils::Utf8String (u8"track_processor_stereo_") + (in ? u8"in" : u8"out"));
  iterate_tuple (
    [&] (const auto &port_ref) {
      auto * port = std::get<dsp::AudioPort *> (port_ref.get_object ());
      port->set_full_designation_provider (this);
    },
    stereo_ports);

  if (in)
    {
      add_input_port (stereo_ports.first);
      add_input_port (stereo_ports.second);
    }
  else
    {
      add_output_port (stereo_ports.first);
      add_output_port (stereo_ports.second);
    }
}

bool
TrackProcessor::is_audio () const
{
  return track_->get_input_signal_type () == PortType::Audio;
}

bool
TrackProcessor::is_midi () const
{
  return track_->get_input_signal_type () == PortType::Event;
}

utils::Utf8String
TrackProcessor::get_full_designation_for_port (const dsp::Port &port) const
{
  auto * tr = get_track ();
  z_return_val_if_fail (tr, {});
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", tr->get_name (), port.get_label ()));
}

#if 0
void
TrackProcessor::on_control_change_event (
  const dsp::PortIdentifier::PortUuid &port_uuid,
  const dsp::PortIdentifier           &id,
  float                                value)
{
  if (ENUM_BITSET_TEST (id.flags_, PortIdentifier::Flags::MidiAutomatable))
    {
      auto port = PROJECT->find_port_by_id (port_uuid);
      z_return_if_fail (
        port.has_value ()
        && std::holds_alternative<ControlPort *> (port.value ()));
      updated_midi_automatable_ports_->push_back (
        std::get<ControlPort *> (port.value ()));
    }
}
#endif

#if 0
bool
TrackProcessor::should_sum_data_from_backend () const
{
  auto * track = get_track ();
  if (auto * recordable_track = dynamic_cast<RecordableTrack *> (track))
    {
      return recordable_track->get_recording ();
    }
  return false;
}
#endif

#if 0
bool
TrackProcessor::should_bounce_to_master (utils::audio::BounceStep step) const
{
  // only pre-insert bounces make sense for TrackProcessor outputs
  if (step != utils::audio::BounceStep::BeforeInserts)
    return false;

  auto * track = get_track ();
  return track->bounce_ && track->bounce_to_master_;
}
#endif

#if 0
bool
TrackProcessor::are_events_on_midi_channel_approved (midi_byte_t channel) const
{
  auto * track = dynamic_cast<ChannelTrack *> (get_track ());
  if (track == nullptr)
    return false;

  if (
    (track->is_midi () || track->is_instrument ())
    && track->channel_->midi_channels_.has_value ()
    && !track->channel_->midi_channels_->at (channel))
    {
      /* different channel */
      /*z_debug ("received event on different channel");*/
      return false;
    }
  return true;
}
#endif

void
init_from (
  TrackProcessor        &obj,
  const TrackProcessor  &other,
  utils::ObjectCloneType clone_type)
{
// TODO
#if 0
  init_from (
    static_cast<dsp::ProcessorBase &> (obj),
    static_cast<const dsp::ProcessorBase &> (other), clone_type);
#endif
  obj.mono_id_ = other.mono_id_;
  obj.input_gain_id_ = other.input_gain_id_;
  obj.output_gain_id_ = other.output_gain_id_;
  obj.monitor_audio_id_ = other.monitor_audio_id_;
  if (other.cc_mappings_)
    obj.cc_mappings_ = utils::clone_unique (
      *other.cc_mappings_, clone_type, obj.param_registry_);

  obj.midi_cc_ids_ = std::make_unique<decltype (obj.midi_cc_ids_)::element_type> (
    make_port_uuid_reference_array<2048> (obj.param_registry_));
  obj.pitch_bend_ids_ =
    std::make_unique<decltype (obj.pitch_bend_ids_)::element_type> (
      make_port_uuid_reference_array<16> (obj.param_registry_));
  obj.poly_key_pressure_ids_ =
    std::make_unique<decltype (obj.poly_key_pressure_ids_)::element_type> (
      make_port_uuid_reference_array<16> (obj.param_registry_));
  obj.channel_pressure_ids_ =
    std::make_unique<decltype (obj.channel_pressure_ids_)::element_type> (
      make_port_uuid_reference_array<16> (obj.param_registry_));

  *obj.midi_cc_ids_ = *other.midi_cc_ids_;
  *obj.pitch_bend_ids_ = *other.pitch_bend_ids_;
  *obj.poly_key_pressure_ids_ = *other.poly_key_pressure_ids_;
  *obj.channel_pressure_ids_ = *other.channel_pressure_ids_;

  obj.init_common ();
}

/**
 * Clears all buffers.
 */
void
TrackProcessor::clear_buffers (std::size_t block_length)
{
  if (is_audio ())
    {
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (block_length); },
        get_stereo_in_ports ());
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (block_length); },
        get_stereo_out_ports ());
    }
  else if (is_midi ())
    {
      get_midi_in_port ().clear_buffer (block_length);
      get_midi_out_port ().clear_buffer (block_length);
    }
  if (track_->has_piano_roll () || track_->is_chord ())
    {
      get_piano_roll_port ().clear_buffer (block_length);
    }
}

void
TrackProcessor::handle_recording (const EngineProcessTimeInfo &time_nfo)
{
  unsigned_frame_t split_points[6];
  nframes_t        each_nframes[6];
  int              num_split_points = 1;

  unsigned_frame_t start_frames = time_nfo.g_start_frame_w_offset_;
  unsigned_frame_t end_frames = time_nfo.g_start_frame_ + time_nfo.nframes_;

  /* split the cycle at loop and punch points and
   * record */
  bool loop_hit = false;
  bool punch_in_hit = false;
  split_points[0] = start_frames;
  each_nframes[0] = time_nfo.nframes_;
  if (TRANSPORT->loop_)
    {
      if (
        (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames () == end_frames)
        {
          loop_hit = true;
          num_split_points = 3;

          /* adjust start slot until loop end */
          each_nframes[0] =
            (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ()
            - start_frames;

          /* add loop end pause */
          split_points[1] =
            (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ();
          each_nframes[1] = 0;

          /* add part after looping */
          split_points[2] =
            (unsigned_frame_t) TRANSPORT->loop_start_pos_->getFrames ();
          each_nframes[2] =
            (unsigned_frame_t) time_nfo.nframes_ - each_nframes[0];
        }
    }
  if (TRANSPORT->punch_mode_)
    {
      if (loop_hit)
        {
          /* before loop */
          if (
            TRANSPORT->punch_in_pos_->is_between_frames_excluding_2nd (
              (signed_frame_t) start_frames,
              TRANSPORT->loop_end_pos_->getFrames ()))
            {
              punch_in_hit = true;
              num_split_points = 4;

              /* move loop start to next slot */
              each_nframes[3] = each_nframes[2];
              split_points[3] = split_points[2];
              each_nframes[2] = each_nframes[1];
              split_points[2] = split_points[1];

              /* add punch in pos */
              split_points[1] =
                (unsigned_frame_t) TRANSPORT->punch_in_pos_->getFrames ();
              each_nframes[1] =
                (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ()
                - (unsigned_frame_t) TRANSPORT->punch_in_pos_->getFrames ();

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (
            TRANSPORT->punch_out_pos_->is_between_frames_excluding_2nd (
              (signed_frame_t) start_frames,
              TRANSPORT->loop_end_pos_->getFrames ()))
            {
              if (punch_in_hit)
                {
                  num_split_points = 6;

                  /* move loop start to next slot */
                  each_nframes[5] = each_nframes[3];
                  split_points[5] = split_points[3];
                  each_nframes[4] = each_nframes[2];
                  split_points[4] = split_points[2];

                  /* add punch out pos */
                  split_points[2] =
                    (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();
                  each_nframes[2] =
                    (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ()
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();

                  /* add pause */
                  split_points[3] = split_points[2] + each_nframes[2];
                  each_nframes[3] = 0;

                  /* adjust num frames for punch in
                   * pos */
                  each_nframes[1] -= each_nframes[2];
                }
              else
                {
                  num_split_points = 5;

                  /* move loop start to next slot */
                  each_nframes[4] = each_nframes[2];
                  split_points[4] = split_points[2];
                  each_nframes[3] = each_nframes[1];
                  split_points[3] = split_points[1];

                  /* add punch out pos */
                  split_points[1] =
                    (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();
                  each_nframes[1] =
                    (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ()
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();

                  /* add pause */
                  split_points[2] = split_points[1] + each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -= each_nframes[1];
                }
            }
        }
      else /* loop not hit */
        {
          if (TRANSPORT->punch_in_pos_->is_between_frames_excluding_2nd (
                (signed_frame_t) start_frames, (signed_frame_t) end_frames))
            {
              punch_in_hit = true;
              num_split_points = 2;

              /* add punch in pos */
              split_points[1] =
                (unsigned_frame_t) TRANSPORT->punch_in_pos_->getFrames ();
              each_nframes[1] =
                end_frames
                - (unsigned_frame_t) TRANSPORT->punch_in_pos_->getFrames ();

              /* adjust num frames for initial
               * pos */
              each_nframes[0] -= each_nframes[1];
            }
          if (TRANSPORT->punch_out_pos_->is_between_frames_excluding_2nd (
                (signed_frame_t) start_frames, (signed_frame_t) end_frames))
            {
              if (punch_in_hit)
                {
                  num_split_points = 4;

                  /* add punch out pos */
                  split_points[2] =
                    (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();
                  each_nframes[2] =
                    end_frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();

                  /* add pause */
                  split_points[3] = split_points[2] + each_nframes[2];
                  each_nframes[3] = 0;

                  /* adjust num frames for punch in
                   * pos */
                  each_nframes[1] -= each_nframes[2];
                }
              else
                {
                  num_split_points = 3;

                  /* add punch out pos */
                  split_points[1] =
                    (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();
                  each_nframes[1] =
                    end_frames
                    - (unsigned_frame_t) TRANSPORT->punch_out_pos_->getFrames ();

                  /* add pause */
                  split_points[2] = split_points[1] + each_nframes[1];
                  each_nframes[2] = 0;

                  /* adjust num frames for init
                   * pos */
                  each_nframes[0] -= each_nframes[1];
                }
            }
        }
    }

  unsigned_frame_t split_point = 0;
  for (int i = 0; i < num_split_points; i++)
    {
      /* skip if same as previous point */
      if (i != 0 && split_points[i] == split_point)
        continue;

      split_point = split_points[i];

      EngineProcessTimeInfo cur_time_nfo = {
        .g_start_frame_ = split_point,
        .g_start_frame_w_offset_ = split_point,
        .local_offset_ = 0,
        .nframes_ = each_nframes[i],
      };
      RECORDING_MANAGER->handle_recording (this, &cur_time_nfo);
    }
}

void
TrackProcessor::add_events_from_midi_cc_control_ports (
  const nframes_t local_offset)
{
// TODO
#if 0
  ControlPort * cc;
  auto         &midi_out = get_midi_out_port ();
  while (updated_midi_automatable_ports_->pop_front (cc))
    {
      /*port_identifier_print (&cc->id_);*/
      if (
        ENUM_BITSET_TEST (cc->id_->flags_, PortIdentifier::Flags::MidiPitchBend))
        {
          midi_out.midi_events_.queued_events_.add_pitchbend (
            cc->midi_channel_,
            utils::math::round_to_signed_32 (cc->control_) + 0x2000,
            local_offset);
        }
      else if (
        ENUM_BITSET_TEST (
          cc->id_->flags_, PortIdentifier::Flags::MidiPolyKeyPressure))
        {
#  if ZRYTHM_TARGET_VER_MAJ > 1
          /* TODO - unsupported in v1 */
#  endif
        }
      else if (
        ENUM_BITSET_TEST (
          cc->id_->flags_, PortIdentifier::Flags::MidiChannelPressure))
        {
          midi_out.midi_events_.queued_events_.add_channel_pressure (
            cc->midi_channel_,
            (midi_byte_t) utils::math::round_to_signed_32 (cc->control_ * 127.f),
            local_offset);
        }
      else
        {
          midi_out.midi_events_.queued_events_.add_control_change (
            cc->midi_channel_, cc->midi_cc_no_,
            (midi_byte_t) utils::math::round_to_signed_32 (cc->control_ * 127.f),
            local_offset);
        }
    }
#endif
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
TrackProcessor::custom_process_block (EngineProcessTimeInfo time_nfo)
{
  z_return_if_fail (track_);

  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;

      /* if frozen or disabled, skip */
      if (tr->is_frozen () || !tr->is_enabled ())
        {
          return;
        }

      /* set the audio clip contents to stereo out */
      if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          const auto stereo_ports = get_stereo_out_ports ();
          tr->fill_events (
            time_nfo,
            std::make_pair (
              std::span (stereo_ports.first.buf_),
              std::span (stereo_ports.second.buf_)));
        }

      /* set the piano roll contents to midi out */
      if constexpr (
        std::derived_from<TrackT, PianoRollTrack>
        || std::is_same_v<TrackT, ChordTrack>)
        {
          auto &pr = get_piano_roll_port ();

          /* panic MIDI if necessary */
          if (AUDIO_ENGINE->panic_.load ())
            {
              pr.midi_events_.queued_events_.panic ();
            }
          /* get events from track if playing */
          else if (TRANSPORT->isRolling () || tr->is_auditioner ())
            {
            /* fill midi events from piano roll data */
#if 0
          z_info (
            "filling midi events for %s from %ld", tr->name, time_nfo->g_start_frame_w_offset);
#endif
              track_->fill_midi_events (
                time_nfo, pr.midi_events_.queued_events_);
            }
          pr.midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);
#if 0
      if (pr->midi_events->num_events > 0)
        {
          z_info (
            "%s piano roll has %d events",
            tr->name,
            pr->midi_events->num_events);
        }
#endif

          /* append midi events from modwheel and pitchbend control ports to
           * MIDI out */
          if (!tr->is_chord ())
            {
              add_events_from_midi_cc_control_ports (time_nfo.local_offset_);
            }
          if (get_midi_out_port ().midi_events_.active_events_.has_any ())
            {
#if 0
          z_debug (
            "%s midi processor out has %d events",
            tr->name, self->midi_out->midi_events_->num_events);
#endif
            }

          /* append the midi events from piano roll to MIDI out */
          get_midi_out_port ().midi_events_.active_events_.append (
            pr.midi_events_.active_events_, time_nfo.local_offset_,
            time_nfo.nframes_);

#if 0
      if (pr->midi_events->num_events > 0)
        {
          z_info ("PR");
          midi_events_print (pr->midi_events_, F_NOT_QUEUED);
        }

      if (self->midi_out->midi_events_->num_events > 0)
        {
          z_info ("midi out");
          midi_events_print (self->midi_out->midi_events_, F_NOT_QUEUED);
        }
#endif
        } /* if has piano roll or is chord track */

      /* if currently active track on the piano roll, fetch events */
      if (
        tr->get_input_signal_type () == dsp::PortType::Event
        && CLIP_EDITOR->has_region ())
        {
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
// TODO
#if 0
              auto clip_editor_track_var = CLIP_EDITOR->get_track ();
              if (
                clip_editor_track_var.has_value ()
                && std::get_if<TrackT *> (&clip_editor_track_var.value ())
                && std::get<TrackT *> (clip_editor_track_var.value ()) == tr)
                {
                  /* if not set to "all channels", filter-append */
                  if (tr->channel_->midi_channels_.has_value ())
                    {
                      get_midi_in_port ()
                        .midi_events_.active_events_.append_w_filter (
                          AUDIO_ENGINE->midi_editor_manual_press_->midi_events_
                            .active_events_,
                          tr->channel_->midi_channels_, time_nfo.local_offset_,
                          time_nfo.nframes_);
                    }
                  /* otherwise append normally */
                  else
                    {
                      get_midi_in_port ().midi_events_.active_events_.append (
                        AUDIO_ENGINE->midi_editor_manual_press_->midi_events_
                          .active_events_,
                        time_nfo.local_offset_, time_nfo.nframes_);
                    }
                }
#endif
            }
        }

      /* add inputs to outputs */
      if (tr->get_input_signal_type () == PortType::Audio)
        {
          const auto &stereo_in = get_stereo_in_ports ();
          const auto &stereo_out = get_stereo_out_ports ();
          const auto  input_gain = [this] () {
            const auto &input_gain_param = get_input_gain_param ();
            return input_gain_param.range ().convert_from_0_to_1 (
              input_gain_param.currentValue ());
          };
          const auto mono = [this] () {
            const auto &mono_param = get_mono_param ();
            return mono_param.range ().is_toggled (mono_param.currentValue ());
          };
          const auto monitor_audio = [this] () {
            const auto &monitor_audio_param = get_monitor_audio_param ();
            return monitor_audio_param.range ().is_toggled (
              monitor_audio_param.currentValue ());
          };

          if (
            tr->get_type () != Track::Type::Audio
            || (tr->get_type () == Track::Type::Audio && monitor_audio ()))
            {
              utils::float_ranges::mix_product (
                &stereo_out.first.buf_[time_nfo.local_offset_],
                &stereo_in.first.buf_[time_nfo.local_offset_],
                input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);

              if (mono_id_ && mono ())
                {
                  utils::float_ranges::mix_product (
                    &stereo_out.second.buf_[time_nfo.local_offset_],
                    &stereo_in.first.buf_[time_nfo.local_offset_],
                    input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);
                }
              else
                {
                  utils::float_ranges::mix_product (
                    &stereo_out.second.buf_[time_nfo.local_offset_],
                    &stereo_in.second.buf_[time_nfo.local_offset_],
                    input_gain_id_ ? input_gain () : 1.f, time_nfo.nframes_);
                }
            }
        }
      else if (tr->get_input_signal_type () == PortType::Event)
        {
          /* change the MIDI channel on the midi input to the channel set on the
           * track
           */
          if constexpr (
            std::derived_from<TrackT, PianoRollTrack>
            && !std::is_same_v<TrackT, ChordTrack>)
            {
              if (!tr->passthrough_midi_input_)
                {
                  get_midi_in_port ().midi_events_.active_events_.set_channel (
                    tr->midi_ch_);
                }
            }

          /* process midi bindings */
          if (cc_mappings_ && TRANSPORT->recording_)
            {
              cc_mappings_->apply_from_cc_events (
                get_midi_in_port ().midi_events_.active_events_);
            }

          /* if chord track, transform MIDI input to appropriate MIDI notes */
          if (tr->is_chord ())
            {
              get_midi_out_port ()
                .midi_events_.active_events_.transform_chord_and_append (
                  get_midi_in_port ().midi_events_.active_events_,
                  [] (midi_byte_t note_number) {
                    return CHORD_EDITOR->get_chord_from_note_number (
                      note_number);
                  },
                  arrangement::MidiNote::DEFAULT_VELOCITY,
                  time_nfo.local_offset_, time_nfo.nframes_);
            }
          /* else if not chord track, simply pass the input MIDI data to the
           * output port */
          else
            {
              get_midi_out_port ().midi_events_.active_events_.append (
                get_midi_in_port ().midi_events_.active_events_,
                time_nfo.local_offset_, time_nfo.nframes_);
            }

          /* if pending a panic message, append it */
          if (pending_midi_panic_)
            {
              get_midi_out_port ()
                .midi_events_.active_events_.panic_without_lock ();
              pending_midi_panic_ = false;
            }
        }

      if (
        !tr->is_auditioner () && TRANSPORT->preroll_frames_remaining_ == 0
        && (std::derived_from<TrackT, RecordableTrack> || !tr->automatableTrackMixin()->automationTracklist()->automation_tracks().empty ()))
        {
          /* handle recording. this will only create events in regions. it
           * will not copy the input content to the output ports. this will
           * also create automation for MIDI CC, if any (see
           * midi_mappings_apply_cc_events above) */
          handle_recording (time_nfo);
        }

      /* apply output gain */
      if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          const auto &output_gain_param = get_output_gain_param ();
          const auto  output_gain =
            output_gain_param.range ().convert_from_0_to_1 (
              output_gain_param.currentValue ());

          const auto stereo_out = get_stereo_out_ports ();
          utils::float_ranges::mul_k2 (
            &stereo_out.first.buf_[time_nfo.local_offset_], output_gain,
            time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_out.second.buf_[time_nfo.local_offset_], output_gain,
            time_nfo.nframes_);
        }
    },
    convert_to_variant<ProcessableTrackPtrVariant> (track_));
}

// ============================================================================

#if 0
/**
 * Copy port values from @ref src to @ref dest.
 */
void
track_processor_copy_values (TrackProcessor * dest, TrackProcessor * src)
{
  if (src->input_gain)
    {
      dest->input_gain->control_ = src->input_gain->control_;
    }
  if (src->monitor_audio)
    {
      dest->monitor_audio->control_ = src->monitor_audio->control_;
    }
  if (src->output_gain)
    {
      dest->output_gain->control_ = src->output_gain->control_;
    }
  if (src->mono)
    {
      dest->mono->control_ = src->mono->control_;
    }
}
#endif

void
from_json (const nlohmann::json &j, TrackProcessor &tp)
{
// TODO
#if 0
  j.at (TrackProcessor::kMonoKey).get_to (tp.mono_id_);
  j.at (TrackProcessor::kInputGainKey).get_to (tp.input_gain_id_);
  j.at (TrackProcessor::kOutputGainKey).get_to (tp.output_gain_id_);
  j.at (TrackProcessor::kMidiInKey).get_to (tp.midi_in_id_);
  j.at (TrackProcessor::kMidiOutKey).get_to (tp.midi_out_id_);
  j.at (TrackProcessor::kPianoRollKey).get_to (tp.piano_roll_id_);
  j.at (TrackProcessor::kMonitorAudioKey).get_to (tp.monitor_audio_id_);
  j.at (TrackProcessor::kStereoInLKey).get_to (tp.stereo_in_left_id_);
  j.at (TrackProcessor::kStereoInRKey).get_to (tp.stereo_in_right_id_);
  j.at (TrackProcessor::kStereoOutLKey).get_to (tp.stereo_out_left_id_);
  j.at (TrackProcessor::kStereoOutRKey).get_to (tp.stereo_out_right_id_);
  if (j.contains (TrackProcessor::kMidiCcKey))
    {
      j.at (TrackProcessor::kMidiCcKey).get_to (tp.midi_cc_ids_);
      j.at (TrackProcessor::kPitchBendKey).get_to (tp.pitch_bend_ids_);
      j.at (TrackProcessor::kPolyKeyPressureKey)
        .get_to (tp.poly_key_pressure_ids_);
      j.at (TrackProcessor::kChannelPressureKey)
        .get_to (tp.channel_pressure_ids_);
    }
#endif
}
}
