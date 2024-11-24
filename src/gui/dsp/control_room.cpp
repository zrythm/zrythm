// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_room.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/fader.h"

#include "utils/gtest_wrapper.h"

using namespace zrythm;

ControlRoom::ControlRoom (AudioEngine * engine) : audio_engine_ (engine)
{
  monitor_fader_ = std::make_unique<Fader> (
    Fader::Type::Monitor, false, nullptr, this, nullptr);
  init_common ();
}

void
ControlRoom::init_common ()
{
  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::monitorVolume ();
  monitor_fader_->set_amp (amp);

  /* init listen/mute/dim faders */
  mute_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.f
      : gui::SettingsManager::monitorMuteVolume ();
  mute_fader_->amp_->deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.f
      : gui::SettingsManager::get_default_monitorMuteVolume ();
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      mute_fader_->amp_->minf_ = 0.f;
      mute_fader_->amp_->maxf_ = 0.5f;
    }
  mute_fader_->set_amp (amp);

  listen_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::monitorListenVolume ();
  listen_fader_->amp_->deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : gui::SettingsManager::get_instance ()->get_default_monitorListenVolume ();
  listen_fader_->set_amp (amp);
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      listen_fader_->amp_->minf_ = 0.5f;
      listen_fader_->amp_->maxf_ = 2.f;
    }

  dim_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.1f
      : gui::SettingsManager::monitorDimVolume ();
  dim_fader_->amp_->deff_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 0.1f
      : gui::SettingsManager::get_default_monitorDimVolume ();
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      dim_fader_->amp_->minf_ = 0.f;
      dim_fader_->amp_->maxf_ = 0.5f;
    }
  dim_fader_->set_amp (amp);

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
  monitor_fader_->mute_->set_control_value (mute ? 1.f : 0.f, false, false);
}

void
ControlRoom::init_after_cloning (const ControlRoom &other)
{
  monitor_fader_ = other.monitor_fader_->clone_unique ();
}

bool
ControlRoom::is_in_active_project () const
{
  return audio_engine_ && audio_engine_->is_in_active_project ();
}

void
ControlRoom::init_loaded (AudioEngine * engine)
{
  audio_engine_ = engine;
  monitor_fader_->init_loaded (nullptr, this, nullptr);
  init_common ();
}
