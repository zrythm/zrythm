// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/transport_action.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
transport_action_init_loaded (TransportAction * self)
{
}

/**
 * FIXME make a general port change action.
 */
UndoableAction *
transport_action_new_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error)
{
  TransportAction * self = object_new (TransportAction);
  UndoableAction *  ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_TRANSPORT);

  self->type = TransportActionType::TRANSPORT_ACTION_BPM_CHANGE;

  self->bpm_before = bpm_before;
  self->bpm_after = bpm_after;
  self->already_done = already_done;
  self->musical_mode =
    ZRYTHM_TESTING ? false : g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

UndoableAction *
transport_action_new_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error)
{
  TransportAction * self = object_new (TransportAction);
  UndoableAction *  ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_TRANSPORT);

  self->type = type;

  self->int_before = before;
  self->int_after = after;
  self->already_done = already_done;
  self->musical_mode =
    ZRYTHM_TESTING ? false : g_settings_get_boolean (S_UI, "musical-mode");

  return ua;
}

TransportAction *
transport_action_clone (const TransportAction * src)
{
  TransportAction * self = object_new (TransportAction);
  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->bpm_before = src->bpm_before;
  self->bpm_after = src->bpm_after;
  self->int_before = src->int_before;
  self->int_after = src->int_after;
  self->musical_mode = src->musical_mode;

  return self;
}

bool
transport_action_perform_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    transport_action_new_bpm_change, error, bpm_before, bpm_after, already_done,
    error);
}

bool
transport_action_perform_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    transport_action_new_time_sig_change, error, type, before, after,
    already_done, error);
}

static bool
need_update_positions_from_ticks (TransportAction * self)
{
  switch (self->type)
    {
    case TransportActionType::TRANSPORT_ACTION_BPM_CHANGE:
      return true;
    case TransportActionType::TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      return true;
    case TransportActionType::TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      return false;
    }

  g_return_val_if_reached (false);
}

static int
do_or_undo (TransportAction * self, bool _do, GError ** error)
{
  ControlPortChange change = {};
  switch (self->type)
    {
    case TransportActionType::TRANSPORT_ACTION_BPM_CHANGE:
      change.flag1 = PortIdentifier::Flags::BPM;
      change.real_val = _do ? self->bpm_after : self->bpm_before;
      break;
    case TransportActionType::TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      change.flag2 = PortIdentifier::Flags2::BEATS_PER_BAR;
      change.ival = _do ? self->int_after : self->int_before;
      break;
    case TransportActionType::TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      change.flag2 = change.flag2 = PortIdentifier::Flags2::BEAT_UNIT;
      change.beat_unit = tempo_track_beat_unit_to_enum (
        _do ? self->int_after : self->int_before);
      break;
    }

  /* queue change */
  router_queue_control_port_change (ROUTER, &change);

  /* run engine to apply the change */
  engine_process_prepare (AUDIO_ENGINE, 1);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = (unsigned_frame_t) PLAYHEAD->frames,
    .g_start_frame_w_offset = (unsigned_frame_t) PLAYHEAD->frames,
    .local_offset = 0,
    .nframes = 1,
  };
  router_start_cycle (ROUTER, time_nfo);
  engine_post_process (AUDIO_ENGINE, 0, 1);

  int  beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  bool update_from_ticks = need_update_positions_from_ticks (self);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true, update_from_ticks, false);

  if (self->type == TransportActionType::TRANSPORT_ACTION_BPM_CHANGE)
    {
      /* get time ratio */
      double time_ratio = 0;
      if (_do)
        {
          time_ratio = self->bpm_after / self->bpm_before;
        }
      else
        {
          time_ratio = self->bpm_before / self->bpm_after;
        }

      if (
        self->musical_mode &&
        /* doesn't work */
        false)
        {
          GError * err = NULL;
          bool     success = transport_stretch_regions (
            TRANSPORT, NULL, true, time_ratio, Z_F_NO_FORCE, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                error, err, "Failed to stretch regions");
              return -1;
            }
        }
    }

  return 0;
}

int
transport_action_do (TransportAction * self, GError ** error)
{
  if (self->already_done)
    {
      self->already_done = false;

      int   beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      bpm_t bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);

      bool update_from_ticks = need_update_positions_from_ticks (self);
      engine_update_frames_per_tick (
        AUDIO_ENGINE, beats_per_bar, bpm, AUDIO_ENGINE->sample_rate, true,
        update_from_ticks, false);
    }
  else
    {
      int ret = do_or_undo (self, true, error);
      if (ret != 0)
        {
          return ret;
        }
      tracklist_set_caches (TRACKLIST, CACHE_TYPE_PLAYBACK_SNAPSHOTS);
    }

  EVENTS_PUSH (EventType::ET_BPM_CHANGED, NULL);
  EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, NULL);

  return 0;
}

int
transport_action_undo (TransportAction * self, GError ** error)
{
  int ret = do_or_undo (self, false, error);
  if (ret != 0)
    {
      return ret;
    }

  EVENTS_PUSH (EventType::ET_BPM_CHANGED, NULL);
  EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, NULL);

  return 0;
}

char *
transport_action_stringize (TransportAction * self)
{
  switch (self->type)
    {
    case TransportActionType::TRANSPORT_ACTION_BPM_CHANGE:
      return g_strdup_printf (
        _ ("Change BPM from %.2f to %.2f"), self->bpm_before, self->bpm_after);
    case TransportActionType::TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE:
      return g_strdup_printf (
        _ ("Change beats per bar from %d to %d"), self->int_before,
        self->int_after);
    case TransportActionType::TRANSPORT_ACTION_BEAT_UNIT_CHANGE:
      return g_strdup_printf (
        _ ("Change beat unit from %d to %d"), self->int_before, self->int_after);
    }

  g_return_val_if_reached (NULL);
}

void
transport_action_free (TransportAction * self)
{
  free (self);
}
