// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_port.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/engine.h"
#include "common/dsp/engine_jack.h"
#include "common/dsp/master_track.h"
#include "common/dsp/recordable_track.h"
#include "common/dsp/rtaudio_device.h"
#include "common/dsp/tracklist.h"
#include "common/utils/dsp.h"
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/project.h"

#include <fmt/format.h>

AudioPort::AudioPort ()
{
  minf_ = -1.f;
  maxf_ = 1.f;
  zerof_ = 0.f;
}

AudioPort::AudioPort (std::string label, PortFlow flow)
    : Port (label, PortType::Audio, flow, -1.f, 1.f, 0.f)
{
}

void
AudioPort::init_after_cloning (const AudioPort &other)
{
  Port::copy_members_from (other);
}

void
AudioPort::allocate_bufs ()
{
  audio_ring_ = std::make_unique<RingBuffer<float>> (AUDIO_RING_SIZE);

  size_t max = std::max (AUDIO_ENGINE->block_length_, 1u);
  buf_.resize (max);
  last_buf_sz_ = max;
}

void
AudioPort::clear_buffer (AudioEngine &engine)
{
  dsp_fill (
    buf_.data (), DENORMAL_PREVENTION_VAL (&engine), engine.block_length_);
}

void
AudioPort::sum_data_from_dummy (
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (
    id_.owner_type_ == PortIdentifier::OwnerType::AudioEngine
    || id_.flow_ != PortFlow::Input || id_.type_ != PortType::Audio
    || AUDIO_ENGINE->audio_backend_ != AudioBackend::AUDIO_BACKEND_DUMMY
    || AUDIO_ENGINE->midi_backend_ != MidiBackend::MIDI_BACKEND_DUMMY)
    return;

  if (AUDIO_ENGINE->dummy_input_)
    {
      Port * port = NULL;
      if (ENUM_BITSET_TEST (
            PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoL))
        {
          port = &AUDIO_ENGINE->dummy_input_->get_l ();
        }
      else if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoR))
        {
          port = &AUDIO_ENGINE->dummy_input_->get_r ();
        }

      if (port)
        {
          dsp_add2 (&buf_[start_frame], &port->buf_[start_frame], nframes);
        }
    }
}

#if HAVE_JACK
void
AudioPort::receive_audio_data_from_jack (
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (
    this->internal_type_ != Port::InternalType::JackPort
    || this->id_.type_ != PortType::Audio)
    return;

  float * in;
  in = (float *) jack_port_get_buffer (
    JACK_PORT_T (this->data_), AUDIO_ENGINE->nframes_);

  dsp_add2 (&this->buf_[start_frames], &in[start_frames], nframes);
}

void
AudioPort::send_audio_data_to_jack (
  const nframes_t start_frames,
  const nframes_t nframes)
{
  if (internal_type_ != Port::InternalType::JackPort)
    return;

  jack_port_t * jport = JACK_PORT_T (data_);

  if (jack_port_connected (jport) <= 0)
    {
      return;
    }

  auto * out = (float *) jack_port_get_buffer (jport, AUDIO_ENGINE->nframes_);

  dsp_copy (&out[start_frames], &buf_.data ()[start_frames], nframes);
}
#endif // HAVE_JACK

#if HAVE_RTAUDIO
void
AudioPort::expose_to_rtaudio (bool expose)
{
  auto track = dynamic_cast<ChannelTrack *> (get_track (false));
  if (!track)
    return;

  auto &ch = track->channel_;

  if (expose)
    {
      if (is_input ())
        {
          auto create_and_append_rtaudio_devices = [this] (const auto &extPorts) {
            for (const auto &ext_port : extPorts)
              {
                auto dev = std::make_shared<RtAudioDevice> (
                  ext_port->rtaudio_is_input_, ext_port->rtaudio_id_,
                  ext_port->rtaudio_channel_idx_, this);
                dev->open (true);
                rtaudio_ins_.emplace_back (std::move (dev));
              }
          };

          if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoL))
            {
              if (!ch->all_stereo_l_ins_)
                {
                  create_and_append_rtaudio_devices (ch->ext_stereo_l_ins_);
                }
            }
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoR))
            {
              if (!ch->all_stereo_r_ins_)
                {
                  create_and_append_rtaudio_devices (ch->ext_stereo_r_ins_);
                }
            }
        }
      z_debug ("exposing {}", get_full_designation ());
    }
  else
    {
      if (is_input ())
        {
          rtaudio_ins_.clear ();
        }
      z_debug ("unexposing {}", get_full_designation ());
    }
  exposed_to_backend_ = expose;
}

void
AudioPort::prepare_rtaudio_data ()
{
  z_return_if_fail (
    // is_input() &&
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend_));

  for (auto &dev : rtaudio_ins_)
    {
      SemaphoreRAII<> sem_raii (dev->audio_ring_sem_);

      /* either copy the data from the ring buffer or fill with 0 */
      if (!dev->audio_ring_.read_multiple (
            dev->audio_buf_.data (), AUDIO_ENGINE->nframes_))
        {
          dsp_fill (
            dev->audio_buf_.data (), DENORMAL_PREVENTION_VAL (AUDIO_ENGINE),
            AUDIO_ENGINE->nframes_);
        }
    }
}

void
AudioPort::sum_data_from_rtaudio (
  const nframes_t start_frame,
  const nframes_t nframes)
{
  z_return_if_fail (
    // is_input() &&
    audio_backend_is_rtaudio (AUDIO_ENGINE->audio_backend_));

  for (auto &dev : rtaudio_ins_)
    {
      dsp_add2 (
        &buf_.data ()[start_frame], &dev->audio_buf_.data ()[start_frame],
        nframes);
    }
}
#endif // HAVE_RTAUDIO

bool
AudioPort::has_sound () const
{
  z_return_val_if_fail (
    this->buf_.size () >= AUDIO_ENGINE->block_length_, false);
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      if (fabsf (this->buf_[i]) > 0.0000001f)
        {
          return true;
        }
    }
  return false;
}

bool
AudioPort::is_stereo_port () const
{
  return ENUM_BITSET_TEST (
           PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoL)
         || ENUM_BITSET_TEST (
           PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoR);
}

void
AudioPort::process (const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  if (noroll)
    {
      dsp_fill (
        &buf_.data ()[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      return;
    }

  const auto &id = id_;
  const auto  owner_type = id.owner_type_;
  auto        track = [&] () -> Track * {
    if (owner_type == PortIdentifier::OwnerType::TrackProcessor
        || owner_type == PortIdentifier::OwnerType::Track
        || owner_type == PortIdentifier::OwnerType::Channel
        || (owner_type == PortIdentifier::OwnerType::Fader
            && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Prefader)
                || ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Postfader)))
        || (owner_type == PortIdentifier::OwnerType::Plugin && id_.plugin_id_.slot_type_ == zrythm::plugins::PluginSlotType::Instrument))
      {
        return ZRYTHM_TESTING ? get_track (true) : track_;
      }
    return nullptr;
  }();
  z_return_if_fail (
    track || owner_type != PortIdentifier::OwnerType::TrackProcessor);

  const auto processable_track = dynamic_cast<ProcessableTrack *> (track);
  const auto channel_track = dynamic_cast<ChannelTrack *> (track);
  const auto recordable_track = dynamic_cast<RecordableTrack *> (track);

  const bool is_stereo = is_stereo_port ();

  /* only consider incoming external data if armed for recording (if the port is
   * owner by a track), otherwise always consider incoming external data */
  if ((owner_type != PortIdentifier::OwnerType::TrackProcessor || (owner_type == PortIdentifier::OwnerType::TrackProcessor && recordable_track && recordable_track->get_recording())) && is_input())
    {
      switch (AUDIO_ENGINE->audio_backend_)
        {
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          receive_audio_data_from_jack (
            time_nfo.local_offset_, time_nfo.nframes_);
          break;
#endif
        case AudioBackend::AUDIO_BACKEND_DUMMY:
          sum_data_from_dummy (time_nfo.local_offset_, time_nfo.nframes_);
          break;
        default:
          break;
        }
    }

  for (size_t k = 0; k < srcs_.size (); k++)
    {
      const auto * src_port = srcs_[k];
      const auto  &conn = src_connections_[k];
      if (!conn.enabled_)
        continue;

      const float multiplier = conn.multiplier_;

      /* sum the signals */
      if (math_floats_equal_epsilon (multiplier, 1.f, 0.00001f)) [[likely]]
        {
          dsp_add2 (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
        }
      else
        {
          dsp_mix2 (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], 1.f, multiplier,
            time_nfo.nframes_);
        }

      if (owner_type == PortIdentifier::OwnerType::Fader)
        {
          constexpr float minf = -2.f;
          constexpr float maxf = 2.f;
          float           abs_peak =
            dsp_abs_max (&buf_[time_nfo.local_offset_], time_nfo.nframes_);
          if (abs_peak > maxf)
            {
              /* this limiting wastes around 50% of port processing so only
               * do it on CV connections and faders if they exceed maxf */
              dsp_limit1 (
                &buf_[time_nfo.local_offset_], minf, maxf, time_nfo.nframes_);
            }
        }
    }

  if (is_output ())
    {
      switch (AUDIO_ENGINE->audio_backend_)
        {
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          send_audio_data_to_jack (time_nfo.local_offset_, time_nfo.nframes_);
          break;
#endif
        default:
          break;
        }
    }

  if (time_nfo.local_offset_ + time_nfo.nframes_ == AUDIO_ENGINE->block_length_)
    {
      audio_ring_->force_write_multiple (
        buf_.data (), AUDIO_ENGINE->block_length_);
    }

  /* if track output (to be shown on mixer) */
  if (
    owner_type == PortIdentifier::OwnerType::Channel && is_stereo
    && is_output ())
    {
      z_return_if_fail (channel_track);
      auto &ch = channel_track->channel_;

      /* calculate meter values */
      if (
        this == &ch->stereo_out_->get_l () || this == &ch->stereo_out_->get_r ())
        {
          /* reset peak if needed */
          gint64 time_now = g_get_monotonic_time ();
          if (time_now - peak_timestamp_ > TIME_TO_RESET_PEAK)
            peak_ = -1.f;

          bool changed = dsp_abs_max_with_existing_peak (
            &buf_[time_nfo.local_offset_], &peak_, time_nfo.nframes_);
          if (changed)
            {
              peak_timestamp_ = g_get_monotonic_time ();
            }
        }
    }

  /* if bouncing tracks directly to master (e.g., when bouncing the track on
   * its own without parents), clear master input */
  if (AUDIO_ENGINE->bounce_mode_ > BounceMode::BOUNCE_OFF && !AUDIO_ENGINE->bounce_with_parents_ && (this == &P_MASTER_TRACK->processor_->stereo_in_->get_l () || this == &P_MASTER_TRACK->processor_->stereo_in_->get_r ())) [[unlikely]]
    {
      dsp_fill (
        &buf_[time_nfo.local_offset_], AUDIO_ENGINE->denormal_prevention_val_,
        time_nfo.nframes_);
    }

  /* if bouncing track directly to master (e.g., when bouncing the track on
   * its own without parents), add the buffer to master output */
  if (AUDIO_ENGINE->bounce_mode_ > BounceMode::BOUNCE_OFF 
  && (owner_type == PortIdentifier::OwnerType::Channel || owner_type == PortIdentifier::OwnerType::TrackProcessor || (owner_type == PortIdentifier::OwnerType::Fader && ENUM_BITSET_TEST (PortIdentifier::Flags2, id.flags2_, PortIdentifier::Flags2::Prefader)) || (owner_type == PortIdentifier::OwnerType::Plugin && id.plugin_id_.slot_type_ == zrythm::plugins::PluginSlotType::Instrument)) && is_stereo && is_output() && (track != nullptr) && track->bounce_to_master_) [[unlikely]]
    {
      auto add_to_master = [&] (const bool left) {
        auto &dest =
          left ? P_MASTER_TRACK->channel_->stereo_out_->get_l ()
               : P_MASTER_TRACK->channel_->stereo_out_->get_r ();
        dsp_add2 (
          &dest.buf_[time_nfo.local_offset_],
          &this->buf_[time_nfo.local_offset_], time_nfo.nframes_);
      };

      switch (AUDIO_ENGINE->bounce_step_)
        {
        case BounceStep::BeforeInserts:
          {
            const auto &tp = processable_track->processor_;
            z_return_if_fail (tp);
            if (track->is_instrument ())
              {
                if (this == channel_track->channel_->instrument_->l_out_)
                  add_to_master (true);
                if (this == channel_track->channel_->instrument_->r_out_)
                  add_to_master (false);
              }
            else if (tp->stereo_out_ && track->bounce_)
              {
                if (this == &tp->stereo_out_->get_l ())
                  add_to_master (true);
                else if (this == &tp->stereo_out_->get_r ())
                  add_to_master (false);
              }
          }
          break;
        case BounceStep::PreFader:
          {
            const auto &prefader = channel_track->channel_->prefader_;
            if (this == &prefader->stereo_out_->get_l ())
              add_to_master (true);
            else if (this == &prefader->stereo_out_->get_r ())
              add_to_master (false);
          }
          break;
        case BounceStep::PostFader:
          {
            const auto &ch = channel_track->channel_;
            if (!track->is_master ())
              {
                if (this == &ch->stereo_out_->get_l ())
                  add_to_master (true);
                else if (this == &ch->stereo_out_->get_r ())
                  add_to_master (false);
              }
          }
          break;
        }
    }
}

void
AudioPort::apply_pan (
  float           pan,
  PanLaw          pan_law,
  PanAlgorithm    pan_algo,
  nframes_t       start_frame,
  const nframes_t nframes)
{
  float calc_r, calc_l;
  pan_get_calc_lr (pan_law, pan_algo, pan, &calc_l, &calc_r);

  /* if stereo R */
  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, id_.flags_, PortIdentifier::Flags::StereoR))
    {
      dsp_mul_k2 (&buf_[start_frame], calc_r, nframes);
    }
  /* else if stereo L */
  else
    {
      dsp_mul_k2 (&buf_[start_frame], calc_l, nframes);
    }
}

void
AudioPort::apply_fader (float amp, nframes_t start_frame, const nframes_t nframes)
{
  dsp_mul_k2 (&buf_[start_frame], amp, nframes);
}

StereoPorts::StereoPorts (bool input, std::string name, std::string symbol)
    : StereoPorts (
        AudioPort (
          fmt::format ("{} L", name),
          input ? PortFlow::Input : PortFlow::Output),
        AudioPort (
          fmt::format ("{} R", name),
          input ? PortFlow::Input : PortFlow::Output))

{
  l_->id_.flags_ |= PortIdentifier::Flags::StereoL;
  l_->id_.sym_ = fmt::format ("{}_l", symbol);
  r_->id_.flags_ |= PortIdentifier::Flags::StereoR;
  r_->id_.sym_ = fmt::format ("{}_r", symbol);
}

StereoPorts::StereoPorts (const AudioPort &l, const AudioPort &r)
{
  l_ = l.clone_unique ();
  r_ = r.clone_unique ();
  l_->id_.flags_ |= PortIdentifier::Flags::StereoL;
  r_->id_.flags_ |= PortIdentifier::Flags::StereoR;
}

void
StereoPorts::disconnect ()
{
  l_->disconnect_all ();
  r_->disconnect_all ();
}

void
StereoPorts::connect_to (StereoPorts &dest, bool locked)
{
  l_->connect_to (*dest.l_, locked);
  r_->connect_to (*dest.r_, locked);
}