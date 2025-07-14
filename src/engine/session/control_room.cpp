// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "engine/session/control_room.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/fader.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::engine::session
{
ControlRoom::ControlRoom (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  AudioEngine *                    engine)
    : audio_engine_ (engine), port_registry_ (port_registry),
      param_registry_ (param_registry)
{
  monitor_fader_ = std::make_unique<Fader> (
    *port_registry_, *param_registry_, Fader::Type::Monitor, nullptr, this,
    nullptr);
  init_common ();
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
  mute_fader_ = std::make_unique<Fader> (
    *port_registry_, *param_registry_, Fader::Type::Generic, nullptr, this,
    nullptr);
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

  listen_fader_ = std::make_unique<Fader> (
    *port_registry_, *param_registry_, Fader::Type::Generic, nullptr, this,
    nullptr);
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

  dim_fader_ = std::make_unique<Fader> (
    *port_registry_, *param_registry_, Fader::Type::Generic, nullptr, this,
    nullptr);
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
  monitor_fader_->init_loaded (
    *port_registry_, *param_registry_, nullptr, this, nullptr);
  init_common ();
}
}
