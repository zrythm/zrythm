// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "audio/control_room.h"
#include "audio/fader.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm.h"

static void
init_common (ControlRoom * self)
{
  self->schema_version =
    CONTROL_ROOM_SCHEMA_VERSION;

  /* set the monitor volume */
  float amp =
    ZRYTHM_TESTING
      ? 1.f
      : (float) g_settings_get_double (
        S_MONITOR, "monitor-vol");
  fader_set_amp (self->monitor_fader, amp);

  double lower, upper;

  /* init listen/mute/dim faders */
  self->mute_fader = fader_new (
    FADER_TYPE_GENERIC, false, NULL, self, NULL);
  amp =
    ZRYTHM_TESTING
      ? 0.f
      : (float) g_settings_get_double (
        S_MONITOR, "mute-vol");
  self->mute_fader->amp->deff =
    ZRYTHM_TESTING
      ? 0.f
      : (float) settings_get_default_value_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "mute-vol");
  if (!ZRYTHM_TESTING)
    {
      settings_get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "mute-vol", &lower, &upper);
      self->mute_fader->amp->minf = (float) lower;
      self->mute_fader->amp->maxf = (float) upper;
    }
  fader_set_amp (self->mute_fader, amp);

  self->listen_fader = fader_new (
    FADER_TYPE_GENERIC, false, NULL, self, NULL);
  amp =
    ZRYTHM_TESTING
      ? 1.f
      : (float) g_settings_get_double (
        S_MONITOR, "listen-vol");
  self->listen_fader->amp->deff =
    ZRYTHM_TESTING
      ? 1.f
      : (float) settings_get_default_value_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "listen-vol");
  fader_set_amp (self->listen_fader, amp);
  if (!ZRYTHM_TESTING)
    {
      settings_get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "listen-vol", &lower, &upper);
      self->listen_fader->amp->minf = (float) lower;
      self->listen_fader->amp->maxf = (float) upper;
    }

  self->dim_fader = fader_new (
    FADER_TYPE_GENERIC, false, NULL, self, NULL);
  amp =
    ZRYTHM_TESTING
      ? 0.1f
      : (float) g_settings_get_double (
        S_MONITOR, "dim-vol");
  self->dim_fader->amp->deff =
    ZRYTHM_TESTING
      ? 0.1f
      : (float) settings_get_default_value_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "dim-vol");
  if (!ZRYTHM_TESTING)
    {
      settings_get_range_double (
        GSETTINGS_ZRYTHM_PREFIX ".monitor",
        "dim-vol", &lower, &upper);
      self->dim_fader->amp->minf = (float) lower;
      self->dim_fader->amp->maxf = (float) upper;
    }
  fader_set_amp (self->dim_fader, amp);

  fader_set_mono_compat_enabled (
    self->monitor_fader,
    ZRYTHM_TESTING
      ? false
      : g_settings_get_boolean (S_MONITOR, "mono"),
    F_NO_PUBLISH_EVENTS);

  self->dim_output =
    ZRYTHM_TESTING
      ? false
      : g_settings_get_boolean (
        S_MONITOR, "dim-output");
  bool mute =
    ZRYTHM_TESTING
      ? false
      : g_settings_get_boolean (S_MONITOR, "mute");
  self->monitor_fader->mute->control =
    mute ? 1.f : 0.f;
}

/**
 * Inits the control room from a project.
 */
void
control_room_init_loaded (
  ControlRoom * self,
  AudioEngine * engine)
{
  self->audio_engine = engine;
  fader_init_loaded (
    self->monitor_fader, NULL, self, NULL);

  init_common (self);
}

/**
 * Creates a new control room.
 */
ControlRoom *
control_room_new (AudioEngine * engine)
{
  ControlRoom * self = object_new (ControlRoom);
  self->audio_engine = engine;

  self->monitor_fader = fader_new (
    FADER_TYPE_MONITOR, false, NULL, self, NULL);

  init_common (self);

  return self;
}

/**
 * Sets dim_output to on/off and notifies interested
 * parties.
 */
void
control_room_set_dim_output (
  ControlRoom * self,
  int           dim_output)
{
  self->dim_output = dim_output;
}

/**
 * Used during serialization.
 */
ControlRoom *
control_room_clone (const ControlRoom * src)
{
  ControlRoom * self = object_new (ControlRoom);
  self->schema_version =
    CONTROL_ROOM_SCHEMA_VERSION;

  self->monitor_fader =
    fader_clone (src->monitor_fader);

  return self;
}

void
control_room_free (ControlRoom * self)
{
  object_free_w_func_and_null (
    fader_free, self->monitor_fader);
  object_free_w_func_and_null (
    fader_free, self->listen_fader);
  object_free_w_func_and_null (
    fader_free, self->mute_fader);
  object_free_w_func_and_null (
    fader_free, self->dim_fader);

  object_zero_and_free (self);
}
