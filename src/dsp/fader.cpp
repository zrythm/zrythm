// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/balance_control.h"
#include "dsp/channel.h"
#include "dsp/control_port.h"
#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/fader.h"
#include "dsp/group_target_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/debug.h"
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
      Port * port = (Port *) g_ptr_array_index (ports, i);
      port->init_loaded (self);
    }
  g_ptr_array_unref (ports);

  fader_set_amp ((void *) self, self->amp->control_);
}

/**
 * Appends the ports owned by fader to the given
 * array.
 */
void
fader_append_ports (const Fader * self, GPtrArray * ports)
{
  auto add_port = [ports] (Port * port) {
    if (port)
      g_ptr_array_add (ports, port);
  };

  add_port (self->amp);
  add_port (self->balance);
  add_port (self->mute);
  add_port (self->solo);
  add_port (self->listen);
  add_port (self->mono_compat_enabled);
  add_port (self->swap_phase);
  if (self->stereo_in)
    {
      add_port (&self->stereo_in->get_l ());
      add_port (&self->stereo_in->get_r ());
    }
  if (self->stereo_out)
    {
      add_port (&self->stereo_out->get_l ());
      add_port (&self->stereo_out->get_r ());
    }
  add_port (self->midi_in);
  add_port (self->midi_out);
}

Port *
fader_create_swap_phase_port (Fader * self, bool passthrough)
{
  Port * swap_phase = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Swap Phase") : _ ("Fader Swap Phase"));
  swap_phase->id_.sym_ =
    passthrough ? "prefader_swap_phase" : "fader_swap_phase";
  swap_phase->id_.flags2_ |= PortIdentifier::Flags2::FADER_SWAP_PHASE;
  swap_phase->id_.flags_ |= PortIdentifier::Flags::TOGGLE;

  return swap_phase;
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
  self->midi_mode = MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER;

  /* set volume */
  self->volume = 0.0f;
  float amp = 1.f;
  self->amp = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Volume") : _ ("Fader Volume"),
    PortIdentifier::OwnerType::FADER, self);
  self->amp->id_.sym_ = passthrough ? "prefader_volume" : "fader_volume";
  self->amp->deff_ = amp;
  self->amp->minf_ = 0.f;
  self->amp->maxf_ = 2.f;
  self->amp->set_control_value (amp, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  self->fader_val = math_get_fader_val_from_amp (amp);
  self->amp->id_.flags_ |= PortIdentifier::Flags::AMPLITUDE;
  if (
    (type == FaderType::FADER_TYPE_AUDIO_CHANNEL
     || type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->amp->id_.flags_ |= PortIdentifier::Flags::AUTOMATABLE;
      self->amp->id_.flags_ |= PortIdentifier::Flags::CHANNEL_FADER;
    }

  /* set phase */
  self->phase = 0.0f;

  /* set pan */
  float balance = 0.5f;
  self->balance = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Balance") : _ ("Fader Balance"),
    PortIdentifier::OwnerType::FADER, self);
  self->balance->id_.sym_ = passthrough ? "prefader_balance" : "fader_balance";
  self->balance->set_control_value (balance, 0, 0);
  self->balance->id_.flags_ |= PortIdentifier::Flags::STEREO_BALANCE;
  if (
    (type == FaderType::FADER_TYPE_AUDIO_CHANNEL
     || type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->balance->id_.flags_ |= PortIdentifier::Flags::AUTOMATABLE;
    }

  /* set mute */
  self->mute = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Mute") : _ ("Fader Mute"),
    PortIdentifier::OwnerType::FADER, self);
  self->mute->id_.sym_ = passthrough ? "prefader_mute" : "fader_mute";
  control_port_set_toggled (self->mute, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->mute->id_.flags_ |= PortIdentifier::Flags::FADER_MUTE;
  self->mute->id_.flags_ |= PortIdentifier::Flags::TOGGLE;
  if (
    (type == FaderType::FADER_TYPE_AUDIO_CHANNEL
     || type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    && !passthrough)
    {
      self->mute->id_.flags_ |= PortIdentifier::Flags::AUTOMATABLE;
    }

  /* set solo */
  self->solo = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Solo") : _ ("Fader Solo"),
    PortIdentifier::OwnerType::FADER, self);
  self->solo->id_.sym_ = passthrough ? ("prefader_solo") : ("fader_solo");
  control_port_set_toggled (self->solo, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->solo->id_.flags2_ |= PortIdentifier::Flags2::FADER_SOLO;
  self->solo->id_.flags_ |= PortIdentifier::Flags::TOGGLE;

  /* set listen */
  self->listen = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Listen") : _ ("Fader Listen"),
    PortIdentifier::OwnerType::FADER, self);
  self->listen->id_.sym_ = passthrough ? ("prefader_listen") : ("fader_listen");
  control_port_set_toggled (self->listen, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->listen->id_.flags2_ |= PortIdentifier::Flags2::FADER_LISTEN;
  self->listen->id_.flags_ |= PortIdentifier::Flags::TOGGLE;

  /* set mono compat */
  self->mono_compat_enabled = new Port (
    PortType::Control, PortFlow::Input,
    passthrough ? _ ("Prefader Mono Compat") : _ ("Fader Mono Compat"),
    PortIdentifier::OwnerType::FADER, self);
  self->mono_compat_enabled->id_.sym_ =
    passthrough ? ("prefader_mono_compat_enabled") : ("fader_mono_compat_enabled");
  control_port_set_toggled (
    self->mono_compat_enabled, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->mono_compat_enabled->id_.flags2_ |=
    PortIdentifier::Flags2::FADER_MONO_COMPAT;
  self->mono_compat_enabled->id_.flags_ |= PortIdentifier::Flags::TOGGLE;

  /* set swap phase */
  self->swap_phase = fader_create_swap_phase_port (self, passthrough);
  control_port_set_toggled (self->swap_phase, F_NO_TOGGLE, F_NO_PUBLISH_EVENTS);
  self->swap_phase->set_owner (PortIdentifier::OwnerType::FADER, self);

  if (
    type == FaderType::FADER_TYPE_AUDIO_CHANNEL
    || type == FaderType::FADER_TYPE_MONITOR
    || type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
    {
      const char * name = NULL;
      const char * sym = NULL;
      if (type == FaderType::FADER_TYPE_AUDIO_CHANNEL)
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
      else if (type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
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
      self->stereo_in = new StereoPorts (
        F_INPUT, name, sym, PortIdentifier::OwnerType::FADER, self);

      /* set proper owner */
      self->stereo_in->set_owner (PortIdentifier::OwnerType::FADER, self);

      if (type == FaderType::FADER_TYPE_AUDIO_CHANNEL)
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
      else if (type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
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
      self->stereo_out = new StereoPorts (
        F_NOT_INPUT, name, sym, PortIdentifier::OwnerType::FADER, self);

      /* set proper owner */
      self->stereo_out->set_owner (PortIdentifier::OwnerType::FADER, self);
    }

  if (type == FaderType::FADER_TYPE_MIDI_CHANNEL)
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
      self->midi_in = new Port (
        PortType::Event, PortFlow::Input, name,
        PortIdentifier::OwnerType::FADER, self);
      self->midi_in->id_.sym_ = sym;
      self->midi_in->midi_events_ = midi_events_new ();

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
      self->midi_out = new Port (
        PortType::Event, PortFlow::Output, name,
        PortIdentifier::OwnerType::FADER, self);
      self->midi_out->id_.sym_ = sym;
      self->midi_out->midi_events_ = midi_events_new ();
    }

  return self;
}

Fader *
fader_find_from_port_identifier (const PortIdentifier * id)
{
  PortIdentifier::Flags2 flag2 = id->flags2_;
  Track *                tr =
    tracklist_find_track_by_name_hash (TRACKLIST, id->track_name_hash_);
  if (
    !tr
    && ENUM_BITSET_TEST (
      PortIdentifier::Flags2, flag2,
      PortIdentifier::Flags2::SAMPLE_PROCESSOR_TRACK))
    {
      tr = tracklist_find_track_by_name_hash (
        SAMPLE_PROCESSOR->tracklist, id->track_name_hash_);
    }
  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags2, flag2, PortIdentifier::Flags2::MonitorFader))
    return MONITOR_FADER;
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, flag2,
      PortIdentifier::Flags2::SAMPLE_PROCESSOR_FADER))
    return SAMPLE_PROCESSOR->fader;
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, flag2, PortIdentifier::Flags2::PREFADER))
    {
      g_return_val_if_fail (tr, NULL);
      g_return_val_if_fail (tr->channel, NULL);
      return tr->channel->prefader;
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, flag2, PortIdentifier::Flags2::POSTFADER))
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
fader_set_muted (Fader * self, bool mute, bool fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (self->mute, mute, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
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
    (self->type != FaderType::FADER_TYPE_AUDIO_CHANNEL
     && self->type != FaderType::FADER_TYPE_MIDI_CHANNEL)
    || self->passthrough || control_port_is_toggled (self->solo))
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
          out_track = out_track->channel->get_output_track ();
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
            tracklist_find_track_by_name_hash (TRACKLIST, track->children[i]);
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
fader_set_soloed (Fader * self, bool solo, bool fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (self->solo, solo, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
    }
}

/**
 * Sets fader listen and optionally adds the action
 * to the undo stack.
 */
void
fader_set_listened (Fader * self, bool listen, bool fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  control_port_set_toggled (self->listen, listen, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
    }
}

void
fader_update_volume_and_fader_val (Fader * self)
{
  /* calculate volume */
  self->volume = math_amp_to_dbfs (self->amp->control_);

  self->fader_val = math_get_fader_val_from_amp (self->amp->control_);
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  g_return_if_fail (IS_FADER (self));

  self->amp->set_control_value (amp, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);

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
  bool    is_equal = math_floats_equal_epsilon (amp_from, amp_to, 0.0001f);
  if (!skip_if_equal || !is_equal)
    {
      GError * err = NULL;
      bool     ret = tracklist_selections_action_perform_edit_single_float (
        EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME, track, amp_from,
        amp_to, F_NOT_ALREADY_EDITED, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to change volume"));
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
  fader_amp = CLAMP (fader_amp + amp, self->amp->minf_, self->amp->maxf_);
  fader_set_amp (self, fader_amp);

  fader_update_volume_and_fader_val (self);
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp->control_;
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
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));

      GError * err = NULL;
      bool     ret = tracklist_selections_action_perform_edit_single_int (
        EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE, track,
        ENUM_VALUE_TO_INT (mode), F_NOT_ALREADY_EDITED, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to set MIDI mode"));
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
  return control_port_is_toggled (self->mono_compat_enabled);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
fader_set_mono_compat_enabled (Fader * self, bool enabled, bool fire_events)
{
  control_port_set_toggled (self->mono_compat_enabled, enabled, fire_events);

  if (
    self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
    || self->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    {
      Track * track = fader_get_track (self);
      g_return_if_fail (track);
      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
        }
    }
}

/**
 * Gets whether mono compatibility is enabled.
 */
bool
fader_get_swap_phase (Fader * self)
{
  return control_port_is_toggled (self->swap_phase);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
fader_set_swap_phase (Fader * self, bool enabled, bool fire_events)
{
  control_port_set_toggled (self->swap_phase, enabled, fire_events);

  if (
    self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
    || self->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    {
      Track * track = fader_get_track (self);
      g_return_if_fail (track);
      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
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
  return math_get_fader_val_from_amp (fader->amp->deff_);
}

void
fader_db_string_getter (void * obj, char * buf)
{
  Fader * fader = (Fader *) obj;

  sprintf (buf, "%.1f", math_amp_to_dbfs (fader->amp->control_));
}

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (Fader * self, float fader_val)
{
  self->fader_val = fader_val;
  float fader_amp = math_get_amp_val_from_fader (fader_val);
  fader_amp = CLAMP (fader_amp, self->amp->minf_, self->amp->maxf_);
  fader_set_amp (self, fader_amp);
  self->volume = math_amp_to_dbfs (fader_amp);

  if (self == MONITOR_FADER)
    {
      g_settings_set_double (S_MONITOR, "monitor-vol", (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->mute_fader)
    {
      g_settings_set_double (S_MONITOR, "mute-vol", (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->listen_fader)
    {
      g_settings_set_double (S_MONITOR, "listen-vol", (double) fader_amp);
    }
  else if (self == CONTROL_ROOM->dim_fader)
    {
      g_settings_set_double (S_MONITOR, "dim-vol", (double) fader_amp);
    }
}

Channel *
fader_get_channel (Fader * self)
{
  Track * track = fader_get_track (self);
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), NULL);

  Channel * ch = track_get_channel (track);
  g_return_val_if_fail (IS_CHANNEL_AND_NONNULL (ch), NULL);

  return ch;
}

Track *
fader_get_track (Fader * self)
{
  g_return_val_if_fail (IS_TRACK (self->track), NULL);

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
    case FaderType::FADER_TYPE_AUDIO_CHANNEL:
    case FaderType::FADER_TYPE_MONITOR:
    case FaderType::FADER_TYPE_SAMPLE_PROCESSOR:
      self->stereo_in->clear_buffer (*AUDIO_ENGINE);
      self->stereo_out->clear_buffer (*AUDIO_ENGINE);
      break;
    case FaderType::FADER_TYPE_MIDI_CHANNEL:
      self->midi_in->clear_buffer (*AUDIO_ENGINE);
      self->midi_out->clear_buffer (*AUDIO_ENGINE);
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
    case FaderType::FADER_TYPE_AUDIO_CHANNEL:
    case FaderType::FADER_TYPE_MONITOR:
    case FaderType::FADER_TYPE_SAMPLE_PROCESSOR:
      self->stereo_in->get_l ().disconnect_all ();
      self->stereo_in->get_r ().disconnect_all ();
      self->stereo_out->get_l ().disconnect_all ();
      self->stereo_out->get_r ().disconnect_all ();
      break;
    case FaderType::FADER_TYPE_MIDI_CHANNEL:
      self->midi_in->disconnect_all ();
      self->midi_out->disconnect_all ();
      break;
    default:
      break;
    }

  self->amp->disconnect_all ();
  self->balance->disconnect_all ();
  self->mute->disconnect_all ();
  self->solo->disconnect_all ();
  self->listen->disconnect_all ();
  self->mono_compat_enabled->disconnect_all ();
  self->swap_phase->disconnect_all ();
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
  dest->amp->control_ = src->amp->control_;
  dest->balance->control_ = src->balance->control_;
  dest->mute->control_ = src->mute->control_;
  dest->solo->control_ = src->solo->control_;
  dest->listen->control_ = src->listen->control_;
  dest->mono_compat_enabled->control_ = src->mono_compat_enabled->control_;
  dest->swap_phase->control_ = src->swap_phase->control_;
}

/**
 * Process the Fader.
 */
void
fader_process (Fader * self, const EngineProcessTimeInfo * const time_nfo)
{
  if (ZRYTHM_TESTING)
    {
#if 0
      g_debug (
        "%s: g_start %ld, start frame %u, nframes "
        "%u",
        __func__, time_nfo->g_start_frame_w_offset,
        time_nfo->local_offset,
        time_nfo->nframes);
#endif
    }

  Track * track = NULL;
  if (self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL)
    {
      track = fader_get_track (self);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
    }

  const int default_fade_frames = FADER_FADE_FRAMES_FOR_TYPE (self);

  bool effectively_muted = false;
  if (!self->passthrough)
    {
      /* muted if any of the following is true:
       * 1. muted
       * 2. other track(s) is soloed and this isn't
       * 3. bounce mode and the track is set to BOUNCE_OFF */
      effectively_muted =
        fader_get_muted (self)
        ||
        ((self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
          ||
          self->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
         && tracklist_has_soloed (TRACKLIST)
         && !fader_get_soloed (self)
         && !fader_get_implied_soloed (self)
         && track != P_MASTER_TRACK)
        ||
        (AUDIO_ENGINE->bounce_mode == BounceMode::BOUNCE_ON
         &&
         (self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
          || self->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
         && track
         /*track->out_signal_type == PortType::Audio &&*/
         && track->type != TrackType::TRACK_TYPE_MASTER
         && !track->bounce);

#if 0
      if (ZRYTHM_TESTING && track &&
          (self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL ||
           self->type == FaderType::FADER_TYPE_MIDI_CHANNEL))
        {
          g_message ("%s soloed %d implied soloed %d effectively muted %d",
            track->name, fader_get_soloed (self),
            fader_get_implied_soloed (self),
            effectively_muted);
        }
#endif
    }

  if (
    self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL
    || self->type == FaderType::FADER_TYPE_MONITOR
    || self->type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
    {
      /* copy the input to output */
      dsp_copy (
        &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
        &self->stereo_in->get_l ().buf_[time_nfo->local_offset],
        time_nfo->nframes);
      dsp_copy (
        &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
        &self->stereo_in->get_r ().buf_[time_nfo->local_offset],
        time_nfo->nframes);

      /* if prefader */
      if (self->passthrough)
        {

          /* if track frozen and transport is
           * rolling */
          if (track && track->frozen && TRANSPORT_IS_ROLLING)
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
                time_nfo->g_start_frame_w_offset,
                time_nfo->local_offset,
                time_nfo->nframes);
#endif
            }
        }
      else /* not prefader */
        {
          /* if monitor */
          float mute_amp;
          if (self->type == FaderType::FADER_TYPE_MONITOR)
            {
              mute_amp = AUDIO_ENGINE->denormal_prevention_val;
              float dim_amp = fader_get_amp (CONTROL_ROOM->dim_fader);

              /* if have listened tracks */
              if (tracklist_has_listened (TRACKLIST))
                {
                  /* dim signal */
                  dsp_mul_k2 (
                    &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                  dsp_mul_k2 (
                    &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);

                  /* add listened signal */
                  /* TODO add "listen" buffer on fader struct and add listened
                   * tracks to it during processing instead of looping here */
                  float listen_amp = fader_get_amp (CONTROL_ROOM->listen_fader);
                  for (int i = 0; i < TRACKLIST->num_tracks; i++)
                    {
                      Track * t = TRACKLIST->tracks[i];

                      if (
                        track_type_has_channel (t->type)
                        && t->out_signal_type == PortType::Audio
                        && track_get_listened (t))
                        {
                          Fader * f = track_get_fader (t, true);
                          dsp_mix2 (
                            &self->stereo_out->get_l ()
                               .buf_[time_nfo->local_offset],
                            &f->stereo_out->get_l ().buf_[time_nfo->local_offset],
                            1.f, listen_amp, time_nfo->nframes);
                          dsp_mix2 (
                            &self->stereo_out->get_r ()
                               .buf_[time_nfo->local_offset],
                            &f->stereo_out->get_r ().buf_[time_nfo->local_offset],
                            1.f, listen_amp, time_nfo->nframes);
                        }
                    }
                } /* endif have listened tracks */

              /* apply dim if enabled */
              if (CONTROL_ROOM->dim_output)
                {
                  dsp_mul_k2 (
                    &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                  dsp_mul_k2 (
                    &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
                    dim_amp, time_nfo->nframes);
                }
            } /* endif monitor fader */
          else
            {
              mute_amp = fader_get_amp (CONTROL_ROOM->mute_fader);

              /* add fade if changed from muted to non-muted or
               * vice versa */
              if (effectively_muted && !self->was_effectively_muted)
                {
                  g_atomic_int_set (
                    &self->fade_out_samples, default_fade_frames);
                  g_atomic_int_set (&self->fading_out, 1);
                }
              else if (!effectively_muted && self->was_effectively_muted)
                {
                  g_atomic_int_set (&self->fading_out, 0);
                  g_atomic_int_set (&self->fade_in_samples, default_fade_frames);
                }
            }

          /* handle fade in */
          int fade_in_samples = g_atomic_int_get (&self->fade_in_samples);
          if (G_UNLIKELY (fade_in_samples > 0))
            {
              z_return_if_fail_cmp (default_fade_frames, >=, fade_in_samples);
#if 0
              g_debug (
                "fading in %d samples", fade_in_samples);
#endif
              dsp_linear_fade_in_from (
                &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                default_fade_frames - fade_in_samples, default_fade_frames,
                time_nfo->nframes, mute_amp);
              dsp_linear_fade_in_from (
                &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
                default_fade_frames - fade_in_samples, default_fade_frames,
                time_nfo->nframes, mute_amp);
              fade_in_samples -= (int) time_nfo->nframes;
              fade_in_samples = MAX (fade_in_samples, 0);
              g_atomic_int_set (&self->fade_in_samples, fade_in_samples);
            }

          /* handle fade out */
          size_t faded_out_frames = 0;
          if (G_UNLIKELY (g_atomic_int_get (&self->fading_out)))
            {
              int fade_out_samples = g_atomic_int_get (&self->fade_out_samples);
              int samples_to_process =
                MAX (0, MIN (fade_out_samples, (int) time_nfo->nframes));
              if (fade_out_samples > 0)
                {
                  z_return_if_fail_cmp (
                    default_fade_frames, >=, fade_out_samples);

#if 0
                  g_debug (
                    "fading out %d frames",
                    samples_to_process);
#endif
                  dsp_linear_fade_out_to (
                    &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                    default_fade_frames - fade_out_samples, default_fade_frames,
                    (size_t) samples_to_process, mute_amp);
                  dsp_linear_fade_out_to (
                    &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
                    default_fade_frames - fade_out_samples, default_fade_frames,
                    (size_t) samples_to_process, mute_amp);
                  fade_out_samples -= samples_to_process;
                  faded_out_frames += (size_t) samples_to_process;
                  g_atomic_int_set (&self->fade_out_samples, fade_out_samples);
                }

              /* if still fading out and have no more fade out samples, silence */
              if (fade_out_samples == 0)
                {
                  size_t remaining_frames =
                    time_nfo->nframes - (size_t) samples_to_process;
#if 0
                  g_debug (
                    "silence for remaining %zu frames", remaining_frames);
#endif
                  if (remaining_frames > 0)
                    {
                      dsp_mul_k2 (
                        &self->stereo_out->get_l ()
                           .buf_[time_nfo->local_offset + faded_out_frames],
                        mute_amp, remaining_frames);
                      dsp_mul_k2 (
                        &self->stereo_out->get_r ()
                           .buf_[time_nfo->local_offset + faded_out_frames],
                        mute_amp, remaining_frames);
                      faded_out_frames += (size_t) remaining_frames;
                    }
                }
            }

          float pan = self->balance->get_control_value (0);
          float amp = self->amp->get_control_value (0);

          float calc_l, calc_r;
          balance_control_get_calc_lr (
            BalanceControlAlgorithm::BALANCE_CONTROL_ALGORITHM_LINEAR, pan,
            &calc_l, &calc_r);

          /* apply fader and pan */
          dsp_mul_k2 (
            &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
            amp * calc_l, time_nfo->nframes);
          dsp_mul_k2 (
            &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
            amp * calc_r, time_nfo->nframes);

          /* make mono if mono compat enabled */
          if (control_port_is_toggled (self->mono_compat_enabled))
            {
              dsp_make_mono (
                &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                &self->stereo_out->get_r ().buf_[time_nfo->local_offset],
                time_nfo->nframes, false);
            }

          /* swap phase if need */
          if (control_port_is_toggled (self->swap_phase))
            {
              dsp_mul_k2 (
                &self->stereo_out->get_l ().buf_[time_nfo->local_offset], -1.f,
                time_nfo->nframes);
              dsp_mul_k2 (
                &self->stereo_out->get_r ().buf_[time_nfo->local_offset], -1.f,
                time_nfo->nframes);
            }

          int fade_out_samples = g_atomic_int_get (&self->fade_out_samples);
          if (
            effectively_muted && fade_out_samples == 0
            && time_nfo->nframes - faded_out_frames > 0)
            {
#if 0
              g_debug (
                "muting %zu frames",
                time_nfo->nframes - faded_out_frames);
#endif
              /* apply mute level */
              if (mute_amp < 0.00001f)
                {
                  dsp_fill (
                    &self->stereo_out->get_l ()
                       .buf_[time_nfo->local_offset + faded_out_frames],
                    AUDIO_ENGINE->denormal_prevention_val,
                    time_nfo->nframes - faded_out_frames);
                  dsp_fill (
                    &self->stereo_out->get_r ()
                       .buf_[time_nfo->local_offset + faded_out_frames],
                    AUDIO_ENGINE->denormal_prevention_val,
                    time_nfo->nframes - faded_out_frames);
                }
              else
                {
                  dsp_mul_k2 (
                    &self->stereo_out->get_l ().buf_[time_nfo->local_offset],
                    mute_amp, time_nfo->nframes - faded_out_frames);
                  dsp_mul_k2 (
                    &self->stereo_out->get_r ()
                       .buf_[time_nfo->local_offset + faded_out_frames],
                    mute_amp, time_nfo->nframes - faded_out_frames);
                }
            }

          /* if master or monitor or sample
           * processor, hard limit the output */
          if (
            (self->type == FaderType::FADER_TYPE_AUDIO_CHANNEL && track
             && track->type == TrackType::TRACK_TYPE_MASTER)
            || self->type == FaderType::FADER_TYPE_MONITOR
            || self->type == FaderType::FADER_TYPE_SAMPLE_PROCESSOR)
            {
              dsp_limit1 (
                &self->stereo_out->get_l ().buf_[time_nfo->local_offset], -2.f,
                2.f, time_nfo->nframes);
              dsp_limit1 (
                &self->stereo_out->get_r ().buf_[time_nfo->local_offset], -2.f,
                2.f, time_nfo->nframes);
            }
        } /* fi not prefader */
    }     /* fi monitor/audio fader */
  else if (self->type == FaderType::FADER_TYPE_MIDI_CHANNEL)
    {
      if (!effectively_muted)
        {
          midi_events_append (
            self->midi_out->midi_events_, self->midi_in->midi_events_,
            time_nfo->local_offset, time_nfo->nframes, F_NOT_QUEUED);

          /* if not prefader, also apply volume
           * changes */
          if (!self->passthrough)
            {
              int num_events = self->midi_out->midi_events_->num_events;
              for (int i = 0; i < num_events; i++)
                {
                  MidiEvent * ev = &self->midi_out->midi_events_->events[i];

                  if (
                    self->midi_mode
                      == MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER
                    && midi_is_note_on (ev->raw_buffer))
                    {
                      midi_byte_t prev_vel = midi_get_velocity (ev->raw_buffer);
                      midi_byte_t new_vel =
                        (midi_byte_t) ((float) prev_vel * self->amp->control_);
                      midi_event_set_velocity (ev, MIN (new_vel, 127));
                    }
                }

              if (
                self->midi_mode == MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME
                && !math_floats_equal (self->last_cc_volume, self->amp->control_))
                {
                  /* TODO add volume event on each
                   * channel */
                }
            }
        }
    }

  self->was_effectively_muted = effectively_muted;

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
  self->amp = new Port (src->amp->clone ());
  self->phase = src->phase;
  self->balance = new Port (src->balance->clone ());
  self->mute = new Port (src->mute->clone ());
  self->solo = new Port (src->solo->clone ());
  self->listen = new Port (src->listen->clone ());
  self->mono_compat_enabled = new Port (src->mono_compat_enabled->clone ());
  self->swap_phase = new Port (src->swap_phase->clone ());
  if (src->midi_in)
    self->midi_in = new Port (src->midi_in->clone ());
  if (src->midi_out)
    self->midi_out = new Port (src->midi_out->clone ());
  if (src->stereo_in)
    self->stereo_in = new StereoPorts (src->stereo_in->clone ());
  if (src->stereo_out)
    self->stereo_out = new StereoPorts (src->stereo_out->clone ());
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
  object_delete_and_null (self->amp);
  object_delete_and_null (self->balance);
  object_delete_and_null (self->mute);
  object_delete_and_null (self->solo);
  object_delete_and_null (self->listen);
  object_delete_and_null (self->mono_compat_enabled);
  object_delete_and_null (self->swap_phase);

  object_delete_and_null (self->stereo_in);
  object_delete_and_null (self->stereo_out);

  object_delete_and_null (self->midi_in);
  object_delete_and_null (self->midi_out);

  object_zero_and_free (self);
}
