// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "actions/tracklist_selections.h"
#include "audio/balance_control.h"
#include "audio/channel.h"
#include "audio/control_port.h"
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/group_target_track.h"
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
fader_init_loaded (
  Fader *           self,
  Track *           track,
  ControlRoom *     control_room,
  SampleProcessor * sample_processor)
{
  self->magic = FADER_MAGIC;
  self->track = track;
  self->control_room = control_room;
  self->sample_processor = sample_processor;

  GPtrArray * ports = g_ptr_array_new ();
  fader_append_ports (self, ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      port_init_loaded (port, self);
    }
  g_ptr_array_unref (ports);

  fader_set_amp ((void *) self, self->amp->control);
}

/**
 * Appends the ports owned by fader to the given
 * array.
 */
void
fader_append_ports (
  const Fader * self,
  GPtrArray *   ports)
{
#define ADD_PORT(x) \
  if (self->x) \
  g_ptr_array_add (ports, self->x)

  ADD_PORT (amp);
  ADD_PORT (balance);
  ADD_PORT (mute);
  ADD_PORT (solo);
  ADD_PORT (listen);
  ADD_PORT (mono_compat_enabled);
  if (self->stereo_in)
    {
      ADD_PORT (stereo_in->l);
      ADD_PORT (stereo_in->r);
    }
  if (self->stereo_out)
    {
      ADD_PORT (stereo_out->l);
      ADD_PORT (stereo_out->r);
    }
  ADD_PORT (midi_in);
  ADD_PORT (midi_out);

#undef ADD_PORT
}

/**
 * Creates a new fader.
 *
 * This assumes that the channel has no plugins.
 *
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
Fader *
fader_new (
  FaderType         type,
  bool              passthrough,
  Track *           track,
  ControlRoom *     control_room,
  SampleProcessor * sample_processor)
{
  Fader * self = object_new (Fader);
  self->schema_version = FADER_SCHEMA_VERSION;
  self->magic = FADER_MAGIC;
  self->track = track;
  self->control_room = control_room;
  self->sample_processor = sample_processor;
  self->type = type;
  self->passthrough = passthrough;
  self->midi_mode = MIDI_FADER_MODE_VEL_MULTIPLIER;

  /* set volume */
  self->volume = 0.0f;
  float amp = 1.f;
  self->amp = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT,
    passthrough
      ? _ ("Prefader Volume")
      : _ ("Fader Volume"),
    PORT_OWNER_TYPE_FADER, self);
  self->amp->id.sym =
    passthrough
      ? g_strdup ("prefader_volume")
      : g_strdup ("fader_volume");
  self->amp->deff = amp;
  self->amp->minf = 0.f;
  self->amp->maxf = 2.f;
  port_set_control_value (
    self->amp, amp, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);
  self->fader_val =
    math_get_fader_val_from_amp (amp);
  self->amp->id.flags |= PORT_FLAG_AMPLITUDE;
  if (
    (type == FADER_TYPE_AUDIO_CHANNEL
     || type == FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->amp->id.flags |= PORT_FLAG_AUTOMATABLE;
      self->amp->id.flags |= PORT_FLAG_CHANNEL_FADER;
    }

  /* set phase */
  self->phase = 0.0f;

  /* set pan */
  float balance = 0.5f;
  self->balance = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT,
    passthrough
      ? _ ("Prefader Balance")
      : _ ("Fader Balance"),
    PORT_OWNER_TYPE_FADER, self);
  self->balance->id.sym =
    passthrough
      ? g_strdup ("prefader_balance")
      : g_strdup ("fader_balance");
  port_set_control_value (
    self->balance, balance, 0, 0);
  self->balance->id.flags |=
    PORT_FLAG_STEREO_BALANCE;
  if (
    (type == FADER_TYPE_AUDIO_CHANNEL
     || type == FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->balance->id.flags |=
        PORT_FLAG_AUTOMATABLE;
    }

  /* set mute */
  self->mute = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT,
    passthrough
      ? _ ("Prefader Mute")
      : _ ("Fader Mute"),
    PORT_OWNER_TYPE_FADER, self);
  self->mute->id.sym =
    passthrough
      ? g_strdup ("prefader_mute")
      : g_strdup ("fader_mute");
  control_port_set_toggled (
    self->mute, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->mute->id.flags |= PORT_FLAG_FADER_MUTE;
  self->mute->id.flags |= PORT_FLAG_TOGGLE;
  if (
    (type == FADER_TYPE_AUDIO_CHANNEL
     || type == FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->mute->id.flags |= PORT_FLAG_AUTOMATABLE;
    }

  /* set solo */
  self->solo = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT,
    passthrough
      ? _ ("Prefader Solo")
      : _ ("Fader Solo"),
    PORT_OWNER_TYPE_FADER, self);
  self->solo->id.sym =
    passthrough
      ? g_strdup ("prefader_solo")
      : g_strdup ("fader_solo");
  control_port_set_toggled (
    self->solo, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->solo->id.flags2 |= PORT_FLAG2_FADER_SOLO;
  self->solo->id.flags |= PORT_FLAG_TOGGLE;

  /* set listen */
  self->listen = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT,
    passthrough
      ? _ ("Prefader Listen")
      : _ ("Fader Listen"),
    PORT_OWNER_TYPE_FADER, self);
  self->listen->id.sym =
    passthrough
      ? g_strdup ("prefader_listen")
      : g_strdup ("fader_listen");
  control_port_set_toggled (
    self->listen, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->listen->id.flags2 |= PORT_FLAG2_FADER_LISTEN;
  self->listen->id.flags |= PORT_FLAG_TOGGLE;

  /* set mono compat */
  self->mono_compat_enabled =
    port_new_with_type_and_owner (
      TYPE_CONTROL, FLOW_INPUT,
      passthrough
        ? _ ("Prefader Mono Compat")
        : _ ("Fader Mono Compat"),
      PORT_OWNER_TYPE_FADER, self);
  self->mono_compat_enabled->id.sym =
    passthrough
      ? g_strdup ("prefader_mono_compat_enabled")
      : g_strdup ("fader_mono_compat_enabled");
  control_port_set_toggled (
    self->mono_compat_enabled, F_NO_TOGGLE,
    F_NO_PUBLISH_EVENTS);
  self->mono_compat_enabled->id.flags2 |=
    PORT_FLAG2_FADER_MONO_COMPAT;
  self->mono_compat_enabled->id.flags |=
    PORT_FLAG_TOGGLE;

  if (
    type == FADER_TYPE_AUDIO_CHANNEL
    || type == FADER_TYPE_MONITOR
    || type == FADER_TYPE_SAMPLE_PROCESSOR)
    {
      const char * name = NULL;
      const char * sym = NULL;
      if (type == FADER_TYPE_AUDIO_CHANNEL)
        {
          if (passthrough)
            {
              name = _ ("Ch Pre-Fader in");
              sym = "ch_prefader_in";
            }
          else
            {
              name = _ ("Ch Fader in");
              sym = "ch_fader_in";
            }
        }
      else if (type == FADER_TYPE_SAMPLE_PROCESSOR)
        {
          name = _ ("Sample Processor Fader in");
          sym = "sample_processor_fader_in";
        }
      else
        {
          name = _ ("Monitor Fader in");
          sym = "monitor_fader_in";
        }

      /* stereo in */
      self->stereo_in = stereo_ports_new_generic (
        F_INPUT, name, sym, PORT_OWNER_TYPE_FADER,
        self);

      /* set proper owner */
      port_set_owner (
        self->stereo_in->l, PORT_OWNER_TYPE_FADER,
        self);
      port_set_owner (
        self->stereo_in->r, PORT_OWNER_TYPE_FADER,
        self);

      if (type == FADER_TYPE_AUDIO_CHANNEL)
        {
          if (passthrough)
            {
              name = _ ("Ch Pre-Fader out");
              sym = "ch_prefader_out";
            }
          else
            {
              name = _ ("Ch Fader out");
              sym = "ch_fader_out";
            }
        }
      else if (type == FADER_TYPE_SAMPLE_PROCESSOR)
        {
          name = _ ("Sample Processor Fader out");
          sym = "sample_processor_fader_out";
        }
      else
        {
          name = _ ("Monitor Fader out");
          sym = "monitor_fader_out";
        }

      /* stereo out */
      self->stereo_out = stereo_ports_new_generic (
        F_NOT_INPUT, name, sym,
        PORT_OWNER_TYPE_FADER, self);

      /* set proper owner */
      port_set_owner (
        self->stereo_out->l, PORT_OWNER_TYPE_FADER,
        self);
      port_set_owner (
        self->stereo_out->r, PORT_OWNER_TYPE_FADER,
        self);
    }

  if (type == FADER_TYPE_MIDI_CHANNEL)
    {
      /* MIDI in */
      const char * name = NULL;
      const char * sym = NULL;
      if (passthrough)
        {
          name = _ ("Ch MIDI Pre-Fader in");
          sym = "ch_midi_prefader_in";
        }
      else
        {
          name = _ ("Ch MIDI Fader in");
          sym = "ch_midi_fader_in";
        }
      self->midi_in = port_new_with_type_and_owner (
        TYPE_EVENT, FLOW_INPUT, name,
        PORT_OWNER_TYPE_FADER, self);
      self->midi_in->id.sym = g_strdup (sym);
      self->midi_in->midi_events =
        midi_events_new ();

      /* MIDI out */
      if (passthrough)
        {
          name = _ ("Ch MIDI Pre-Fader out");
          sym = "ch_midi_prefader_out";
        }
      else
        {
          name = _ ("Ch MIDI Fader out");
          sym = "ch_midi_fader_out";
        }
      self->midi_out = port_new_with_type_and_owner (
        TYPE_EVENT, FLOW_OUTPUT, name,
        PORT_OWNER_TYPE_FADER, self);
      self->midi_out->id.sym = g_strdup (sym);
      self->midi_out->midi_events =
        midi_events_new ();
    }

  return self;
}

Fader *
fader_find_from_port_identifier (
  const PortIdentifier * id)
{
  PortFlags2 flag2 = id->flags2;
  Track * tr = tracklist_find_track_by_name_hash (
    TRACKLIST, id->track_name_hash);
  if (!tr && flag2 & PORT_FLAG2_SAMPLE_PROCESSOR_TRACK)
    {
      tr = tracklist_find_track_by_name_hash (
        SAMPLE_PROCESSOR->tracklist,
        id->track_name_hash);
    }
  if (flag2 & PORT_FLAG2_MONITOR_FADER)
    return MONITOR_FADER;
  else if (flag2 & PORT_FLAG2_SAMPLE_PROCESSOR_FADER)
    return SAMPLE_PROCESSOR->fader;
  else if (flag2 & PORT_FLAG2_PREFADER)
    {
      g_return_val_if_fail (tr, NULL);
      g_return_val_if_fail (tr->channel, NULL);
      return tr->channel->prefader;
    }
  else if (flag2 & PORT_FLAG2_POSTFADER)
    {
      g_return_val_if_fail (tr, NULL);
      g_return_val_if_fail (tr->channel, NULL);
      return tr->channel->fader;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
fader_set_muted (
  Fader * self,
  bool    mute,
  bool    fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (
    self->mute, mute, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }
}

/**
 * Returns if the fader is muted.
 */
bool
fader_get_muted (const Fader * const self)
{
  return control_port_is_toggled (self->mute);
}

/**
 * Returns if the track is soloed.
 */
bool
fader_get_soloed (const Fader * const self)
{
  return control_port_is_toggled (self->solo);
}

/**
 * Returns whether the fader is not soloed on its
 * own but its direct out (or its direct out's direct
 * out, etc.) or its child (or its children's child,
 * etc.) is soloed.
 */
bool
fader_get_implied_soloed (Fader * self)
{
  /* only check channel faders */
  if (
    (self->type != FADER_TYPE_AUDIO_CHANNEL
     && self->type != FADER_TYPE_MIDI_CHANNEL)
    || self->passthrough
    || control_port_is_toggled (self->solo))
    {
      return false;
    }

  Track * track = fader_get_track (self);
  g_return_val_if_fail (track, false);

  /* check parents */
  Track * out_track = track;
  do
    {
      if (track_type_has_channel (out_track->type))
        {
          out_track = channel_get_output_track (
            out_track->channel);
          if (out_track && track_get_soloed (out_track))
            {
              return true;
            }
        }
      else
        {
          out_track = NULL;
        }
    }
  while (out_track);

  /* check children */
  if (TRACK_CAN_BE_GROUP_TARGET (track))
    {
      for (int i = 0; i < track->num_children; i++)
        {
          Track * child_track =
            tracklist_find_track_by_name_hash (
              TRACKLIST, track->children[i]);
          if (
            child_track
            && (track_get_soloed (child_track) || track_get_implied_soloed (child_track)))
            {
              return true;
            }
        }
    }

  return false;
}

/**
 * Sets track soloed and optionally adds the action
 * to the undo stack.
 */
void
fader_set_soloed (
  Fader * self,
  bool    solo,
  bool    fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (
    self->solo, solo, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }
}

/**
 * Sets fader listen and optionally adds the action
 * to the undo stack.
 */
void
fader_set_listened (
  Fader * self,
  bool    listen,
  bool    fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (
    self->listen, listen, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_STATE_CHANGED, track);
    }
}

void
fader_update_volume_and_fader_val (Fader * self)
{
  /* calculate volume */
  self->volume =
    math_amp_to_dbfs (self->amp->control);

  self->fader_val = math_get_fader_val_from_amp (
    self->amp->control);
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  g_return_if_fail (IS_FADER (self));

  port_set_control_value (
    self->amp, amp, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  fader_update_volume_and_fader_val (self);
}

/**
 * Sets the amp value with an undoable action.
 *
 * @param skip_if_equal Whether to skip the action
 *   if the amp hasn't changed.
 */
void
fader_set_amp_with_action (
  Fader * self,
  float   amp_from,
  float   amp_to,
  bool    skip_if_equal)
{
  Track * track = fader_get_track (self);
  bool    is_equal = math_floats_equal_epsilon (
       amp_from, amp_to, 0.0001f);
  if (!skip_if_equal || !is_equal)
    {
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_single_float (
          EDIT_TRACK_ACTION_TYPE_VOLUME, track,
          amp_from, amp_to, true, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _ ("Failed to change volume"));
        }
    }
}

/**
 * Adds (or subtracts if negative) to the amplitude
 * of the fader (clamped at 0.0 to 2.0).
 */
void
fader_add_amp (void * _self, sample_t amp)
{
  Fader * self = (Fader *) _self;

  float fader_amp = fader_get_amp (self);
  fader_amp = CLAMP (
    fader_amp + amp, self->amp->minf,
    self->amp->maxf);
  fader_set_amp (self, fader_amp);

  fader_update_volume_and_fader_val (self);
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp->control;
}

void
fader_set_midi_mode (
  Fader *       self,
  MidiFaderMode mode,
  bool          with_action,
  bool          fire_events)
{
  if (with_action)
    {
      Track * track = fader_get_track (self);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (track));

      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_single_int (
          EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE,
          track, mode, F_NOT_ALREADY_EDITED, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _ ("Failed to set MIDI mode"));
        }
    }
  else
    {
      self->midi_mode = mode;
    }

  if (fire_events)
    {
      /* TODO */
    }
}

/**
 * Gets whether mono compatibility is enabled.
 */
bool
fader_get_mono_compat_enabled (Fader * self)
{
  return control_port_is_toggled (
    self->mono_compat_enabled);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
fader_set_mono_compat_enabled (
  Fader * self,
  bool    enabled,
  bool    fire_events)
{
  control_port_set_toggled (
    self->mono_compat_enabled, enabled, fire_events);

  if (
    self->type == FADER_TYPE_AUDIO_CHANNEL
    || self->type == FADER_TYPE_MIDI_CHANNEL)
    {
      Track * track = fader_get_track (self);
      g_return_if_fail (track);
      if (fire_events)
        {
          EVENTS_PUSH (
            ET_TRACK_STATE_CHANGED, track);
        }
    }
}

float
fader_get_fader_val (void * self)
{
  return ((Fader *) self)->fader_val;
}

float
fader_get_default_fader_val (void * self)
{
  Fader * fader = (Fader *) self;
  return math_get_fader_val_from_amp (
    fader->amp->deff);
}

void
fader_db_string_getter (void * obj, char * buf)
{
  Fader * fader = (Fader *) obj;

  sprintf (
    buf, "%.1f",
    math_amp_to_dbfs (fader->amp->control));
}

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (Fader * self, float fader_val)
{
  self->fader_val = fader_val;
  float fader_amp =
    math_get_amp_val_from_fader (fader_val);
  fader_amp = CLAMP (
    fader_amp, self->amp->minf, self->amp->maxf);
  fader_set_amp (self, fader_amp);
  self->volume = math_amp_to_dbfs (fader_amp);

  if (self == MONITOR_FADER)
    {
      g_settings_set_double (
        S_MONITOR, "monitor-vol",
        (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->mute_fader)
    {
      g_settings_set_double (
        S_MONITOR, "mute-vol", (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->listen_fader)
    {
      g_settings_set_double (
        S_MONITOR, "listen-vol", (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->dim_fader)
    {
      g_settings_set_double (
        S_MONITOR, "dim-vol", (double) fader_amp);
    }
}

Channel *
fader_get_channel (Fader * self)
{
  Track * track = fader_get_track (self);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  Channel * ch = track_get_channel (track);
  g_return_val_if_fail (
    IS_CHANNEL_AND_NONNULL (ch), NULL);

  return ch;
}

Track *
fader_get_track (Fader * self)
{
  g_return_val_if_fail (
    IS_TRACK (self->track), NULL);

  return self->track;
}

/**
 * Clears all buffers.
 */
void
fader_clear_buffers (Fader * self)
{
  switch (self->type)
    {
    case FADER_TYPE_AUDIO_CHANNEL:
    case FADER_TYPE_MONITOR:
    case FADER_TYPE_SAMPLE_PROCESSOR:
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
      break;
    case FADER_TYPE_MIDI_CHANNEL:
      port_clear_buffer (self->midi_in);
      port_clear_buffer (self->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Disconnects all ports connected to the fader.
 */
void
fader_disconnect_all (Fader * self)
{
  switch (self->type)
    {
    case FADER_TYPE_AUDIO_CHANNEL:
    case FADER_TYPE_MONITOR:
    case FADER_TYPE_SAMPLE_PROCESSOR:
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case FADER_TYPE_MIDI_CHANNEL:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      break;
    default:
      break;
    }

  port_disconnect_all (self->amp);
  port_disconnect_all (self->balance);
  port_disconnect_all (self->mute);
  port_disconnect_all (self->solo);
  port_disconnect_all (self->listen);
  port_disconnect_all (self->mono_compat_enabled);
}

/**
 * Copy the fader values from source to dest.
 *
 * Used when cloning channels.
 */
void
fader_copy_values (Fader * src, Fader * dest)
{
  dest->volume = src->volume;
  dest->phase = src->phase;
  dest->fader_val = src->fader_val;
  dest->amp->control = src->amp->control;
  dest->balance->control = src->balance->control;
  dest->mute->control = src->mute->control;
  dest->solo->control = src->solo->control;
  dest->listen->control = src->listen->control;
  dest->mono_compat_enabled->control =
    src->mono_compat_enabled->control;
}

#if 0
/**
 * Updates the track pos of the fader.
 */
void
fader_update_track_pos (
  Fader * self,
  int     pos)
{
  self->track_pos = pos;

  if (self->amp)
    {
      port_update_track_pos (self->amp, NULL, pos);
    }
  if (self->balance)
    {
      port_update_track_pos (
        self->balance, NULL, pos);
    }
  if (self->mute)
    {
      port_update_track_pos (self->mute, NULL, pos);
    }
  if (self->solo)
    {
      port_update_track_pos (self->solo, NULL, pos);
    }
  if (self->listen)
    {
      port_update_track_pos (
        self->listen, NULL, pos);
    }
  if (self->mono_compat_enabled)
    {
      port_update_track_pos (
        self->mono_compat_enabled, NULL, pos);
    }
  if (self->stereo_in)
    {
      port_update_track_pos (
        self->stereo_in->l, NULL, pos);
      port_update_track_pos (
        self->stereo_in->r, NULL, pos);
    }
  if (self->stereo_out)
    {
      port_update_track_pos (
        self->stereo_out->l, NULL, pos);
      port_update_track_pos (
        self->stereo_out->r, NULL, pos);
    }
  if (self->midi_in)
    {
      port_update_track_pos (
        self->midi_in, NULL, pos);
    }
  if (self->midi_out)
    {
      port_update_track_pos (
        self->midi_out, NULL, pos);
    }
}
#endif

/**
 * Process the Fader.
 */
void
fader_process (
  Fader *                             self,
  const EngineProcessTimeInfo * const time_nfo)
{
  if (ZRYTHM_TESTING)
    {
#if 0
      g_debug (
        "%s: g_start %ld, start frame %u, nframes "
        "%u",
        __func__, time_nfo->g_start_frame,
        time_nfo->local_offset,
        time_nfo->nframes);
#endif
    }

  Track * track = NULL;
  if (self->type == FADER_TYPE_AUDIO_CHANNEL)
    {
      track = fader_get_track (self);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (track));
    }

  bool effectively_muted = false;
  if (!self->passthrough)
    {
      /* muted if any of the following is true:
       * 1. muted
       * 2. other track(s) is soloed and this
       *   isn't
       * 3. bounce mode and the track is set
       *   to BOUNCE_OFF */
      effectively_muted =
        fader_get_muted (self)
        ||
        ((self->type == FADER_TYPE_AUDIO_CHANNEL
          ||
          self->type == FADER_TYPE_MIDI_CHANNEL)
         && tracklist_has_soloed (TRACKLIST)
         && !fader_get_soloed (self)
         && !fader_get_implied_soloed (self)
         && track != P_MASTER_TRACK)
        ||
        (AUDIO_ENGINE->bounce_mode == BOUNCE_ON
         &&
         (self->type == FADER_TYPE_AUDIO_CHANNEL
          || self->type == FADER_TYPE_MIDI_CHANNEL)
         && track
         /*track->out_signal_type == TYPE_AUDIO &&*/
         && track->type != TRACK_TYPE_MASTER
         && !track->bounce);

#if 0
      if (ZRYTHM_TESTING && track &&
          (self->type == FADER_TYPE_AUDIO_CHANNEL ||
           self->type == FADER_TYPE_MIDI_CHANNEL))
        {
          g_message ("%s soloed %d implied soloed %d effectively muted %d",
            track->name, fader_get_soloed (self),
            fader_get_implied_soloed (self),
            effectively_muted);
        }
#endif
    }

  if (
    self->type == FADER_TYPE_AUDIO_CHANNEL
    || self->type == FADER_TYPE_MONITOR
    || self->type == FADER_TYPE_SAMPLE_PROCESSOR)
    {
      /* copy the input to output */
      dsp_copy (
        &self->stereo_out->l
           ->buf[time_nfo->local_offset],
        &self->stereo_in->l
           ->buf[time_nfo->local_offset],
        time_nfo->nframes);
      dsp_copy (
        &self->stereo_out->r
           ->buf[time_nfo->local_offset],
        &self->stereo_in->r
           ->buf[time_nfo->local_offset],
        time_nfo->nframes);

      /* if prefader */
      if (self->passthrough)
        {

          /* if track frozen and transport is
           * rolling */
          if (
            track && track->frozen
            && TRANSPORT_IS_ROLLING)
            {
#if 0
              /* get audio from clip */
              AudioClip * clip =
                audio_pool_get_clip (
                  AUDIO_POOL, track->pool_id);
              /* FIXME this is wrong - need to
               * also calculate the offset in the
               * clip */
              stereo_ports_fill_from_clip (
                self->stereo_out, clip,
                time_nfo->g_start_frame,
                time_nfo->local_offset,
                time_nfo->nframes);
#endif
            }
        }
      else /* not prefader */
        {
          /* if monitor */
          if (self->type == FADER_TYPE_MONITOR)
            {
              float dim_amp = fader_get_amp (
                CONTROL_ROOM->dim_fader);

              /* if have listened tracks */
              if (tracklist_has_listened (TRACKLIST))
                {
                  /* dim signal */
                  dsp_mul_k2 (
                    &self->stereo_out->l
                       ->buf[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                  dsp_mul_k2 (
                    &self->stereo_out->r
                       ->buf[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);

                  /* add listened signal */
                  /* TODO add "listen" buffer
                   * on fader struct and add
                   * listened tracks to it during
                   * processing instead of looping
                   * here */
                  float listen_amp = fader_get_amp (
                    CONTROL_ROOM->listen_fader);
                  for (int i = 0;
                       i < TRACKLIST->num_tracks; i++)
                    {
                      Track * t =
                        TRACKLIST->tracks[i];

                      if (
                        track_type_has_channel (
                          t->type)
                        && t->out_signal_type
                             == TYPE_AUDIO
                        && track_get_listened (t))
                        {
                          Fader * f =
                            track_get_fader (t, true);
                          dsp_mix2 (
                            &self->stereo_out->l->buf
                               [time_nfo->local_offset],
                            &f->stereo_out->l->buf
                               [time_nfo->local_offset],
                            1.f, listen_amp,
                            time_nfo->nframes);
                          dsp_mix2 (
                            &self->stereo_out->r->buf
                               [time_nfo->local_offset],
                            &f->stereo_out->r->buf
                               [time_nfo->local_offset],
                            1.f, listen_amp,
                            time_nfo->nframes);
                        }
                    }
                }

              /* apply dim if enabled */
              if (CONTROL_ROOM->dim_output)
                {
                  dsp_mul_k2 (
                    &self->stereo_out->l
                       ->buf[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                  dsp_mul_k2 (
                    &self->stereo_out->r
                       ->buf[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                }
            } /* endif monitor fader */

          float pan = port_get_control_value (
            self->balance, 0);
          float amp =
            port_get_control_value (self->amp, 0);

          float calc_l, calc_r;
          balance_control_get_calc_lr (
            BALANCE_CONTROL_ALGORITHM_LINEAR, pan,
            &calc_l, &calc_r);

          /* apply fader and pan */
          dsp_mul_k2 (
            &self->stereo_out->l
               ->buf[time_nfo->local_offset],
            amp * calc_l, time_nfo->nframes);
          dsp_mul_k2 (
            &self->stereo_out->r
               ->buf[time_nfo->local_offset],
            amp * calc_r, time_nfo->nframes);

          /* make mono if mono compat
           * enabled. equal amplitude is
           * more suitable for mono
           * compatibility checking */
          /* for reference:
           * equal power sum =
           * (L+R) * 0.7079 (-3dB)
           * equal amplitude sum =
           * (L+R) /2 (-6.02dB) */
          if (control_port_is_toggled (
                self->mono_compat_enabled))
            {
              dsp_make_mono (
                &self->stereo_out->l
                   ->buf[time_nfo->local_offset],
                &self->stereo_out->r
                   ->buf[time_nfo->local_offset],
                time_nfo->nframes, false);
            }

          if (effectively_muted)
            {
              /* apply mute level */
              float mute_amp = fader_get_amp (
                CONTROL_ROOM->mute_fader);
              if (mute_amp < 0.00001f)
                {
                  dsp_fill (
                    &self->stereo_out->l
                       ->buf[time_nfo->local_offset],
                    AUDIO_ENGINE
                      ->denormal_prevention_val,
                    time_nfo->nframes);
                  dsp_fill (
                    &self->stereo_out->r
                       ->buf[time_nfo->local_offset],
                    AUDIO_ENGINE
                      ->denormal_prevention_val,
                    time_nfo->nframes);
                }
              else
                {
                  dsp_mul_k2 (
                    &self->stereo_out->l
                       ->buf[time_nfo->local_offset],
                    mute_amp, time_nfo->nframes);
                  dsp_mul_k2 (
                    &self->stereo_out->r
                       ->buf[time_nfo->local_offset],
                    mute_amp, time_nfo->nframes);
                }
            }

          /* if master or monitor or sample
           * processor, hard limit the output */
          if (
            (self->type == FADER_TYPE_AUDIO_CHANNEL
             && track
             && track->type == TRACK_TYPE_MASTER)
            || self->type == FADER_TYPE_MONITOR
            || self->type
                 == FADER_TYPE_SAMPLE_PROCESSOR)
            {
              dsp_limit1 (
                &self->stereo_out->l
                   ->buf[time_nfo->local_offset],
                -2.f, 2.f, time_nfo->nframes);
              dsp_limit1 (
                &self->stereo_out->r
                   ->buf[time_nfo->local_offset],
                -2.f, 2.f, time_nfo->nframes);
            }
        } /* fi not prefader */
    }     /* fi monitor/audio fader */
  else if (self->type == FADER_TYPE_MIDI_CHANNEL)
    {
      if (!effectively_muted)
        {
          midi_events_append (
            self->midi_out->midi_events,
            self->midi_in->midi_events,
            time_nfo->local_offset,
            time_nfo->nframes, F_NOT_QUEUED);

          /* if not prefader, also apply volume
           * changes */
          if (!self->passthrough)
            {
              int num_events =
                self->midi_out->midi_events
                  ->num_events;
              for (int i = 0; i < num_events; i++)
                {
                  MidiEvent * ev =
                    &self->midi_out->midi_events
                       ->events[i];

                  if (
                    self->midi_mode
                      == MIDI_FADER_MODE_VEL_MULTIPLIER
                    && midi_is_note_on (
                      ev->raw_buffer))
                    {
                      midi_byte_t prev_vel =
                        midi_get_velocity (
                          ev->raw_buffer);
                      midi_byte_t new_vel =
                        (midi_byte_t)
                        ((float) prev_vel *
                         self->amp->control);
                      midi_event_set_velocity (
                        ev, MIN (new_vel, 127));
                    }
                }

              if (
                self->midi_mode
                  == MIDI_FADER_MODE_CC_VOLUME
                && !math_floats_equal (
                  self->last_cc_volume,
                  self->amp->control))
                {
                  /* TODO add volume event on each
                   * channel */
                }
            }
        }
    }

  if (ZRYTHM_TESTING)
    {
#if 0
      g_debug ("%s: done", __func__);
#endif
    }
}

Fader *
fader_clone (const Fader * src)
{
  Fader * self = object_new (Fader);
  self->schema_version = FADER_SCHEMA_VERSION;

  self->type = src->type;
  self->volume = src->volume;
  self->amp = port_clone (src->amp);
  self->phase = src->phase;
  self->balance = port_clone (src->balance);
  self->mute = port_clone (src->mute);
  self->solo = port_clone (src->solo);
  self->listen = port_clone (src->listen);
  self->mono_compat_enabled =
    port_clone (src->mono_compat_enabled);
  if (src->midi_in)
    self->midi_in = port_clone (src->midi_in);
  if (src->midi_out)
    self->midi_out = port_clone (src->midi_out);
  if (src->stereo_in)
    self->stereo_in =
      stereo_ports_clone (src->stereo_in);
  if (src->stereo_out)
    self->stereo_out =
      stereo_ports_clone (src->stereo_out);
  self->midi_mode = src->midi_mode;
  self->passthrough = src->passthrough;

  return self;
}

/**
 * Frees the fader members.
 */
void
fader_free (Fader * self)
{
#define DISCONNECT_AND_FREE(x) \
  object_free_w_func_and_null (port_free, x)

  DISCONNECT_AND_FREE (self->amp);
  DISCONNECT_AND_FREE (self->balance);
  DISCONNECT_AND_FREE (self->mute);
  DISCONNECT_AND_FREE (self->solo);
  DISCONNECT_AND_FREE (self->listen);
  DISCONNECT_AND_FREE (self->mono_compat_enabled);

#define DISCONNECT_AND_FREE_STEREO(x) \
  object_free_w_func_and_null (stereo_ports_free, x)

  DISCONNECT_AND_FREE_STEREO (self->stereo_in);
  DISCONNECT_AND_FREE_STEREO (self->stereo_out);

  DISCONNECT_AND_FREE (self->midi_in);
  DISCONNECT_AND_FREE (self->midi_out);

#undef DISCONNECT_AND_FREE
#undef DISCONNECT_AND_FREE_STEREO

  object_zero_and_free (self);
}
