/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTIONS_TRACKLIST_SELECTIONS_ACTION_H__
#define __ACTIONS_TRACKLIST_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/supported_file.h"
#include "audio/track.h"
#include "gui/backend/tracklist_selections.h"
#include "settings/plugin_settings.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum TracklistSelectionsActionType
{
  TRACKLIST_SELECTIONS_ACTION_COPY,
  TRACKLIST_SELECTIONS_ACTION_CREATE,
  TRACKLIST_SELECTIONS_ACTION_DELETE,
  TRACKLIST_SELECTIONS_ACTION_EDIT,
  TRACKLIST_SELECTIONS_ACTION_MOVE,
  TRACKLIST_SELECTIONS_ACTION_PIN,
  TRACKLIST_SELECTIONS_ACTION_UNPIN,
} TracklistSelectionsActionType;

static const cyaml_strval_t
tracklist_selections_action_type_strings[] =
{
  { "Copy", TRACKLIST_SELECTIONS_ACTION_COPY },
  { "Create", TRACKLIST_SELECTIONS_ACTION_CREATE },
  { "Delete", TRACKLIST_SELECTIONS_ACTION_DELETE },
  { "Edit", TRACKLIST_SELECTIONS_ACTION_EDIT },
  { "Move", TRACKLIST_SELECTIONS_ACTION_MOVE },
  { "Pin", TRACKLIST_SELECTIONS_ACTION_PIN },
  { "Unpin", TRACKLIST_SELECTIONS_ACTION_UNPIN },
};

/**
 * Action type.
 */
typedef enum EditTracksActionType
{
  EDIT_TRACK_ACTION_TYPE_SOLO,
  EDIT_TRACK_ACTION_TYPE_MUTE,
  EDIT_TRACK_ACTION_TYPE_LISTEN,
  EDIT_TRACK_ACTION_TYPE_ENABLE,
  EDIT_TRACK_ACTION_TYPE_VOLUME,
  EDIT_TRACK_ACTION_TYPE_PAN,

  /** Direct out change. */
  EDIT_TRACK_ACTION_TYPE_DIRECT_OUT,

  /** Rename track. */
  EDIT_TRACK_ACTION_TYPE_RENAME,

  EDIT_TRACK_ACTION_TYPE_COLOR,
  EDIT_TRACK_ACTION_TYPE_COMMENT,
  EDIT_TRACK_ACTION_TYPE_ICON,

  EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE,
} EditTracksActionType;

static const cyaml_strval_t
  edit_tracks_action_type_strings[] =
{
  { "solo", EDIT_TRACK_ACTION_TYPE_SOLO },
  { "mute", EDIT_TRACK_ACTION_TYPE_MUTE },
  { "listen", EDIT_TRACK_ACTION_TYPE_LISTEN },
  { "enable", EDIT_TRACK_ACTION_TYPE_ENABLE },
  { "volume", EDIT_TRACK_ACTION_TYPE_VOLUME },
  { "pan", EDIT_TRACK_ACTION_TYPE_PAN },
  { "direct out", EDIT_TRACK_ACTION_TYPE_DIRECT_OUT },
  { "Rename", EDIT_TRACK_ACTION_TYPE_RENAME },
  { "Color", EDIT_TRACK_ACTION_TYPE_COLOR },
  { "comment", EDIT_TRACK_ACTION_TYPE_COMMENT },
  { "Icon", EDIT_TRACK_ACTION_TYPE_ICON },
  { "MIDI fader mode",
    EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE },
};

/**
 * Tracklist selections (tracks) action.
 */
typedef struct TracklistSelectionsAction
{
  UndoableAction       parent_instance;

  /** Type of action. */
  TracklistSelectionsActionType type;

  /** Position to make the tracks at.
   *
   * Used when undoing too. */
  int                   track_pos;

  /** Position to add the audio region to, if
   * applicable. */
  Position              pos;

  bool                  have_pos;

  /** Track type. */
  TrackType             track_type;

  /** Flag to know if we are making an empty
   * track. */
  int                   is_empty;

  /** PluginSetting, if making an instrument or
   * bus track from a plugin.
   *
   * If this is empty and the track type is
   * instrument, it is assumed that it's an empty
   * track. */
  PluginSetting *       pl_setting;

  /**
   * The basename of the file, if any.
   *
   * This will be used as the track name.
   */
  char *                file_basename;

  /**
   * If this is an action to create a MIDI track
   * from a MIDI file, this is the base64
   * representation so that the file does not need
   * to be stored in the project.
   *
   * @note For audio files,
   *   TracklistSelectionsAction.pool_id is used.
   */
  char *                base64_midi;

  /**
   * If this is an action to create an Audio track
   * from an audio file, this is the pool ID of the
   * audio file.
   *
   * If this is not -1, this means that an audio
   * file exists in the pool.
   */
  int                   pool_id;

  /** Source sends that need to be deleted/
   * recreated on do/undo. */
  ChannelSend **        src_sends;
  int                   num_src_sends;
  size_t                src_sends_size;

  /** Direct out tracks of the original tracks */
  int *                 out_tracks;
  int                   num_out_tracks;

  EditTracksActionType  edit_type;

  /**
   * Track positions.
   *
   * Used for actions where full selection clones
   * are not needed.
   */
  int                   tracks_before[600];
  int                   tracks_after[600];
  int                   num_tracks;

  /** Clone of the TracklistSelections, if
   * applicable. */
  TracklistSelections * tls_before;

  /** Clone of the TracklistSelections, if
   * applicable. */
  TracklistSelections * tls_after;

  /* TODO remove the following and use
   * before/after */

  /* --------------- DELTAS ---------------- */

  /**
   * Int value.
   *
   * Also used for bool.
   */
  int *                 ival_before;
  int                   ival_after;

  /* -------------- end DELTAS ------------- */

  /** Track position to direct output to. */
  int                   new_direct_out_pos;

  GdkRGBA *             colors_before;
  GdkRGBA               new_color;

  char *                new_txt;

  /** Skip do if true. */
  bool                  already_edited;

  /** Float values. */
  float                 val_before;
  float                 val_after;

} TracklistSelectionsAction;

static const cyaml_schema_field_t
  tracklist_selections_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    TracklistSelectionsAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    TracklistSelectionsAction, type,
    tracklist_selections_action_type_strings),
  YAML_FIELD_ENUM (
    TracklistSelectionsAction, track_type,
    track_type_strings),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TracklistSelectionsAction, pl_setting,
    plugin_setting_fields_schema),
  YAML_FIELD_INT (
    TracklistSelectionsAction, is_empty),
  YAML_FIELD_INT (
    TracklistSelectionsAction, track_pos),
  YAML_FIELD_INT (
    TracklistSelectionsAction, have_pos),
  YAML_FIELD_MAPPING_EMBEDDED (
    TracklistSelectionsAction, pos, position_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "tracks_before", CYAML_FLAG_DEFAULT,
    TracklistSelectionsAction, tracks_before,
    num_tracks, &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "tracks_after", CYAML_FLAG_DEFAULT,
    TracklistSelectionsAction, tracks_after,
    num_tracks, &int_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    TracklistSelectionsAction, num_tracks),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    TracklistSelectionsAction, file_basename),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    TracklistSelectionsAction, base64_midi),
  YAML_FIELD_INT (
    TracklistSelectionsAction, pool_id),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TracklistSelectionsAction, tls_before,
    tracklist_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TracklistSelectionsAction, tls_after,
    tracklist_selections_fields_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    TracklistSelectionsAction, out_tracks,
    int_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    TracklistSelectionsAction, src_sends,
    channel_send_schema),
  YAML_FIELD_ENUM (
    TracklistSelectionsAction, edit_type,
    edit_tracks_action_type_strings),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ival_before",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    TracklistSelectionsAction, ival_before,
    num_tracks, &int_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    TracklistSelectionsAction, ival_after),
  YAML_FIELD_INT (
    TracklistSelectionsAction, new_direct_out_pos),
  CYAML_FIELD_SEQUENCE_COUNT (
    "colors_before",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    TracklistSelectionsAction, colors_before,
    num_tracks, &gdk_rgba_schema_default, 0,
    CYAML_UNLIMITED),
  YAML_FIELD_MAPPING_EMBEDDED (
    TracklistSelectionsAction, new_color,
    gdk_rgba_fields_schema),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    TracklistSelectionsAction, new_txt),
  YAML_FIELD_FLOAT (
    TracklistSelectionsAction, val_before),
  YAML_FIELD_FLOAT (
    TracklistSelectionsAction, val_after),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  tracklist_selections_action_schema =
{
  YAML_VALUE_PTR (
    TracklistSelectionsAction,
    tracklist_selections_action_fields_schema),
};

void
tracklist_selections_action_init_loaded (
  TracklistSelectionsAction * self);

/**
 * Creates a new TracklistSelectionsAction.
 *
 * @param tls_before Tracklist selections to act
 *   upon.
 * @param pos Position to make the tracks at.
 * @param pl_setting Plugin setting, if any.
 * @param track Track, if single-track action. Used
 *   if \ref tls_before and \ref tls_after are NULL.
 */
UndoableAction *
tracklist_selections_action_new (
  TracklistSelectionsActionType type,
  TracklistSelections *         tls_before,
  TracklistSelections *         tls_after,
  Track *                       track,
  TrackType                     track_type,
  PluginSetting *               pl_setting,
  SupportedFile *               file_descr,
  int                           track_pos,
  const Position *              pos,
  int                           num_tracks,
  EditTracksActionType          edit_type,
  Track *                       direct_out,
  int                           ival_after,
  const GdkRGBA *               color_new,
  float                         val_before,
  float                         val_after,
  const char *                  new_txt,
  bool                          already_edited);

/**
 * @param disable_track_pos Track position to
 *   disable, or -1 to not disable any track.
 */
#define tracklist_selections_action_new_create( \
  track_type,pl_setting,file_descr,track_pos, \
  pos,num_tracks,disable_track_pos) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_CREATE, \
    NULL, NULL, NULL, track_type, pl_setting, \
    file_descr, \
    track_pos, pos, num_tracks, 0, NULL, \
    disable_track_pos, NULL, 0.f, 0.f, NULL, false)

/**
 * Creates a new TracklistSelectionsAction for an
 * audio FX track.
 */
#define tracklist_selections_action_new_create_audio_fx( \
  pl_setting,track_pos,num_tracks) \
  tracklist_selections_action_new_create ( \
    TRACK_TYPE_AUDIO_BUS, pl_setting, NULL, track_pos, \
    NULL, num_tracks, -1)

/**
 * Creates a new TracklistSelectionsAction for a
 * MIDI FX track.
 */
#define tracklist_selections_action_new_create_midi_fx( \
  pl_setting,track_pos,num_tracks) \
  tracklist_selections_action_new_create ( \
    TRACK_TYPE_MIDI_BUS, pl_setting, NULL, track_pos, \
    NULL, num_tracks, -1)

/**
 * Creates a new TracklistSelectionsAction for an
 * instrument track.
 */
#define tracklist_selections_action_new_create_instrument( \
  pl_setting,track_pos,num_tracks) \
  tracklist_selections_action_new_create ( \
    TRACK_TYPE_INSTRUMENT, pl_setting, NULL, track_pos, \
    NULL, num_tracks, -1)

/**
 * Creates a new TracklistSelectionsAction for an
 * audio group track.
 */
#define tracklist_selections_action_new_create_audio_group( \
  track_pos,num_tracks) \
  tracklist_selections_action_new_create ( \
    TRACK_TYPE_AUDIO_GROUP, NULL, NULL, track_pos, \
    NULL, num_tracks, -1)

/**
 * Creates a new TracklistSelectionsAction for a MIDI
 * track.
 */
#define tracklist_selections_action_new_create_midi( \
  track_pos,num_tracks) \
  tracklist_selections_action_new_create ( \
    TRACK_TYPE_MIDI, NULL, NULL, track_pos, \
    NULL, num_tracks, -1)

/**
 * Generic edit action.
 */
#define tracklist_selections_action_new_edit_generic( \
  type,tls_before,tls_after,already_edited) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls_before, tls_after, NULL, 0, NULL, NULL, \
    -1, NULL, \
    -1, type, NULL, false, NULL, \
    0.f, 0.f, NULL, already_edited)


/**
 * Convenience wrapper over
 * tracklist_selections_action_new() for single-track
 * float edit changes.
 */
#define tracklist_selections_action_new_edit_single_float( \
  type,track,val_before,val_after,already_edited) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    NULL, NULL, track, 0, NULL, NULL, -1, NULL, \
    -1, type, NULL, false, NULL, \
    val_before, val_after, NULL, already_edited)

/**
 * Convenience wrapper over
 * tracklist_selections_action_new() for single-track
 * int edit changes.
 */
#define tracklist_selections_action_new_edit_single_int( \
  type,track,val_after,already_edited) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    NULL, NULL, track, 0, NULL, NULL, -1, NULL, \
    -1, type, NULL, val_after, NULL, \
    0.f, 0.f, NULL, already_edited)

#define tracklist_selections_action_new_edit_mute( \
  tls_before,mute_new) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls_before, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_MUTE, NULL, \
    mute_new, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_solo( \
  tls_before,solo_new) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls_before, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_SOLO, NULL, \
    solo_new, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_listen( \
  tls_before,solo_new) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls_before, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_LISTEN, NULL, \
    solo_new, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_enable( \
  tls_before,enable_new) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls_before, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_ENABLE, NULL, \
    enable_new, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_direct_out( \
  tls,direct_out) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_DIRECT_OUT, \
    direct_out, \
    false, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_color( \
  tls,color) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_COLOR, NULL, \
    false, color, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_edit_icon( \
  tls,icon) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_ICON, NULL, \
    false, NULL, \
    0.f, 0.f, icon, false)

#define tracklist_selections_action_new_edit_comment( \
  tls,comment) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_COMMENT, NULL, \
    false, NULL, \
    0.f, 0.f, comment, false)

#define tracklist_selections_action_new_edit_rename( \
  track,name) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_EDIT, \
    NULL, NULL, track, 0, NULL, NULL, -1, NULL, \
    -1, EDIT_TRACK_ACTION_TYPE_RENAME, NULL, \
    false, NULL, \
    0.f, 0.f, name, false)

#define tracklist_selections_action_new_move( \
  tls,track_pos) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_MOVE, \
    tls, NULL, NULL, 0, NULL, NULL, track_pos, NULL, \
    -1, 0, NULL, false, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_copy( \
  tls,track_pos) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_COPY, \
    tls, NULL, NULL, 0, NULL, NULL, track_pos, NULL, \
    -1, 0, NULL, false, NULL, \
    0.f, 0.f, NULL, false)

#define tracklist_selections_action_new_delete( \
  tls) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_DELETE, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, 0, NULL, false, NULL, \
    0.f, 0.f, NULL, false)

/**
 * Toggle the current pin status of the track.
 */
#define tracklist_selections_action_new_pin( \
  tls) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_PIN, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, 0, NULL, false, NULL, \
    0.f, 0.f, NULL, false)

/**
 * Toggle the current pin status of the track.
 */
#define tracklist_selections_action_new_unpin( \
  tls) \
  tracklist_selections_action_new ( \
    TRACKLIST_SELECTIONS_ACTION_UNPIN, \
    tls, NULL, NULL, 0, NULL, NULL, -1, NULL, \
    -1, 0, NULL, false, NULL, \
    0.f, 0.f, NULL, false)

int
tracklist_selections_action_do (
  TracklistSelectionsAction * self);

int
tracklist_selections_action_undo (
  TracklistSelectionsAction * self);

char *
tracklist_selections_action_stringize (
  TracklistSelectionsAction * self);

void
tracklist_selections_action_free (
  TracklistSelectionsAction * self);

/**
 * @}
 */

#endif
