// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/fader.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "zrythm.h"

void
ControlRoom::init_common ()
{
  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING
      ? 1.f
      : static_cast<float> (g_settings_get_double (S_MONITOR, "monitor-vol"));
  monitor_fader_->set_amp (amp);

  double lower, upper;

  /* init listen/mute/dim faders */
  mute_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING
      ? 0.f
      : static_cast<float> (g_settings_get_double (S_MONITOR, "mute-vol"));
  mute_fader_->amp_->deff_ =
    ZRYTHM_TESTING
      ? 0.f
      : static_cast<float> (GSettingsManager::get_default_value_double (
          GSETTINGS_ZRYTHM_PREFIX ".monitor", "mute-vol"));
  if (!ZRYTHM_TESTING)
    {
      GSettingsManager::get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor", "mute-vol", &lower, &upper);
      mute_fader_->amp_->minf_ = static_cast<float> (lower);
      mute_fader_->amp_->maxf_ = static_cast<float> (upper);
    }
  mute_fader_->set_amp (amp);

  listen_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING
      ? 1.f
      : static_cast<float> (g_settings_get_double (S_MONITOR, "listen-vol"));
  listen_fader_->amp_->deff_ =
    ZRYTHM_TESTING
      ? 1.f
      : static_cast<float> (GSettingsManager::get_default_value_double (
          GSETTINGS_ZRYTHM_PREFIX ".monitor", "listen-vol"));
  listen_fader_->set_amp (amp);
  if (!ZRYTHM_TESTING)
    {
      GSettingsManager::get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor", "listen-vol", &lower, &upper);
      listen_fader_->amp_->minf_ = static_cast<float> (lower);
      listen_fader_->amp_->maxf_ = static_cast<float> (upper);
    }

  dim_fader_ = std::make_unique<Fader> (
    Fader::Type::Generic, false, nullptr, this, nullptr);
  amp =
    ZRYTHM_TESTING
      ? 0.1f
      : static_cast<float> (g_settings_get_double (S_MONITOR, "dim-vol"));
  dim_fader_->amp_->deff_ =
    ZRYTHM_TESTING
      ? 0.1f
      : static_cast<float> (GSettingsManager::get_default_value_double (
          GSETTINGS_ZRYTHM_PREFIX ".monitor", "dim-vol"));
  if (!ZRYTHM_TESTING)
    {
      GSettingsManager::get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor", "dim-vol", &lower, &upper);
      dim_fader_->amp_->minf_ = static_cast<float> (lower);
      dim_fader_->amp_->maxf_ = static_cast<float> (upper);
    }
  dim_fader_->set_amp (amp);

  monitor_fader_->set_mono_compat_enabled (
    ZRYTHM_TESTING ? false : g_settings_get_boolean (S_MONITOR, "mono"), false);

  dim_output_ =
    ZRYTHM_TESTING ? false : g_settings_get_boolean (S_MONITOR, "dim-output");
  bool mute =
    ZRYTHM_TESTING ? false : g_settings_get_boolean (S_MONITOR, "mute");
  monitor_fader_->mute_->set_control_value (mute ? 1.f : 0.f, false, false);
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

ControlRoom::ControlRoom (AudioEngine * engine)
{
  audio_engine_ = engine;
  monitor_fader_ = std::make_unique<Fader> (
    Fader::Type::Monitor, false, nullptr, this, nullptr);
  init_common ();
}