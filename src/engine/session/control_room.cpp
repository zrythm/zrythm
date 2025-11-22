// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "engine/session/control_room.h"
#include "gui/backend/backend/project.h"

namespace zrythm::engine::session
{
ControlRoom::ControlRoom (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  AudioEngine *                    engine,
  QObject *                        parent)
    : QObject (parent), audio_engine_ (engine), port_registry_ (port_registry),
      param_registry_ (param_registry)
{
  monitor_fader_ = std::make_unique<Fader> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, true, false,
    [] () -> utils::Utf8String { return u8"Control Room"; },
    [] (bool fader_solo_status) { return false; });
  init_common ();
  monitor_fader_->set_mute_gain_callback ([this] () {
    return mute_volume_->baseValue ();
  });
  monitor_fader_->set_preprocess_audio_callback (
    [&] (
      std::pair<std::span<float>, std::span<float>> stereo_bufs,
      const EngineProcessTimeInfo                  &time_nfo) {
      const float dim_amp = dim_volume_->baseValue ();

      /* if have listened tracks */
      const auto have_listened =
        std::ranges::any_of (PROJECT->tracks_rt_, [] (const auto &track_var) {
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
          for (const auto &cur_t : PROJECT->tracks_rt_)
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

void
ControlRoom::init_common ()
{
#if 0
  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::monitorVolume ();
  monitor_fader_->set_amp (amp);
#endif

  /* init listen/mute/dim faders */
  mute_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    *port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"mute_volume") },
    dsp::ParameterRange::make_gain (2.f), utils::Utf8String (u8"Mute"), this);
#if 0
  amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.f
      : gui::SettingsManager::monitorMuteVolume ();
      mute_fader_->get_amp_port ().deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.f
      : gui::SettingsManager::get_default_monitorMuteVolume ();
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      mute_fader_->get_amp_port ().range_.minf_ = 0.f;
      mute_fader_->get_amp_port ().range_.maxf_ = 0.5f;
    }
  mute_fader_->set_amp (amp);
#endif

  listen_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    *port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"listen_volume") },
    dsp::ParameterRange::make_gain (2.f), utils::Utf8String (u8"Listen"), this);
#if 0
  amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::monitorListenVolume ();
  listen_fader_->get_amp_port ().deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::get_instance ()->get_default_monitorListenVolume ();
  listen_fader_->set_amp (amp);
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      listen_fader_->get_amp_port ().range_.minf_ = 0.5f;
      listen_fader_->get_amp_port ().range_.maxf_ = 2.f;
    }
#endif

  dim_volume_ = utils::make_qobject_unique<dsp::ProcessorParameter> (
    *port_registry_,
    dsp::ProcessorParameter::UniqueId{ utils::Utf8String (u8"dim_volume") },
    dsp::ParameterRange::make_gain (2.f), utils::Utf8String (u8"Dim"), this);
#if 0
    amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.1f
      : gui::SettingsManager::monitorDimVolume ();
  dim_fader_->get_amp_port ().deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.1f
      : gui::SettingsManager::get_default_monitorDimVolume ();
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      dim_fader_->get_amp_port ().range_.minf_ = 0.f;
      dim_fader_->get_amp_port ().range_.maxf_ = 0.5f;
    }
  dim_fader_->set_amp (amp);
#endif

#if 0
  monitor_fader_->set_mono_compat_enabled (
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? false
      : gui::SettingsManager::monitorMonoEnabled (),
    false);

  dim_output_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? false
      : gui::SettingsManager::monitorDimEnabled ();
  bool mute =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? false
      : gui::SettingsManager::monitorMuteEnabled ();
  monitor_fader_->get_mute_port ().set_control_value (
    mute ? 1.f : 0.f, false, false);
#endif

// TODO: attach to signals when there are changes for the
// monitor/mute/listen/dim gains and update user settings to remember the values
#if 0
void
Fader::set_fader_val (float fader_val)
{
  fader_val_ = fader_val;
  float fader_amp = utils::math::get_amp_val_from_fader (fader_val);
  fader_amp = get_amp_port ().range_.clamp_to_range (fader_amp);
  set_amp (fader_amp);
  volume_ = utils::math::amp_to_dbfs (fader_amp);

  if (this == MONITOR_FADER.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->mute_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorMuteVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->listen_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorListenVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->dim_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorDimVolume (fader_amp);
    }
}
#endif
}

void
init_from (
  ControlRoom           &obj,
  const ControlRoom     &other,
  utils::ObjectCloneType clone_type)
{
  // TODO
  // obj.monitor_fader_ = utils::clone_unique (*other.monitor_fader_);
}

void
ControlRoom::init_loaded (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  AudioEngine *                    engine)
{
  audio_engine_ = engine;
  port_registry_ = port_registry;
  param_registry_ = param_registry;
  init_common ();
}
}
