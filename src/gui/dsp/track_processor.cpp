// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <utility>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/audio_track.h"
#include "gui/dsp/clip.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/control_room.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/fader.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_mapping.h"
#include "gui/dsp/midi_track.h"
#include "gui/dsp/port.h"
#include "gui/dsp/recording_manager.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"

#include <fmt/format.h>

TrackProcessor::TrackProcessor (const DeserializationDependencyHolder &dh)
    : TrackProcessor (
        dh.get<std::reference_wrapper<ProcessableTrack>> ().get (),
        dh.get<std::reference_wrapper<PortRegistry>> ().get (),
        false)
{
}

TrackProcessor::TrackProcessor (
  ProcessableTrack &tr,
  PortRegistry     &port_registry,
  bool              create_ports)
    : port_registry_ (port_registry), track_ (&tr)
{
  if (!create_ports)
    {
      // ports will be created when deserializing
      return;
    }

  switch (tr.in_signal_type_)
    {
    case PortType::Event:
      init_midi_port (false);
      init_midi_port (true);

      /* set up piano roll port */
      if (tr.has_piano_roll () || tr.is_chord ())
        {
          auto * piano_roll = port_registry_.create_object<MidiPort> (
            "TP Piano Roll", PortFlow::Input);
          piano_roll->set_owner (*this);
          piano_roll->id_->sym_ = "track_processor_piano_roll";
          piano_roll->id_->flags_ = PortIdentifier::Flags::PianoRoll;
          piano_roll_id_ = piano_roll->get_uuid ();
          if (!tr.is_chord ())
            {
              init_midi_cc_ports ();
            }
        }
      break;
    case PortType::Audio:
      init_stereo_out_ports (false);
      init_stereo_out_ports (true);
      if (tr.type_ == Track::Type::Audio)
        {
          auto * mono =
            port_registry_.create_object<ControlPort> ("TP Mono Toggle");
          mono->set_owner (*this);
          mono->id_->sym_ = "track_processor_mono_toggle";
          mono->id_->flags_ |= PortIdentifier::Flags::Toggle;
          mono->id_->flags_ |= PortIdentifier::Flags::TpMono;
          mono_id_ = mono->get_uuid ();
          auto * input_gain =
            port_registry_.create_object<ControlPort> ("TP Input Gain");
          input_gain->set_owner (*this);
          input_gain->id_->sym_ = "track_processor_input_gain";
          input_gain->range_ = { 0.f, 4.f, 0.f };
          input_gain->deff_ = 1.f;
          input_gain->id_->flags_ |= PortIdentifier::Flags::TpInputGain;
          input_gain->set_control_value (1.f, false, false);
          input_gain_id_ = input_gain->get_uuid ();
        }
      break;
    default:
      break;
    }

  if (tr.type_ == Track::Type::Audio)
    {
      auto * output_gain =
        port_registry_.create_object<ControlPort> ("TP Output Gain");
      output_gain->set_owner (*this);
      output_gain->id_->sym_ = "track_processor_output_gain";
      output_gain->range_ = { 0.f, 4.f, 0.f };
      output_gain->deff_ = 1.f;
      output_gain->id_->flags2_ |= dsp::PortIdentifier::Flags2::TpOutputGain;
      output_gain->set_control_value (1.f, false, false);
      output_gain_id_ = output_gain->get_uuid ();

      auto * monitor_audio =
        port_registry_.create_object<ControlPort> ("Monitor audio");
      monitor_audio->set_owner (*this);
      monitor_audio->id_->sym_ = "track_processor_monitor_audio";
      monitor_audio->id_->flags_ |= dsp::PortIdentifier::Flags::Toggle;
      monitor_audio->id_->flags2_ |= dsp::PortIdentifier::Flags2::TpMonitorAudio;
      monitor_audio->set_control_value (0.f, false, false);
      monitor_audio_id_ = monitor_audio->get_uuid ();
    }

  init_common ();
}

void
TrackProcessor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("mono", mono_id_, true),
    make_field ("inputGain", input_gain_id_, true),
    make_field ("outputGain", output_gain_id_, true),
    make_field ("midiIn", midi_in_id_, true),
    make_field ("midiOut", midi_out_id_, true),
    make_field ("pianoRoll", piano_roll_id_, true),
    make_field ("monitorAudio", monitor_audio_id_, true),
    make_field ("stereoInL", stereo_in_left_id_, true),
    make_field ("stereoInR", stereo_in_right_id_, true),
    make_field ("stereoOutL", stereo_out_left_id_, true),
    make_field ("stereoOutR", stereo_out_right_id_, true),
    make_field ("midiCc", midi_cc_ids_, true),
    make_field ("pitchBend", pitch_bend_ids_, true),
    make_field ("polyKeyPressure", poly_key_pressure_ids_, true),
    make_field ("channelPressure", channel_pressure_ids_, true));
}

void
TrackProcessor::init_loaded (
  ProcessableTrack * track,
  PortRegistry      &port_registry)
{
  track_ = track;

  std::vector<Port *> ports;
  append_ports (ports);
  for (auto port : ports)
    {
      port->init_loaded (*this);
    }

  init_common ();
}

void
TrackProcessor::init_common ()
{
  if (midi_cc_ids_)
    {
      cc_mappings_ = std::make_unique<MidiMappings> ();

      for (const auto i : std::views::iota (16))
        {
          for (const auto j : std::views::iota (128))
            {
              auto cc_port_id = (*midi_cc_ids_)[i * 128 + j];
              auto cc_port = std::get<ControlPort *> (
                port_registry_.find_by_id_or_throw (cc_port_id));

              /* set caches */
              cc_port->midi_channel_ = i + 1;
              cc_port->midi_cc_no_ = j;

              /* set model bytes for CC:
               * [0] = ctrl change + channel
               * [1] = controller
               * [2] (unused) = control */
              std::array<midi_byte_t, 3> buf{};
              buf[0] = (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (midi_byte_t) i);
              buf[1] = (midi_byte_t) j;
              buf[2] = 0;

              /* bind */
              cc_mappings_->bind_track (buf, *cc_port, false);
            } /* endforeach 0..127 */

          auto pb_id = pitch_bend_ids_->at (i);
          auto kp_id = poly_key_pressure_ids_->at (i);
          auto cp_id = channel_pressure_ids_->at (i);
          auto pb = std::get<ControlPort *> (
            port_registry_.find_by_id_or_throw (pb_id));
          auto kp = std::get<ControlPort *> (
            port_registry_.find_by_id_or_throw (kp_id));
          auto cp = std::get<ControlPort *> (
            port_registry_.find_by_id_or_throw (cp_id));

          /* set caches */
          pb->midi_channel_ = i + 1;
          kp->midi_channel_ = i + 1;
          cp->midi_channel_ = i + 1;

        } /* endforeach 0..15 */

      updated_midi_automatable_ports_ =
        std::make_unique<MPMCQueue<ControlPort *>> (128 * 16);
    }
}

void
TrackProcessor::init_midi_port (bool in)
{
  if (in)
    {
      auto * midi_in = port_registry_.create_object<MidiPort> (
        "TP MIDI in", dsp::PortFlow::Input);
      midi_in->set_owner (*this);
      midi_in->id_->sym_ = ("track_processor_midi_in");
      midi_in->id_->flags_ |= dsp::PortIdentifier::Flags::SendReceivable;
      midi_in_id_ = midi_in->get_uuid ();
    }
  else
    {
      auto * midi_out = port_registry_.create_object<MidiPort> (
        "TP MIDI out", dsp::PortFlow::Output);
      midi_out->set_owner (*this);
      midi_out->id_->sym_ = ("track_processor_midi_out");
      midi_out_id_ = midi_out->get_uuid ();
    }
}

void
TrackProcessor::init_midi_cc_ports ()
{
  constexpr auto init_midi_port = [&] (const auto &port, const auto &index) {
    port->id_->flags_ |= dsp::PortIdentifier::Flags::MidiAutomatable;
    port->id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
    port->id_->midi_channel_ = index + 1;
    port->id_->port_index_ = index;
  };

  midi_cc_ids_ = std::make_unique<decltype (midi_cc_ids_)::element_type> ();
  pitch_bend_ids_ =
    std::make_unique<decltype (pitch_bend_ids_)::element_type> ();
  poly_key_pressure_ids_ =
    std::make_unique<decltype (poly_key_pressure_ids_)::element_type> ();
  channel_pressure_ids_ =
    std::make_unique<decltype (channel_pressure_ids_)::element_type> ();

  for (const auto i : std::views::iota (16))
    {
      /* starting from 1 */
      int channel = i + 1;

      for (const auto j : std::views::iota (128))
        {
          auto * cc = port_registry_.create_object<ControlPort> (
            fmt::format ("Ch{} {}", channel, midi_get_controller_name (j)));
          cc->set_owner (*this);
          init_midi_port (cc, (i * 128) + j);
          cc->id_->sym_ =
            fmt::format ("midi_controller_ch{}_{}", channel, j + 1);
          (*midi_cc_ids_)[(i * 128) + j] = cc->get_uuid ();
        }

      auto * pitch_bend = port_registry_.create_object<ControlPort> (
        fmt::format ("Ch{} Pitch bend", i + 1));
      pitch_bend->set_owner (*this);
      pitch_bend->id_->sym_ = fmt::format ("ch{}_pitch_bend", i + 1);
      init_midi_port (pitch_bend, i);
      pitch_bend->range_ = { -8192.f, 8191.f, 0.f };
      pitch_bend->deff_ = 0.f;
      pitch_bend->id_->flags2_ |= PortIdentifier::Flags2::MidiPitchBend;
      (*pitch_bend_ids_)[i] = pitch_bend->get_uuid ();

      auto * poly_key_pressure = port_registry_.create_object<ControlPort> (
        fmt::format ("Ch{} Poly key pressure", i + 1));
      poly_key_pressure->set_owner (*this);
      poly_key_pressure->id_->sym_ =
        fmt::format ("ch{}_poly_key_pressure", i + 1);
      init_midi_port (poly_key_pressure, i);
      poly_key_pressure->id_->flags2_ |=
        PortIdentifier::Flags2::MidiPolyKeyPressure;
      (*poly_key_pressure_ids_)[i] = poly_key_pressure->get_uuid ();

      auto * channel_pressure = port_registry_.create_object<ControlPort> (
        fmt::format ("Ch{} Channel pressure", i + 1));
      channel_pressure->set_owner (*this);
      channel_pressure->id_->sym_ = fmt::format ("ch{}_channel_pressure", i + 1);
      init_midi_port (channel_pressure, i);
      channel_pressure->id_->flags2_ |=
        PortIdentifier::Flags2::MidiChannelPressure;
      (*channel_pressure_ids_)[i] = channel_pressure->get_uuid ();
    }
}

void
TrackProcessor::init_stereo_out_ports (bool in)
{
  auto stereo_ports = StereoPorts::create_stereo_ports (
    port_registry_, in, in ? "TP Stereo in" : "TP Stereo out",
    std::string ("track_processor_stereo_") + (in ? "in" : "out"));
  iterate_tuple (
    [&] (const auto &port) {
      port->id_->flags_ |= PortIdentifier::Flags::SendReceivable;
      port->set_owner (*this);
    },
    stereo_ports);

  if (in)
    {
      stereo_in_left_id_ = stereo_ports.first->get_uuid ();
      stereo_in_right_id_ = stereo_ports.first->get_uuid ();
    }
  else
    {
      stereo_out_left_id_ = stereo_ports.first->get_uuid ();
      stereo_out_right_id_ = stereo_ports.first->get_uuid ();
    }
}

bool
TrackProcessor::is_audio () const
{
  return track_->in_signal_type_ == PortType::Audio;
}

bool
TrackProcessor::is_midi () const
{
  return track_->in_signal_type_ == PortType::Event;
}

bool
TrackProcessor::is_in_active_project () const
{
  return track_ && track_->is_in_active_project ();
}

void
TrackProcessor::set_port_metadata_from_owner (
  dsp::PortIdentifier &id,
  PortRange           &range) const
{
  auto * track = get_track ();
  z_return_if_fail (track);
  id.track_id_ = track->get_uuid ();
  id.owner_type_ = PortIdentifier::OwnerType::TrackProcessor;
}

std::string
TrackProcessor::get_full_designation_for_port (
  const dsp::PortIdentifier &id) const
{
  auto * tr = get_track ();
  z_return_val_if_fail (tr, {});
  return fmt::format ("{}/{}", tr->get_name (), id.label_);
}

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

void
TrackProcessor::on_midi_activity (const dsp::PortIdentifier &id)
{
  auto * track = get_track ();
  z_return_if_fail (track);
  track_->trigger_midi_activity_ = true;
}

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

bool
TrackProcessor::should_bounce_to_master (utils::audio::BounceStep step) const
{
  // only pre-insert bounces make sense for TrackProcessor outputs
  if (step != utils::audio::BounceStep::BeforeInserts)
    return false;

  auto * track = get_track ();
  return track->bounce_ && track->bounce_to_master_;
}

bool
TrackProcessor::are_events_on_midi_channel_approved (midi_byte_t channel) const
{
  auto * track = dynamic_cast<ChannelTrack *> (get_track ());
  if (track == nullptr)
    return false;

  if (
    (track->is_midi () || track->is_instrument ())
    && !track->channel_->all_midi_channels_
    && !track->channel_->midi_channels_[channel])
    {
      /* different channel */
      /*z_debug ("received event on different channel");*/
      return false;
    }
  return true;
}

void
TrackProcessor::init_after_cloning (
  const TrackProcessor &other,
  ObjectCloneType       clone_type)
{
  stereo_in_left_id_ = other.stereo_in_left_id_;
  stereo_in_right_id_ = other.stereo_in_right_id_;
  mono_id_ = other.mono_id_;
  input_gain_id_ = other.input_gain_id_;
  output_gain_id_ = other.output_gain_id_;
  stereo_out_left_id_ = other.stereo_out_left_id_;
  stereo_out_right_id_ = other.stereo_out_right_id_;
  midi_in_id_ = other.midi_in_id_;
  midi_out_id_ = other.midi_out_id_;
  piano_roll_id_ = other.piano_roll_id_;
  monitor_audio_id_ = other.monitor_audio_id_;
  if (other.cc_mappings_)
    cc_mappings_ = other.cc_mappings_->clone_unique ();

  midi_cc_ids_ = std::make_unique<decltype (midi_cc_ids_)::element_type> ();
  pitch_bend_ids_ =
    std::make_unique<decltype (pitch_bend_ids_)::element_type> ();
  poly_key_pressure_ids_ =
    std::make_unique<decltype (poly_key_pressure_ids_)::element_type> ();
  channel_pressure_ids_ =
    std::make_unique<decltype (channel_pressure_ids_)::element_type> ();

  *midi_cc_ids_ = *other.midi_cc_ids_;
  *pitch_bend_ids_ = *other.pitch_bend_ids_;
  *poly_key_pressure_ids_ = *other.poly_key_pressure_ids_;
  *channel_pressure_ids_ = *other.channel_pressure_ids_;

  init_common ();
}

void
TrackProcessor::append_ports (std::vector<Port *> &ports)
{
  if (stereo_in_left_id_ && stereo_in_right_id_)
    {
      iterate_tuple (
        [&] (auto &port) { ports.push_back (std::addressof (port)); },
        get_stereo_in_ports ());
    }
  if (mono_id_)
    {
      ports.push_back (std::addressof (get_mono_port ()));
    }
  if (input_gain_id_)
    {
      ports.push_back (std::addressof (get_input_gain_port ()));
    }
  if (output_gain_id_)
    {
      ports.push_back (std::addressof (get_output_gain_port ()));
    }
  if (monitor_audio_id_)
    {
      ports.push_back (std::addressof (get_monitor_audio_port ()));
    }
  if (stereo_out_left_id_ && stereo_out_right_id_)
    {
      iterate_tuple (
        [&] (auto &port) { ports.push_back (std::addressof (port)); },
        get_stereo_out_ports ());
    }
  if (midi_in_id_)
    {
      ports.push_back (std::addressof (get_midi_in_port ()));
    }
  if (midi_out_id_)
    {
      ports.push_back (std::addressof (get_midi_out_port ()));
    }
  if (piano_roll_id_)
    {
      ports.push_back (std::addressof (get_piano_roll_port ()));
    }
  if (midi_cc_ids_)
    {
      for (const auto ch_idx : std::views::iota (16))
        {
          for (const auto cc_idx : std::views::iota (128))
            {
              ports.push_back (
                std::addressof (get_midi_cc_port (ch_idx, cc_idx)));
            }
          ports.push_back (std::addressof (get_pitch_bend_port (ch_idx)));
          ports.push_back (std::addressof (get_poly_key_pressure_port (ch_idx)));
          ports.push_back (std::addressof (get_channel_pressure_port (ch_idx)));
        }
    }
}

/**
 * Clears all buffers.
 */
void
TrackProcessor::clear_buffers ()
{
  if (stereo_in_left_id_.has_value ())
    {
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (*AUDIO_ENGINE); },
        get_stereo_in_ports ());
    }
  if (stereo_out_left_id_.has_value ())
    {
      iterate_tuple (
        [&] (auto &port) { port.clear_buffer (*AUDIO_ENGINE); },
        get_stereo_out_ports ());
    }
  if (midi_in_id_)
    {
      get_midi_in_port ().clear_buffer (*AUDIO_ENGINE);
    }
  if (midi_out_id_)
    {
      get_midi_out_port ().clear_buffer (*AUDIO_ENGINE);
    }
  if (piano_roll_id_)
    {
      get_piano_roll_port ().clear_buffer (*AUDIO_ENGINE);
    }
}

/**
 * Disconnects all ports connected to the TrackProcessor.
 */
void
TrackProcessor::disconnect_all ()
{
  auto track = get_track ();
  z_return_if_fail (track);

  switch (track->in_signal_type_)
    {
    case PortType::Audio:
      get_mono_port ().disconnect_all ();
      get_input_gain_port ().disconnect_all ();
      get_output_gain_port ().disconnect_all ();
      get_monitor_audio_port ().disconnect_all ();
      iterate_tuple (
        [&] (auto &port) { port.disconnect_all (); }, get_stereo_in_ports ());
      iterate_tuple (
        [&] (auto &port) { port.disconnect_all (); }, get_stereo_out_ports ());
      break;
    case PortType::Event:
      get_midi_in_port ().disconnect_all ();
      get_midi_out_port ().disconnect_all ();
      if (track->has_piano_roll ())
        get_piano_roll_port ().disconnect_all ();
      break;
    default:
      break;
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
  ControlPort * cc;
  auto         &midi_out = get_midi_out_port ();
  while (updated_midi_automatable_ports_->pop_front (cc))
    {
      /*port_identifier_print (&cc->id_);*/
      if (ENUM_BITSET_TEST (
            cc->id_->flags2_, PortIdentifier::Flags2::MidiPitchBend))
        {
          midi_out.midi_events_.queued_events_.add_pitchbend (
            cc->midi_channel_,
            utils::math::round_to_signed_32 (cc->control_) + 0x2000,
            local_offset);
        }
      else if (
        ENUM_BITSET_TEST (
          cc->id_->flags2_, PortIdentifier::Flags2::MidiPolyKeyPressure))
        {
#if ZRYTHM_TARGET_VER_MAJ > 1
          /* TODO - unsupported in v1 */
#endif
        }
      else if (
        ENUM_BITSET_TEST (
          cc->id_->flags2_, PortIdentifier::Flags2::MidiChannelPressure))
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
}

void
TrackProcessor::process (const EngineProcessTimeInfo &time_nfo)
{
  z_return_if_fail (track_);

  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;

      /* if frozen or disabled, skip */
      if (tr->frozen_ || !tr->is_enabled ())
        {
          return;
        }

      /* set the audio clip contents to stereo out */
      if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          tr->fill_events (time_nfo, get_stereo_out_ports ());
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
        tr->in_signal_type_ == dsp::PortType::Event && CLIP_EDITOR->has_region_)
        {
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              auto clip_editor_track_var = CLIP_EDITOR->get_track ();
              if (
                clip_editor_track_var.has_value ()
                && std::get_if<TrackT *> (&clip_editor_track_var.value ())
                && std::get<TrackT *> (clip_editor_track_var.value ()) == tr)
                {
                  /* if not set to "all channels", filter-append */
                  if (!tr->channel_->all_midi_channels_)
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
            }
        }

      /* add inputs to outputs */
      if (tr->in_signal_type_ == PortType::Audio)
        {
          const auto &stereo_in = get_stereo_in_ports ();
          const auto &stereo_out = get_stereo_out_ports ();
          if (
            tr->type_ != Track::Type::Audio
            || (tr->type_ == Track::Type::Audio && get_monitor_audio_port ().is_toggled ()))
            {
              utils::float_ranges::mix_product (
                &stereo_out.first.buf_[time_nfo.local_offset_],
                &stereo_in.first.buf_[time_nfo.local_offset_],
                input_gain_id_ ? get_input_gain_port ().control_ : 1.f,
                time_nfo.nframes_);

              if (mono_id_ && get_mono_port ().is_toggled ())
                {
                  utils::float_ranges::mix_product (
                    &stereo_out.second.buf_[time_nfo.local_offset_],
                    &stereo_in.first.buf_[time_nfo.local_offset_],
                    input_gain_id_ ? get_input_gain_port ().control_ : 1.f,
                    time_nfo.nframes_);
                }
              else
                {
                  utils::float_ranges::mix_product (
                    &stereo_out.second.buf_[time_nfo.local_offset_],
                    &stereo_in.second.buf_[time_nfo.local_offset_],
                    input_gain_id_ ? get_input_gain_port ().control_ : 1.f,
                    time_nfo.nframes_);
                }
            }
        }
      else if (tr->in_signal_type_ == PortType::Event)
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
        && (tr->can_record () || !tr->automation_tracklist_->ats_.empty ()))
        {
          /* handle recording. this will only create events in regions. it will
           * not copy the input content to the output ports. this will also
           * create automation for MIDI CC, if any (see
           * midi_mappings_apply_cc_events above) */
          handle_recording (time_nfo);
        }

      /* apply output gain */
      if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          const auto stereo_out = get_stereo_out_ports ();
          utils::float_ranges::mul_k2 (
            &stereo_out.first.buf_[time_nfo.local_offset_],
            get_output_gain_port ().control_, time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_out.second.buf_[time_nfo.local_offset_],
            get_output_gain_port ().control_, time_nfo.nframes_);
        }
    },
    convert_to_variant<ProcessableTrackPtrVariant> (track_));
}

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
TrackProcessor::disconnect_from_prefader ()
{
  auto * tr = dynamic_cast<ChannelTrack *> (get_track ());
  z_return_if_fail (tr);

  auto &prefader = tr->channel_->prefader_;
  switch (tr->in_signal_type_)
    {
    case PortType::Audio:
      if (tr->out_signal_type_ == PortType::Audio)
        {
          const auto stereo_out = get_stereo_out_ports ();
          const auto prefader_stereo_in = prefader->get_stereo_in_ports ();
          disconnect_ports (
            stereo_out.first.get_uuid (), prefader_stereo_in.first.get_uuid ());
          disconnect_ports (
            stereo_out.second.get_uuid (),
            prefader_stereo_in.second.get_uuid ());
        }
      break;
    case PortType::Event:
      if (tr->out_signal_type_ == PortType::Event)
        {
          disconnect_ports (*midi_out_id_, prefader->get_midi_in_id ());
        }
      break;
    default:
      break;
    }
}

void
TrackProcessor::connect_to_prefader ()
{
  auto * tr = dynamic_cast<ChannelTrack *> (get_track ());
  z_return_if_fail (tr);
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  auto &prefader = tr->channel_->prefader_;
  switch (tr->in_signal_type_)
    {
    case PortType::Audio:
      if (tr->out_signal_type_ == PortType::Audio)
        {
          const auto stereo_out = get_stereo_out_ports ();
          const auto prefader_stereo_in = prefader->get_stereo_in_ports ();
          connect_ports (
            stereo_out.first.get_uuid (), prefader_stereo_in.first.get_uuid ());
          connect_ports (
            stereo_out.second.get_uuid (),
            prefader_stereo_in.second.get_uuid ());
        }
      break;
    case PortType::Event:
      if (tr->out_signal_type_ == PortType::Event)
        {
          connect_ports (*midi_out_id_, prefader->get_midi_in_id ());
        }
      break;
    default:
      break;
    }
}

void
TrackProcessor::connect_ports (const PortUuid &src, const PortUuid &dest)
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);
  mgr->ensure_connect_default (src, dest, true);
}

void
TrackProcessor::disconnect_ports (const PortUuid &src, const PortUuid &dest)
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);
  if (mgr->are_ports_connected (src, dest))
    {
      mgr->ensure_disconnect (src, dest);
    }
}

void
TrackProcessor::disconnect_from_plugin (
  zrythm::gui::old_dsp::plugins::Plugin &pl)
{
  auto * tr = get_track ();
  z_return_if_fail (tr);
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  for (auto &in_port_var : pl.get_input_port_span ())
    {
      std::visit (
        [&] (auto &&in_port) {
          using PortT = base_type<decltype (in_port)>;
          if constexpr (std::is_same_v<PortT, AudioPort>)
            {
              if (tr->in_signal_type_ == PortType::Audio)
                {
                  disconnect_ports (*stereo_out_left_id_, in_port->get_uuid ());
                  disconnect_ports (*stereo_out_right_id_, in_port->get_uuid ());
                }
            }
          if constexpr (std::is_same_v<PortT, MidiPort>)
            {
              if (tr->in_signal_type_ == PortType::Event)
                {
                  disconnect_ports (*midi_out_id_, in_port->get_uuid ());
                }
            }
        },
        in_port_var);
    }
}

void
TrackProcessor::connect_to_plugin (zrythm::gui::old_dsp::plugins::Plugin &pl)
{
  auto * tr = get_track ();
  z_return_if_fail (tr);
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  size_t last_index, num_ports_to_connect, i;
  auto   pl_in_ports = pl.get_input_port_span ();
  if (tr->in_signal_type_ == PortType::Event)
    {
      for (const auto * in_port : pl_in_ports.get_elements_by_type<MidiPort> ())
        {
          if (in_port->id_->flow_ == PortFlow::Input)
            {
              connect_ports (*midi_out_id_, in_port->get_uuid ());
            }
        }
    }
  else if (tr->in_signal_type_ == PortType::Audio)
    {
      auto num_pl_audio_ins =
        std::ranges::distance (pl_in_ports.get_elements_by_type<AudioPort> ());

      num_ports_to_connect = 0;
      if (num_pl_audio_ins == 1)
        {
          num_ports_to_connect = 1;
        }
      else if (num_pl_audio_ins > 1)
        {
          num_ports_to_connect = 2;
        }

      last_index = 0;
      for (i = 0; i < num_ports_to_connect; i++)
        {
          for (; last_index < pl.in_ports_.size (); last_index++)
            {
              const auto in_port_var = pl_in_ports[last_index];
              if (std::holds_alternative<AudioPort *> (in_port_var))
                {
                  auto * in_port = std::get<AudioPort *> (in_port_var);
                  if (i == 0)
                    {
                      connect_ports (*stereo_out_left_id_, in_port->get_uuid ());
                      last_index++;
                      break;
                    }
                  if (i == 1)
                    {
                      connect_ports (
                        *stereo_out_right_id_, in_port->get_uuid ());
                      last_index++;
                      break;
                    }
                }
            }
        }
    }
}

PortConnectionsManager *
TrackProcessor::get_port_connections_manager () const
{
  auto * track = get_track ();
  z_return_val_if_fail (track, nullptr);
  return track->get_port_connections_manager ();
}
