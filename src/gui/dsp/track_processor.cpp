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

void
TrackProcessor::init_loaded (ProcessableTrack * track)
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
  if (!midi_cc_.empty ())
    {
      cc_mappings_ = std::make_unique<MidiMappings> ();

      for (size_t i = 0; i < 16; i++)
        {
          for (size_t j = 0; j < 128; j++)
            {
              auto &cc_port = midi_cc_[i * 128 + j];
              z_return_if_fail (cc_port);

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

          auto &pb = pitch_bend_[i];
          z_return_if_fail (pb);
          auto &kp = poly_key_pressure_[i];
          z_return_if_fail (kp);
          auto &cp = channel_pressure_[i];
          z_return_if_fail (cp);

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
      midi_in_ = std::make_unique<MidiPort> ("TP MIDI in", dsp::PortFlow::Input);
      midi_in_->set_owner (*this);
      midi_in_->id_->sym_ = ("track_processor_midi_in");
      midi_in_->id_->flags_ |= dsp::PortIdentifier::Flags::SendReceivable;
    }
  else
    {
      midi_out_ =
        std::make_unique<MidiPort> ("TP MIDI out", dsp::PortFlow::Output);
      midi_out_->set_owner (*this);
      midi_out_->id_->sym_ = ("track_processor_midi_out");
    }
}

void
TrackProcessor::init_midi_cc_ports (bool loading)
{
#define INIT_MIDI_PORT(x, idx) \
  x->id_->flags_ |= dsp::PortIdentifier::Flags::MidiAutomatable; \
  x->id_->flags_ |= dsp::PortIdentifier::Flags::Automatable; \
  x->id_->port_index_ = idx;

  midi_cc_.resize (16 * 128);
  pitch_bend_.resize (16);
  poly_key_pressure_.resize (16);
  channel_pressure_.resize (16);

  for (int i = 0; i < 16; i++)
    {
      /* starting from 1 */
      int channel = i + 1;

      for (int j = 0; j < 128; j++)
        {
          auto cc = std::make_unique<ControlPort> (
            fmt::format ("Ch{} {}", channel, midi_get_controller_name (j)));
          cc->set_owner (*this);
          INIT_MIDI_PORT (cc, i * 128 + j);
          cc->id_->sym_ =
            fmt::format ("midi_controller_ch{}_{}", channel, j + 1);
          midi_cc_[i * 128 + j] = std::move (cc);
        }

      pitch_bend_[i] =
        std::make_unique<ControlPort> (fmt::format ("Ch{} Pitch bend", i + 1));
      pitch_bend_[i]->set_owner (*this);
      pitch_bend_[i]->id_->sym_ = fmt::format ("ch{}_pitch_bend", i + 1);
      INIT_MIDI_PORT (pitch_bend_[i], i);
      pitch_bend_[i]->range_ = { -8192.f, 8191.f, 0.f };
      pitch_bend_[i]->deff_ = 0.f;
      pitch_bend_[i]->id_->flags2_ |= PortIdentifier::Flags2::MidiPitchBend;

      poly_key_pressure_[i] = std::make_unique<ControlPort> (
        fmt::format ("Ch{} Poly key pressure", i + 1));
      poly_key_pressure_[i]->set_owner (*this);
      poly_key_pressure_[i]->id_->sym_ =
        fmt::format ("ch{}_poly_key_pressure", i + 1);
      INIT_MIDI_PORT (poly_key_pressure_.at (i), i);
      poly_key_pressure_[i]->id_->flags2_ |=
        PortIdentifier::Flags2::MidiPolyKeyPressure;

      channel_pressure_[i] = std::make_unique<ControlPort> (
        fmt::format ("Ch{} Channel pressure", i + 1));
      channel_pressure_[i]->set_owner (*this);
      channel_pressure_[i]->id_->sym_ =
        fmt::format ("ch{}_channel_pressure", i + 1);
      INIT_MIDI_PORT (channel_pressure_.at (i), i);
      channel_pressure_[i]->id_->flags2_ |=
        PortIdentifier::Flags2::MidiChannelPressure;
    }

#undef INIT_MIDI_PORT
}

void
TrackProcessor::init_stereo_out_ports (bool in)
{
  PortFlow  flow = in ? PortFlow::Input : PortFlow::Output;
  AudioPort l (in ? "TP Stereo in L" : "TP Stereo out L", flow);
  l.id_->sym_ =
    std::string ("track_processor_stereo_") + (in ? "in" : "out") + "_l";
  AudioPort r (in ? "TP Stereo in R" : "TP Stereo out R", flow);
  r.id_->sym_ =
    std::string ("track_processor_stereo_") + (in ? "in" : "out") + "_r";
  if (in)
    {
      l.id_->flags_ |= PortIdentifier::Flags::SendReceivable;
      r.id_->flags_ |= PortIdentifier::Flags::SendReceivable;
    }

  auto &sp = in ? stereo_in_ : stereo_out_;
  sp = std::make_unique<StereoPorts> (std::move (l), std::move (r));
  sp->set_owner (*this);
}

TrackProcessor::TrackProcessor (ProcessableTrack * tr) : track_ (tr)
{
  switch (tr->in_signal_type_)
    {
    case PortType::Event:
      init_midi_port (false);
      init_midi_port (true);

      /* set up piano roll port */
      if (tr->has_piano_roll () || tr->is_chord ())
        {
          piano_roll_ =
            std::make_unique<MidiPort> ("TP Piano Roll", PortFlow::Input);
          piano_roll_->set_owner (*this);
          piano_roll_->id_->sym_ = "track_processor_piano_roll";
          piano_roll_->id_->flags_ = PortIdentifier::Flags::PianoRoll;
          if (!tr->is_chord ())
            {
              init_midi_cc_ports (false);
            }
        }
      break;
    case PortType::Audio:
      init_stereo_out_ports (false);
      init_stereo_out_ports (true);
      if (tr->type_ == Track::Type::Audio)
        {
          mono_ = std::make_unique<ControlPort> ("TP Mono Toggle");
          mono_->set_owner (*this);
          mono_->id_->sym_ = "track_processor_mono_toggle";
          mono_->id_->flags_ |= PortIdentifier::Flags::Toggle;
          mono_->id_->flags_ |= PortIdentifier::Flags::TpMono;
          input_gain_ = std::make_unique<ControlPort> ("TP Input Gain");
          input_gain_->set_owner (*this);
          input_gain_->id_->sym_ = "track_processor_input_gain";
          input_gain_->range_ = { 0.f, 4.f, 0.f };
          input_gain_->deff_ = 1.f;
          input_gain_->id_->flags_ |= PortIdentifier::Flags::TpInputGain;
          input_gain_->set_control_value (1.f, false, false);
        }
      break;
    default:
      break;
    }

  if (tr->type_ == Track::Type::Audio)
    {
      output_gain_ = std::make_unique<ControlPort> ("TP Output Gain");
      output_gain_->set_owner (*this);
      output_gain_->id_->sym_ = "track_processor_output_gain";
      output_gain_->range_ = { 0.f, 4.f, 0.f };
      output_gain_->deff_ = 1.f;
      output_gain_->id_->flags2_ |= dsp::PortIdentifier::Flags2::TpOutputGain;
      output_gain_->set_control_value (1.f, false, false);

      monitor_audio_ = std::make_unique<ControlPort> ("Monitor audio");
      monitor_audio_->set_owner (*this);
      monitor_audio_->id_->sym_ = "track_processor_monitor_audio";
      monitor_audio_->id_->flags_ |= dsp::PortIdentifier::Flags::Toggle;
      monitor_audio_->id_->flags2_ |=
        dsp::PortIdentifier::Flags2::TpMonitorAudio;
      monitor_audio_->set_control_value (0.f, false, false);
    }

  init_common ();
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
  auto get_track_name_hash = [] (const auto &track) -> Track::NameHashT {
    return track->name_.empty () ? 0 : track->get_name_hash ();
  };

  auto * track = get_track ();
  z_return_if_fail (track);
  id.track_name_hash_ = get_track_name_hash (track);
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
  const dsp::PortIdentifier &id,
  float                      value)
{
  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, id.flags_, PortIdentifier::Flags::MidiAutomatable))
    {
      auto port = PROJECT->find_port_by_id (id);
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
TrackProcessor::init_after_cloning (const TrackProcessor &other)
{
  if (other.stereo_in_)
    stereo_in_ = other.stereo_in_->clone_unique ();
  if (other.mono_)
    mono_ = other.mono_->clone_unique ();
  if (other.input_gain_)
    input_gain_ = other.input_gain_->clone_unique ();
  if (other.output_gain_)
    output_gain_ = other.output_gain_->clone_unique ();
  if (other.stereo_out_)
    stereo_out_ = other.stereo_out_->clone_unique ();
  if (other.midi_in_)
    midi_in_ = other.midi_in_->clone_unique ();
  if (other.midi_out_)
    midi_out_ = other.midi_out_->clone_unique ();
  if (other.piano_roll_)
    piano_roll_ = other.piano_roll_->clone_unique ();
  if (other.monitor_audio_)
    monitor_audio_ = other.monitor_audio_->clone_unique ();
  if (other.cc_mappings_)
    cc_mappings_ = other.cc_mappings_->clone_unique ();
  for (size_t i = 0; i < other.midi_cc_.size (); i++)
    {
      if (!other.midi_cc_[i])
        break;

      z_return_if_fail (midi_cc_.size () == i);
      midi_cc_.emplace_back (other.midi_cc_[i]->clone_unique ());
    }
  for (size_t i = 0; i < other.pitch_bend_.size (); i++)
    {
      if (!other.pitch_bend_[i])
        break;
      z_return_if_fail (pitch_bend_.size () == i);
      pitch_bend_.emplace_back (other.pitch_bend_[i]->clone_unique ());
    }
  for (size_t i = 0; i < other.poly_key_pressure_.size (); i++)
    {
      if (!other.poly_key_pressure_[i])
        break;
      z_return_if_fail (poly_key_pressure_.size () == i);
      poly_key_pressure_.emplace_back (
        other.poly_key_pressure_[i]->clone_unique ());
    }
  for (size_t i = 0; i < other.channel_pressure_.size (); i++)
    {
      if (!other.channel_pressure_[i])
        break;
      z_return_if_fail (channel_pressure_.size () == i);
      channel_pressure_.emplace_back (
        other.channel_pressure_[i]->clone_unique ());
    }
  init_common ();
}

void
TrackProcessor::append_ports (std::vector<Port *> &ports)
{
  if (stereo_in_)
    {
      ports.push_back (&stereo_in_->get_l ());
      ports.push_back (&stereo_in_->get_r ());
    }
  if (mono_)
    {
      ports.push_back (mono_.get ());
    }
  if (input_gain_)
    {
      ports.push_back (input_gain_.get ());
    }
  if (output_gain_)
    {
      ports.push_back (output_gain_.get ());
    }
  if (monitor_audio_)
    {
      ports.push_back (monitor_audio_.get ());
    }
  if (stereo_out_)
    {
      ports.push_back (&stereo_out_->get_l ());
      ports.push_back (&stereo_out_->get_r ());
    }
  if (midi_in_)
    {
      ports.push_back (midi_in_.get ());
    }
  if (midi_out_)
    {
      ports.push_back (midi_out_.get ());
    }
  if (piano_roll_)
    {
      ports.push_back (piano_roll_.get ());
    }
  if (!midi_cc_.empty ())
    {
      for (int i = 0; i < 16; i++)
        {
          for (int j = 0; j < 128; j++)
            {
              if (midi_cc_[i * 128 + j])
                {
                  ports.push_back (midi_cc_[i * 128 + j].get ());
                }
            }
          if (pitch_bend_[i])
            {
              ports.push_back (pitch_bend_[i].get ());
            }
          if (poly_key_pressure_[i])
            {
              ports.push_back (poly_key_pressure_[i].get ());
            }
          if (channel_pressure_[i])
            {
              ports.push_back (channel_pressure_[i].get ());
            }
        }
    }
}

/**
 * Clears all buffers.
 */
void
TrackProcessor::clear_buffers ()
{
  if (stereo_in_)
    {
      stereo_in_->clear_buffer (*AUDIO_ENGINE);
    }
  if (stereo_out_)
    {
      stereo_out_->clear_buffer (*AUDIO_ENGINE);
    }
  if (midi_in_)
    {
      midi_in_->clear_buffer (*AUDIO_ENGINE);
    }
  if (midi_out_)
    {
      midi_out_->clear_buffer (*AUDIO_ENGINE);
    }
  if (piano_roll_)
    {
      piano_roll_->clear_buffer (*AUDIO_ENGINE);
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
      mono_->disconnect_all ();
      input_gain_->disconnect_all ();
      output_gain_->disconnect_all ();
      monitor_audio_->disconnect_all ();
      stereo_in_->get_l ().disconnect_all ();
      stereo_in_->get_r ().disconnect_all ();
      stereo_out_->get_l ().disconnect_all ();
      stereo_out_->get_r ().disconnect_all ();
      break;
    case PortType::Event:
      midi_in_->disconnect_all ();
      midi_out_->disconnect_all ();
      if (track->has_piano_roll ())
        piano_roll_->disconnect_all ();
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
  while (updated_midi_automatable_ports_->pop_front (cc))
    {
      /*port_identifier_print (&cc->id_);*/
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, cc->id_->flags2_,
          PortIdentifier::Flags2::MidiPitchBend))
        {
          midi_out_->midi_events_.queued_events_.add_pitchbend (
            cc->midi_channel_,
            utils::math::round_to_signed_32 (cc->control_) + 0x2000,
            local_offset);
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, cc->id_->flags2_,
          PortIdentifier::Flags2::MidiPolyKeyPressure))
        {
#if ZRYTHM_TARGET_VER_MAJ > 1
          /* TODO - unsupported in v1 */
#endif
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, cc->id_->flags2_,
          PortIdentifier::Flags2::MidiChannelPressure))
        {
          midi_out_->midi_events_.queued_events_.add_channel_pressure (
            cc->midi_channel_,
            (midi_byte_t) utils::math::round_to_signed_32 (cc->control_ * 127.f),
            local_offset);
        }
      else
        {
          midi_out_->midi_events_.queued_events_.add_control_change (
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
          tr->fill_events (time_nfo, *stereo_out_);
        }

      /* set the piano roll contents to midi out */
      if constexpr (
        std::derived_from<TrackT, PianoRollTrack>
        || std::is_same_v<TrackT, ChordTrack>)
        {
          auto &pr = piano_roll_;

          /* panic MIDI if necessary */
          if (AUDIO_ENGINE->panic_.load ())
            {
              pr->midi_events_.queued_events_.panic ();
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
                time_nfo, pr->midi_events_.queued_events_);
            }
          pr->midi_events_.dequeue (time_nfo.local_offset_, time_nfo.nframes_);
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
          if (midi_out_->midi_events_.active_events_.has_any ())
            {
#if 0
          z_debug (
            "%s midi processor out has %d events",
            tr->name, self->midi_out->midi_events_->num_events);
#endif
            }

          /* append the midi events from piano roll to MIDI out */
          midi_out_->midi_events_.active_events_.append (
            pr->midi_events_.active_events_, time_nfo.local_offset_,
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
                      midi_in_->midi_events_.active_events_.append_w_filter (
                        AUDIO_ENGINE->midi_editor_manual_press_->midi_events_
                          .active_events_,
                        tr->channel_->midi_channels_, time_nfo.local_offset_,
                        time_nfo.nframes_);
                    }
                  /* otherwise append normally */
                  else
                    {
                      midi_in_->midi_events_.active_events_.append (
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
          if (
            tr->type_ != Track::Type::Audio
            || (tr->type_ == Track::Type::Audio && monitor_audio_->is_toggled ()))
            {
              utils::float_ranges::mix_product (
                &stereo_out_->get_l ().buf_[time_nfo.local_offset_],
                &stereo_in_->get_l ().buf_[time_nfo.local_offset_],
                input_gain_ ? input_gain_->control_ : 1.f, time_nfo.nframes_);

              if (mono_ && mono_->is_toggled ())
                {
                  utils::float_ranges::mix_product (
                    &stereo_out_->get_r ().buf_[time_nfo.local_offset_],
                    &stereo_in_->get_l ().buf_[time_nfo.local_offset_],
                    input_gain_ ? input_gain_->control_ : 1.f,
                    time_nfo.nframes_);
                }
              else
                {
                  utils::float_ranges::mix_product (
                    &stereo_out_->get_r ().buf_[time_nfo.local_offset_],
                    &stereo_in_->get_r ().buf_[time_nfo.local_offset_],
                    input_gain_ ? input_gain_->control_ : 1.f,
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
                  midi_in_->midi_events_.active_events_.set_channel (
                    tr->midi_ch_);
                }
            }

          /* process midi bindings */
          if (cc_mappings_ && TRANSPORT->recording_)
            {
              cc_mappings_->apply_from_cc_events (
                midi_in_->midi_events_.active_events_);
            }

          /* if chord track, transform MIDI input to appropriate MIDI notes */
          if (tr->is_chord ())
            {
              midi_out_->midi_events_.active_events_.transform_chord_and_append (
                midi_in_->midi_events_.active_events_, time_nfo.local_offset_,
                time_nfo.nframes_);
            }
          /* else if not chord track, simply pass the input MIDI data to the
           * output port */
          else
            {
              midi_out_->midi_events_.active_events_.append (
                midi_in_->midi_events_.active_events_, time_nfo.local_offset_,
                time_nfo.nframes_);
            }

          /* if pending a panic message, append it */
          if (pending_midi_panic_)
            {
              midi_out_->midi_events_.active_events_.panic_without_lock ();
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
          utils::float_ranges::mul_k2 (
            &stereo_out_->get_l ().buf_[time_nfo.local_offset_],
            output_gain_->control_, time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_out_->get_r ().buf_[time_nfo.local_offset_],
            output_gain_->control_, time_nfo.nframes_);
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
          disconnect_ports (*stereo_out_, *prefader->stereo_in_);
        }
      break;
    case PortType::Event:
      if (tr->out_signal_type_ == PortType::Event)
        {
          disconnect_ports (*midi_out_, *prefader->midi_in_);
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
          connect_ports (stereo_out_->get_l (), prefader->stereo_in_->get_l ());
          connect_ports (stereo_out_->get_r (), prefader->stereo_in_->get_r ());
        }
      break;
    case PortType::Event:
      if (tr->out_signal_type_ == PortType::Event)
        {
          connect_ports (*midi_out_, *prefader->midi_in_);
        }
      break;
    default:
      break;
    }
}

void
TrackProcessor::connect_ports (const Port &src, const Port &dest)
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);
  mgr->ensure_connect_default (*src.id_, *dest.id_, true);
}

void
TrackProcessor::disconnect_ports (const Port &src, const Port &dest)
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);
  if (mgr->are_ports_connected (*src.id_, *dest.id_))
    {
      mgr->ensure_disconnect (*src.id_, *dest.id_);
    }
}

void
TrackProcessor::connect_ports (const StereoPorts &src, const StereoPorts &dest)
{
  connect_ports (src.get_l (), dest.get_l ());
  connect_ports (src.get_r (), dest.get_r ());
}

void
TrackProcessor::disconnect_ports (const StereoPorts &src, const StereoPorts &dest)
{
  disconnect_ports (src.get_l (), dest.get_l ());
  disconnect_ports (src.get_r (), dest.get_r ());
}

void
TrackProcessor::disconnect_from_plugin (
  zrythm::gui::old_dsp::plugins::Plugin &pl)
{
  auto * tr = get_track ();
  z_return_if_fail (tr);
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  for (auto &in_port : pl.in_ports_)
    {
      if (tr->in_signal_type_ == PortType::Audio)
        {
          if (!in_port->is_audio ())
            continue;

          disconnect_ports (stereo_out_->get_l (), *in_port);
          disconnect_ports (stereo_out_->get_r (), *in_port);
        }
      else if (tr->in_signal_type_ == PortType::Event)
        {
          if (!in_port->is_midi ())
            continue;

          disconnect_ports (*midi_out_, *in_port);
        }
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

  if (tr->in_signal_type_ == PortType::Event)
    {
      for (i = 0; i < pl.in_ports_.size (); i++)
        {
          auto &in_port = pl.in_ports_[i];
          if (
            in_port->id_->type_ == PortType::Event
            && in_port->id_->flow_ == PortFlow::Input)
            {
              connect_ports (*midi_out_, *in_port);
            }
        }
    }
  else if (tr->in_signal_type_ == PortType::Audio)
    {
      int num_pl_audio_ins = 0;
      for (i = 0; i < pl.in_ports_.size (); i++)
        {
          auto &port = pl.in_ports_[i];
          if (port->id_->type_ == PortType::Audio)
            num_pl_audio_ins++;
        }

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
              auto &in_port = pl.in_ports_[last_index];
              if (in_port->id_->type_ == PortType::Audio)
                {
                  if (i == 0)
                    {
                      connect_ports (stereo_out_->get_l (), *in_port);
                      last_index++;
                      break;
                    }
                  else if (i == 1)
                    {
                      connect_ports (stereo_out_->get_r (), *in_port);
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
