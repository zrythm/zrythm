// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/fader.h"
#include "engine/session/control_room.h"

#include <boost/unordered/unordered_flat_map.hpp>

namespace zrythm::engine::session
{
ControlRoom::ControlRoom (
  RealtimeTracksProvider rt_tracks_provider,
  QObject *              parent)
    : QObject (parent),
      monitor_fader_ (
        utils::make_qobject_unique<dsp::Fader> (
          dsp::ProcessorBase::ProcessorBaseDependencies{
            .port_registry_ = port_registry_,
            .param_registry_ = param_registry_ },
          dsp::PortType::Audio,
          true,
          false,
          [] () -> utils::Utf8String { return u8"Control Room"; },
          [] (bool fader_solo_status) { return false; })),
      rt_tracks_provider_ (std::move (rt_tracks_provider))
{
  /* init listen/mute/dim faders */
  mute_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"mute_volume") },
    dsp::ParameterRange{
      dsp::ParameterRange::Type::GainAmplitude, 0.f, 0.5f, 0.f, 0.f },
    utils::Utf8String (u8"Mute"), this);

  listen_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"listen_volume") },
    dsp::ParameterRange{
      dsp::ParameterRange::Type::GainAmplitude, 0.5f, 2.0f, 0.f, 1.f },
    utils::Utf8String (u8"Listen"), this);

  dim_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"dim_volume") },
    dsp::ParameterRange{
      dsp::ParameterRange::Type::GainAmplitude, 0.0f, 0.5f, 0.f, 0.1f },
    utils::Utf8String (u8"Dim"), this);

  monitor_fader_->set_mute_gain_callback ([this] () {
    return mute_volume_->baseValue ();
  });
  monitor_fader_->set_preprocess_audio_callback (
    [this] (
      std::pair<std::span<float>, std::span<float>> stereo_bufs,
      const EngineProcessTimeInfo                  &time_nfo) {
      const float dim_amp = dim_volume_->baseValue ();

      /* if have listened tracks */
      const auto have_listened = std::ranges::any_of (
        rt_tracks_provider_ (), [] (const auto &track_var) {
          const auto * track =
            structure::tracks::from_variant (track_var.second);
          const auto * ch = track->channel ();
          if (ch == nullptr)
            {
              return false;
            }
          return ch->fader ()->currently_listened_rt ();
        });
      if (have_listened)
        {
          /* dim signal */
          utils::float_ranges::mul_k2 (
            &stereo_bufs.first[time_nfo.local_offset_], dim_amp,
            time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_bufs.second[time_nfo.local_offset_], dim_amp,
            time_nfo.nframes_);

          /* add listened signal */
          /* TODO add "listen" buffer on fader struct and add listened
           * tracks to it during processing instead of looping here */
          const float listen_amp = listen_volume_->baseValue ();
          for (const auto &cur_t : rt_tracks_provider_ ())
            {
              std::visit (
                [&] (auto &&t) {
                  if (t->channel ())
                    {
                      if (
                        t->get_output_signal_type () == dsp::PortType::Audio
                        && t->channel ()->fader ()->currently_listened_rt ())
                        {
                          auto * f = t->channel ()->fader ();
                          utils::float_ranges::product (
                            &stereo_bufs.first[time_nfo.local_offset_],
                            f->get_stereo_out_port ().buffers ()->getReadPointer (
                              0, time_nfo.local_offset_),
                            listen_amp, time_nfo.nframes_);
                          utils::float_ranges::product (
                            &stereo_bufs.second[time_nfo.local_offset_],
                            f->get_stereo_out_port ().buffers ()->getReadPointer (
                              1, time_nfo.local_offset_),
                            listen_amp, time_nfo.nframes_);
                        }
                    }
                },
                cur_t.second);
            }
        } /* endif have listened tracks */

      /* apply dim if enabled */
      if (dim_output_)
        {
          utils::float_ranges::mul_k2 (
            &stereo_bufs.first[time_nfo.local_offset_], dim_amp,
            time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_bufs.second[time_nfo.local_offset_], dim_amp,
            time_nfo.nframes_);
        }
    });
}
}
