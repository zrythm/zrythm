// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/audio_region.h"
#include "dsp/foldable_track.h"
#include "dsp/group_target_track.h"
#include "dsp/router.h"
#include "dsp/supported_file.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "io/midi_file.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/algorithms.h"
#include "utils/arrays.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define TYPE_IS(x) \
  (self->type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_##x)

typedef enum
{
  Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR_FAILED,
  Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR_NO_TRACKS,
} ZActionsTracklistSelectionsError;

#define Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR \
  z_actions_tracklist_selections_error_quark ()
GQuark
z_actions_tracklist_selections_error_quark (void);
G_DEFINE_QUARK (
  z - actions - tracklist - selections - error - quark,
  z_actions_tracklist_selections_error)

void
tracklist_selections_action_init_loaded (TracklistSelectionsAction * self)
{
  if (self->tls_before)
    {
      tracklist_selections_init_loaded (self->tls_before);
    }
  if (self->tls_after)
    {
      tracklist_selections_init_loaded (self->tls_after);
    }
  if (self->foldable_tls_before)
    {
      tracklist_selections_init_loaded (self->foldable_tls_before);
    }

  self->src_sends_size = (size_t) self->num_src_sends;

  for (int i = 0; i < self->num_src_sends; i++)
    {
      channel_send_init_loaded (self->src_sends[i], NULL);
    }
}

static void
copy_track_positions (TracklistSelections * sel, int * tracks, int * num_tracks)
{
  *num_tracks = sel->num_tracks;
  for (int i = 0; i < sel->num_tracks; i++)
    {
      tracks[i] = sel->tracks[i]->pos;
    }
  qsort (tracks, (size_t) *num_tracks, sizeof (int), algorithm_sort_int_cmpfunc);
}

/**
 * Resets the foldable track sizes when undoing
 * an action.
 *
 * @note Must only be used during undo.
 */
static void
reset_foldable_track_sizes (TracklistSelectionsAction * self)
{
  for (int i = 0; i < self->foldable_tls_before->num_tracks; i++)
    {
      Track * own_tr = self->foldable_tls_before->tracks[i];
      Track * prj_tr = track_find_by_name (own_tr->name);
      prj_tr->size = own_tr->size;
    }
}

/**
 * Validates the newly-created action.
 */
static bool
validate (TracklistSelectionsAction * self)
{
  if (TYPE_IS (DELETE))
    {
      if (
        !self->tls_before
        || tracklist_selections_contains_undeletable_track (self->tls_before))
        {
          return false;
        }
    }
  else if (TYPE_IS (MOVE_INSIDE))
    {
      if (
        !self->tls_before
        || tracklist_selections_contains_track_index (
          self->tls_before, self->track_pos))
        return false;
    }

  return true;
}

/**
 * Creates a new TracklistSelectionsAction.
 *
 * @param tls_before Tracklist selections to act
 *   upon.
 * @param port_connections_mgr Port connections
 *   manager at the start of the action.
 * @param pos Position to make the tracks at.
 * @param pl_setting Plugin setting, if any.
 * @param track Track, if single-track action. Used
 *   if @ref tls_before and @ref tls_after are NULL.
 * @param error To be filled in if an error occurred.
 */
UndoableAction *
tracklist_selections_action_new (
  TracklistSelectionsActionType  type,
  TracklistSelections *          tls_before,
  TracklistSelections *          tls_after,
  const PortConnectionsManager * port_connections_mgr,
  Track *                        track,
  TrackType                      track_type,
  const PluginSetting *          pl_setting,
  const SupportedFile *          file_descr,
  int                            track_pos,
  int                            lane_pos,
  const Position *               pos,
  int                            num_tracks,
  EditTrackActionType            edit_type,
  int                            ival_after,
  const GdkRGBA *                color_new,
  float                          val_before,
  float                          val_after,
  const char *                   new_txt,
  bool                           already_edited,
  GError **                      error)
{
  TracklistSelectionsAction * self = object_new (TracklistSelectionsAction);
  UndoableAction *            ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_TRACKLIST_SELECTIONS);

  position_init (&self->pos);

  if (num_tracks < 0)
    {
      num_tracks = 0;
    }

  /* --- validation --- */

  if (
    type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_CREATE
    && num_tracks <= 0)
    {
      g_critical ("attempted to create %d tracks", num_tracks);
      return NULL;
    }

  if (
    (type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY
     || type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE)
    && tracklist_selections_contains_uncopyable_track (tls_before))
    {
      g_set_error (
        error, Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR,
        Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Cannot duplicate tracks: selection contains an uncopyable track"));
      return NULL;
    }

  if (
    type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_DELETE
    && tracklist_selections_contains_undeletable_track (tls_before))
    {
      g_set_error (
        error, Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR,
        Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Cannot delete tracks: selection contains an undeletable track"));
      return NULL;
    }

  if (
    type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE
    || type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE)
    {
      Track * foldable_tr = foldable_tr = TRACKLIST->tracks[track_pos];
      g_return_val_if_fail (track_type_is_foldable (foldable_tr->type), NULL);
    }

  /* --- end validation --- */

  if (
    tls_before == TRACKLIST_SELECTIONS
    && (type != TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_EDIT || edit_type != EditTrackActionType::EDIT_TRACK_ACTION_TYPE_FOLD))
    {
      tracklist_selections_select_foldable_children (tls_before);
    }

  self->type = type;
  if (pl_setting)
    {
      self->pl_setting = plugin_setting_clone (pl_setting, F_VALIDATE);
    }
  else if (!file_descr)
    {
      self->is_empty = 1;
    }
  self->track_type = track_type;
  self->track_pos = track_pos;
  self->lane_pos = lane_pos;
  if (type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_PIN)
    {
      self->track_pos = TRACKLIST->pinned_tracks_cutoff;
    }
  else if (
    type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_UNPIN)
    {
      self->track_pos = TRACKLIST->num_tracks - 1;
    }

  self->pool_id = -1;
  if (pos)
    {
      position_set_to_pos (&self->pos, pos);
      self->have_pos = true;
    }

  /* calculate number of tracks */
  if (file_descr && track_type == TrackType::TRACK_TYPE_MIDI)
    {
      self->num_tracks = midi_file_get_num_tracks (file_descr->abs_path, true);
    }
  else
    {
      self->num_tracks = num_tracks;
    }

  /* create the file in the pool or save base64 if
   * MIDI */
  if (file_descr)
    {
      g_warning ("use async API");
      if (track_type == TrackType::TRACK_TYPE_MIDI)
        {
          GError *  err = NULL;
          uint8_t * data = NULL;
          size_t    length = 0;
          if (!g_file_get_contents (
                file_descr->abs_path, (gchar **) &data, &length, &err))
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, _ ("Failed getting contents for %s"),
                file_descr->abs_path);
              return NULL;
            }
          self->base64_midi = g_base64_encode (data, length);
        }
      else if (track_type == TrackType::TRACK_TYPE_AUDIO)
        {
          GError *    err = NULL;
          AudioClip * clip =
            audio_clip_new_from_file (file_descr->abs_path, &err);
          if (!clip)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, _ ("Failed creating audio clip from file at %s"),
                file_descr->abs_path);
              return NULL;
            }
          self->pool_id = audio_pool_add_clip (AUDIO_POOL, clip);
        }
      else
        {
          g_return_val_if_reached (NULL);
        }

      self->file_basename = g_path_get_basename (file_descr->abs_path);
    }

  bool need_full_selections = true;
  if (type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_EDIT)
    {
      need_full_selections = false;
    }

  if (tls_before)
    {
      if (tls_before->num_tracks == 0)
        {
          g_set_error_literal (
            error, Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR,
            Z_ACTIONS_TRACKLIST_SELECTIONS_ERROR_NO_TRACKS,
            _ ("No tracks selected"));
          object_free_w_func_and_null_cast (
            undoable_action_free, UndoableAction *, self);
          return NULL;
        }

      if (need_full_selections)
        {
          GError * err = NULL;
          self->tls_before = tracklist_selections_clone (tls_before, &err);
          if (err)
            {
              object_free_w_func_and_null_cast (
                undoable_action_free, UndoableAction *, self);
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s",
                _ ("Failed to clone tracklist "
                   "selections"));
              return NULL;
            }
          tracklist_selections_sort (self->tls_before, true);
          self->foldable_tls_before = tracklist_selections_new (F_NOT_PROJECT);
          for (int i = 0; i < TRACKLIST->num_tracks; i++)
            {
              Track * tr = TRACKLIST->tracks[i];
              if (track_type_is_foldable (tr->type))
                {
                  Track * clone_tr = track_clone (tr, &err);
                  if (err)
                    {
                      PROPAGATE_PREFIXED_ERROR (
                        error, err, "%s", _ ("Failed to clone track"));
                      return NULL;
                    }
                  tracklist_selections_add_track (
                    self->foldable_tls_before, clone_tr, F_NO_PUBLISH_EVENTS);
                }
            }
        }
      else
        {
          copy_track_positions (
            tls_before, self->tracks_before, &self->num_tracks);
        }
    }

  if (tls_after)
    {
      if (need_full_selections)
        {
          GError * err = NULL;
          self->tls_after = tracklist_selections_clone (tls_after, &err);
          if (err)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s",
                _ ("Failed to clone tracklist "
                   "selections: "));
              return NULL;
            }
          tracklist_selections_sort (self->tls_after, true);
        }
      else
        {
          copy_track_positions (
            tls_after, self->tracks_after, &self->num_tracks);
        }
    }

  if (tls_before && need_full_selections)
    {
      int num_before_tracks = self->tls_before->num_tracks;
      self->num_out_tracks = num_before_tracks;
      self->out_tracks = static_cast<unsigned int *> (
        calloc ((size_t) num_before_tracks, sizeof (unsigned int)));

      /* save the outputs & incoming sends */
      for (int k = 0; k < num_before_tracks; k++)
        {
          Track * clone_track = self->tls_before->tracks[k];

          if (clone_track->channel && clone_track->channel->has_output)
            {
              self->out_tracks[k] = clone_track->channel->output_name_hash;
            }
          else
            {
              self->out_tracks[k] = 0;
            }

          for (int i = 0; i < TRACKLIST->num_tracks; i++)
            {
              Track * cur_track = TRACKLIST->tracks[i];
              if (!track_type_has_channel (cur_track->type))
                {
                  continue;
                }

              for (int j = 0; j < STRIP_SIZE; j++)
                {
                  ChannelSend * send = cur_track->channel->sends[j];
                  if (channel_send_is_empty (send))
                    {
                      continue;
                    }

                  Track * target_track =
                    channel_send_get_target_track (send, cur_track);
                  g_return_val_if_fail (
                    IS_TRACK_AND_NONNULL (target_track), NULL);

                  if (target_track->pos == clone_track->pos)
                    {
                      array_double_size_if_full (
                        self->src_sends, self->num_src_sends,
                        self->src_sends_size, ChannelSend);
                      self->src_sends[self->num_src_sends++] =
                        channel_send_clone (send);
                    }

                } /* end foreach send */

            } /* end foreach track */

        } /* end foreach before track */

    } /* if need to clone tls_before */

  self->edit_type = edit_type;
  self->ival_after = ival_after;
  self->val_before = val_before;
  self->val_after = val_after;
  self->already_edited = already_edited;

  if (
    self->type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_EDIT
    && track)
    {
      self->num_tracks = 1;
      self->tracks_before[0] = track->pos;
      self->tracks_after[0] = track->pos;
    }

  if (color_new)
    {
      self->new_color = *color_new;
    }
  if (new_txt)
    {
      self->new_txt = g_strdup (new_txt);
    }

  self->ival_before = static_cast<int *> (
    calloc (MAX (1, (size_t) self->num_tracks), sizeof (int)));
  self->colors_before = static_cast<GdkRGBA *> (
    calloc (MAX (1, (size_t) self->num_tracks), sizeof (GdkRGBA)));

  if (port_connections_mgr)
    {
      self->connections_mgr_before =
        port_connections_manager_clone (port_connections_mgr);
    }

  if (!validate (self))
    {
      g_critical (
        "failed to validate tracklist "
        "selections action");
      tracklist_selections_action_free (self);
      return NULL;
    }

  return ua;
}

TracklistSelectionsAction *
tracklist_selections_action_clone (const TracklistSelectionsAction * src)
{
  TracklistSelectionsAction * self = object_new (TracklistSelectionsAction);

  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->track_type = src->track_type;
  if (src->pl_setting)
    self->pl_setting = plugin_setting_clone (src->pl_setting, F_NO_VALIDATE);
  self->is_empty = src->is_empty;
  self->track_pos = src->track_pos;
  self->lane_pos = src->lane_pos;
  self->have_pos = src->have_pos;
  self->pos = src->pos;

  if (src->num_tracks > 0)
    {
      self->ival_before = object_new_n ((size_t) src->num_tracks, int);
      self->colors_before = object_new_n ((size_t) src->num_tracks, GdkRGBA);
      for (int i = 0; i < src->num_tracks; i++)
        {
          self->tracks_before[i] = src->tracks_before[i];
          self->tracks_after[i] = src->tracks_after[i];
          self->ival_before[i] = src->ival_before[i];
          self->colors_before[i] = src->colors_before[i];
        }
      self->num_tracks = src->num_tracks;
    }
  self->ival_after = src->ival_after;
  self->new_color = src->new_color;

  self->file_basename = g_strdup (src->file_basename);
  self->base64_midi = g_strdup (src->base64_midi);
  self->pool_id = src->pool_id;
  if (src->tls_before)
    {
      GError * err = NULL;
      self->tls_before = tracklist_selections_clone (src->tls_before, &err);
      if (!self->tls_before)
        {
          g_critical ("%s", err->message);
          return NULL;
        }
    }
  if (src->tls_after)
    {
      GError * err = NULL;
      self->tls_after = tracklist_selections_clone (src->tls_after, &err);
      if (!self->tls_after)
        {
          g_critical ("%s", err->message);
          return NULL;
        }
    }
  if (src->foldable_tls_before)
    {
      GError * err = NULL;
      self->foldable_tls_before =
        tracklist_selections_clone (src->foldable_tls_before, &err);
      if (!self->foldable_tls_before)
        {
          g_critical ("%s", err->message);
          return NULL;
        }
    }

  if (src->num_out_tracks > 0)
    {
      self->out_tracks =
        object_new_n ((size_t) src->num_out_tracks, unsigned int);
      for (int i = 0; i < src->num_out_tracks; i++)
        {
          self->out_tracks[i] = src->out_tracks[i];
        }
      self->num_out_tracks = src->num_out_tracks;
    }

  if (src->num_src_sends > 0)
    {
      self->src_sends =
        object_new_n ((size_t) src->num_src_sends, ChannelSend *);
      for (int i = 0; i < src->num_src_sends; i++)
        {
          self->src_sends[i] = channel_send_clone (src->src_sends[i]);
        }
      self->num_src_sends = src->num_src_sends;
    }

  self->edit_type = src->edit_type;
  self->new_txt = g_strdup (src->new_txt);
  self->val_before = src->val_before;
  self->val_after = src->val_after;
  self->num_fold_change_tracks = src->num_fold_change_tracks;

  if (src->connections_mgr_before)
    self->connections_mgr_before =
      port_connections_manager_clone (src->connections_mgr_before);
  if (src->connections_mgr_after)
    self->connections_mgr_after =
      port_connections_manager_clone (src->connections_mgr_after);

  return self;
}

bool
tracklist_selections_action_perform (
  TracklistSelectionsActionType  type,
  TracklistSelections *          tls_before,
  TracklistSelections *          tls_after,
  const PortConnectionsManager * port_connections_mgr,
  Track *                        track,
  TrackType                      track_type,
  const PluginSetting *          pl_setting,
  const SupportedFile *          file_descr,
  int                            track_pos,
  int                            lane_pos,
  const Position *               pos,
  int                            num_tracks,
  EditTrackActionType            edit_type,
  int                            ival_after,
  const GdkRGBA *                color_new,
  float                          val_before,
  float                          val_after,
  const char *                   new_txt,
  bool                           already_edited,
  GError **                      error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    tracklist_selections_action_new, error, type, tls_before, tls_after,
    port_connections_mgr, track, track_type, pl_setting, file_descr, track_pos,
    lane_pos, pos, num_tracks, edit_type, ival_after, color_new, val_before,
    val_after, new_txt, already_edited, error);
}

/**
 * Edit or remove direct out.
 *
 * @param direct_out A track to route the
 *   selections to, or NULL to route nowhere.
 *
 * @return Whether successful.
 */
bool
tracklist_selections_action_perform_set_direct_out (
  TracklistSelections *    self,
  PortConnectionsManager * port_connections_mgr,
  Track *                  direct_out,
  GError **                error)
{
  if (direct_out)
    {
      UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
        tracklist_selections_action_new_edit_direct_out, error, self,
        port_connections_mgr, direct_out, error);
    }
  else
    {
      UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
        tracklist_selections_action_new_edit_remove_direct_out, error, self,
        port_connections_mgr, error);
    }
}

/**
 * @param add_to_project Used when the track to
 *   create is meant to be used in the project (ie
 *   not one of the tracks in the action).
 *
 * @return Non-zero if error.
 */
static int
create_track (TracklistSelectionsAction * self, int idx, GError ** error)
{
  Track * track;
  int     pos = self->track_pos + idx;

  if (self->is_empty)
    {
      const char * track_type_str = track_stringize_type (self->track_type);
      char         label[600];
      sprintf (label, _ ("%s Track"), track_type_str);

      track = track_new (self->track_type, pos, label, F_WITH_LANE);
      tracklist_insert_track (
        TRACKLIST, track, pos, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
    }
  else /* else if track is not empty */
    {
      Plugin * pl = NULL;

      /* if creating audio track from file */
      if (self->track_type == TrackType::TRACK_TYPE_AUDIO && self->pool_id >= 0)
        {
          track =
            track_new (self->track_type, pos, self->file_basename, F_WITH_LANE);
        }
      /* else if creating MIDI track from file */
      else if (
        self->track_type == TrackType::TRACK_TYPE_MIDI && self->base64_midi)
        {
          track =
            track_new (self->track_type, pos, self->file_basename, F_WITH_LANE);
        }
      /* at this point we can assume it has a
       * plugin */
      else
        {
          PluginSetting *    setting = self->pl_setting;
          PluginDescriptor * descr = setting->descr;

          track = track_new (self->track_type, pos, descr->name, F_WITH_LANE);

          GError * err = NULL;
          pl = plugin_new_from_setting (
            setting, track_get_name_hash (track),
            ZPluginSlotType::Z_PLUGIN_SLOT_INSERT, 0, &err);
          if (!pl)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, _ ("Failed to create plugin %s"), descr->name);
              return -1;
            }

          int ret = plugin_instantiate (pl, &err);
          if (ret != 0)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, _ ("Error instantiating plugin %s"), descr->name);
              plugin_free (pl);
              return -1;
            }

          /* activate */
          g_return_val_if_fail (plugin_activate (pl, F_ACTIVATE) == 0, -1);
        }

      tracklist_insert_track (
        TRACKLIST, track, track->pos, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

      if (track->channel && pl)
        {
          bool is_instrument = track->type == TrackType::TRACK_TYPE_INSTRUMENT;
          channel_add_plugin (
            track->channel,
            is_instrument
              ? ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT
              : ZPluginSlotType::Z_PLUGIN_SLOT_INSERT,
            is_instrument ? -1 : pl->id.slot, pl, F_CONFIRM, F_NOT_MOVING_PLUGIN,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
        }

      Position start_pos;
      position_init (&start_pos);
      if (self->have_pos)
        {
          position_set_to_pos (&start_pos, &self->pos);
        }
      if (self->track_type == TrackType::TRACK_TYPE_AUDIO)
        {
          /* create an audio region & add to
           * track */
          GError *  err = NULL;
          ZRegion * ar = audio_region_new (
            self->pool_id, NULL, true, NULL, 0, NULL, 0,
            ENUM_INT_TO_VALUE (BitDepth, 0), &start_pos,
            track_get_name_hash (track), 0, 0, &err);
          if (!ar)
            {
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                error, err, "Failed to create region");
              return -1;
            }
          bool success = track_add_region (
            track, ar, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "%s", "Failed to add region to track");
              return -1;
            }
        }
      else if (
        self->track_type == TrackType::TRACK_TYPE_MIDI && self->base64_midi
        && self->file_basename)
        {
          /* create a temporary midi file */
          GError * err = NULL;
          char *   dir = g_dir_make_tmp ("zrythm_tmp_midi_XXXXXX", &err);
          if (!dir)
            {
              g_critical ("failed creating tmpdir: %s", err->message);
              return -1;
            }
          char *    full_path = g_build_filename (dir, "data.MID", NULL);
          size_t    len;
          uint8_t * data = g_base64_decode (self->base64_midi, &len);
          err = NULL;
          if (!g_file_set_contents (
                full_path, (const gchar *) data, (gssize) len, &err))
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, _ ("Failed saving file %s"), full_path);
              return -1;
            }

          /* create a MIDI region from the MIDI
           * file & add to track */
          ZRegion * mr = midi_region_new_from_midi_file (
            &start_pos, full_path, track_get_name_hash (track), 0, 0, idx);
          if (mr)
            {
              bool success = track_add_region (
                track, mr, NULL, 0,
                /* name could already be generated
                 * based
                 * on the track name (if any) in
                 * the MIDI file */
                mr->name ? F_NO_GEN_NAME : F_GEN_NAME, F_NO_PUBLISH_EVENTS,
                &err);
              if (!success)
                {
                  PROPAGATE_PREFIXED_ERROR (
                    error, err, "%s", "Failed to add region to track");
                  return -1;
                }
            }
          else
            {
              g_warning (
                "Failed to create MIDI region from "
                "file %s",
                full_path);
            }

          /* remove temporary data */
          io_remove (full_path);
          io_rmdir (dir, Z_F_NO_FORCE);
          g_free (dir);
          g_free (full_path);
          g_free (data);
        }

      if (pl)
        {
          g_return_val_if_fail (pl->instantiated, -1);
        }

      if (
        pl && ZRYTHM_HAVE_UI
        && g_settings_get_boolean (S_P_PLUGINS_UIS, "open-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  return 0;
}

static void
save_or_load_port_connections (TracklistSelectionsAction * self, bool _do)
{
  undoable_action_save_or_load_port_connections (
    (UndoableAction *) self, _do, &self->connections_mgr_before,
    &self->connections_mgr_after);
}

static int
do_or_undo_create_or_delete (
  TracklistSelectionsAction * self,
  bool                        _do,
  bool                        create,
  GError **                   error)
{
  port_connections_manager_print (PORT_CONNECTIONS_MGR);

  /* if creating tracks (do create or undo
   * delete) */
  if ((create && _do) || (!create && !_do))
    {
      if (create)
        {
          for (int i = 0; i < self->num_tracks; i++)
            {
              GError * err = NULL;
              int      ret = create_track (self, i, &err);
              if (ret != 0)
                {
                  PROPAGATE_PREFIXED_ERROR (
                    error, err,
                    _ ("Failed to create track "
                       "at %d"),
                    i);
                  return ret;
                }

              /* TODO select each plugin that was
               * selected */
            }

          /* disable given track, if any (eg when
           * bouncing) */
          if (self->ival_after > -1)
            {
              g_return_val_if_fail (
                self->ival_after < TRACKLIST->num_tracks, -1);
              Track * tr_to_disable = TRACKLIST->tracks[self->ival_after];
              g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr_to_disable), -1);
              track_set_enabled (
                tr_to_disable, F_NO_ENABLE, F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT,
                F_PUBLISH_EVENTS);
            }
        }
      /* else if delete undo */
      else
        {
          int num_tracks = self->tls_before->num_tracks;

          for (int i = 0; i < num_tracks; i++)
            {
              Track * own_track = self->tls_before->tracks[i];

              /* clone our own track */
              GError * err = NULL;
              Track *  track = track_clone (own_track, &err);
              if (!track)
                {
                  PROPAGATE_PREFIXED_ERROR_LITERAL (
                    error, err, _ ("Failed to clone track"));
                  return -1;
                }

              /* remove output */
              if (track->channel)
                {
                  track->channel->has_output = false;
                  track->channel->output_name_hash = 0;
                }

              /* remove the sends (will be added
               * later) */
              if (track->channel)
                {
                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      ChannelSend * send = track->channel->sends[j];
                      send->enabled->control = 0.f;
                    }
                }

              /* insert it to the tracklist at its
               * original pos */
              track->num_children = 0;
              tracklist_insert_track (
                TRACKLIST, track, track->pos, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              /* if group track, readd all children */
              if (TRACK_CAN_BE_GROUP_TARGET (track))
                {
                  group_target_track_add_children (
                    track, own_track->children, own_track->num_children,
                    F_DISCONNECT, F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                }

              if (track->type == TrackType::TRACK_TYPE_INSTRUMENT)
                {
                  if (own_track->channel->instrument->visible)
                    {
                      EVENTS_PUSH (
                        EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                        track->channel->instrument);
                    }
                }
            }

          for (int i = 0; i < num_tracks; i++)
            {
              Track * own_track = self->tls_before->tracks[i];

              /* get the project track */
              Track * track = TRACKLIST->tracks[own_track->pos];
              if (!track_type_has_channel (track->type))
                continue;

              /* reconnect output */
              if (self->out_tracks[i] != 0)
                {
                  Track * out_track = channel_get_output_track (track->channel);
                  group_target_track_remove_child (
                    out_track, track_get_name_hash (track), F_DISCONNECT,
                    F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                  out_track = tracklist_find_track_by_name_hash (
                    TRACKLIST, self->out_tracks[i]);
                  group_target_track_add_child (
                    out_track, track_get_name_hash (track), F_CONNECT,
                    F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                }

              /* reconnect any sends sent from the
               * track */
              for (int j = 0; j < STRIP_SIZE; j++)
                {
                  ChannelSend * clone_send = own_track->channel->sends[j];
                  ChannelSend * send = track->channel->sends[j];
                  channel_send_copy_values (send, clone_send);
                }

              /* reconnect any custom connections */
              GPtrArray * ports = g_ptr_array_new ();
              track_append_ports (own_track, ports, true);
              for (size_t j = 0; j < ports->len; j++)
                {
                  Port * port = (Port *) g_ptr_array_index (ports, j);
                  Port * prj_port = port_find_from_identifier (&port->id);
                  port_restore_from_non_project (prj_port, port);
                }
              object_free_w_func_and_null (g_ptr_array_unref, ports);
            }

          /* re-connect any source sends */
          for (int i = 0; i < self->num_src_sends; i++)
            {
              ChannelSend * clone_send = self->src_sends[i];

              /* get the original send and connect
               * it */
              ChannelSend * prj_send = channel_send_find (clone_send);
              /*Track * orig_track =*/
              /*channel_send_get_track (prj_send);*/

              channel_send_copy_values (prj_send, clone_send);
            }

          /* reset foldable track sizes */
          reset_foldable_track_sizes (self);
        } /* if delete undo */

      EVENTS_PUSH (EventType::ET_TRACKS_ADDED, NULL);
      EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
    }
  /* else if deleting tracks (delete do or create
   * undo) */
  else
    {
      /* if create undo */
      if (create)
        {
          for (int i = self->num_tracks - 1; i >= 0; i--)
            {
              Track * track = TRACKLIST->tracks[self->track_pos + i];
              g_return_val_if_fail (track, -1);

              tracklist_remove_track (
                TRACKLIST, track, F_REMOVE_PL, F_FREE, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }

          /* reenable given track, if any (eg when
           * bouncing) */
          if (self->ival_after > -1)
            {
              g_return_val_if_fail (
                self->ival_after < TRACKLIST->num_tracks, -1);
              Track * tr_to_enable = TRACKLIST->tracks[self->ival_after];
              g_return_val_if_fail (IS_TRACK_AND_NONNULL (tr_to_enable), -1);
              track_set_enabled (
                tr_to_enable, F_ENABLE, F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT,
                F_PUBLISH_EVENTS);
            }
        }
      /* else if delete do */
      else
        {
          /* remove any sends pointing to any track */
          for (int i = 0; i < self->num_src_sends; i++)
            {
              ChannelSend * clone_send = self->src_sends[i];

              /* get the original send and disconnect
               * it */
              ChannelSend * send = channel_send_find (clone_send);
              channel_send_disconnect (send, F_NO_RECALC_GRAPH);
            }

          for (int i = self->tls_before->num_tracks - 1; i >= 0; i--)
            {
              Track * own_track = self->tls_before->tracks[i];

              /* get track from pos */
              Track * track = TRACKLIST->tracks[own_track->pos];
              g_return_val_if_fail (track, -1);

              /* remember any custom connections */
              GPtrArray * ports = g_ptr_array_new ();
              track_append_ports (track, ports, true);
              GPtrArray * clone_ports = g_ptr_array_new ();
              track_append_ports (own_track, clone_ports, true);
              for (size_t j = 0; j < ports->len; j++)
                {
                  Port * prj_port = (Port *) g_ptr_array_index (ports, j);

                  Port * clone_port = NULL;
                  for (size_t k = 0; k < clone_ports->len; k++)
                    {
                      Port * cur_clone_port =
                        (Port *) g_ptr_array_index (clone_ports, k);
                      if (port_identifier_is_equal (
                            &cur_clone_port->id, &prj_port->id))
                        {
                          clone_port = cur_clone_port;
                          break;
                        }
                    }
                  g_return_val_if_fail (clone_port, -1);

                  port_copy_metadata_from_project (clone_port, prj_port);
                }
              object_free_w_func_and_null (g_ptr_array_unref, ports);
              object_free_w_func_and_null (g_ptr_array_unref, clone_ports);

              /* if group track, remove all
               * children */
              if (TRACK_CAN_BE_GROUP_TARGET (track))
                {
                  group_target_track_remove_all_children (
                    track, F_DISCONNECT, F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                }

              /* remove it */
              tracklist_remove_track (
                TRACKLIST, track, F_REMOVE_PL, F_FREE, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }
        }

      EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, NULL);
      EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, NULL);
    }

  /* restore connections */
  save_or_load_port_connections (self, _do);

  tracklist_set_caches (TRACKLIST, CACHE_TYPE_ALL);
  tracklist_validate (TRACKLIST);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  tracklist_validate (TRACKLIST);
  mixer_selections_validate (MIXER_SELECTIONS);

  return 0;
}

/**
 * @param inside Whether moving/copying inside a
 *   foldable track.
 */
static int
do_or_undo_move_or_copy (
  TracklistSelectionsAction * self,
  bool                        _do,
  bool                        copy,
  bool                        inside,
  GError **                   error)
{
  bool move = !copy;
  bool pin =
    self->type == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_PIN;
  bool unpin =
    self->type
    == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_UNPIN;

  /* if moving, this will be set back */
  ZRegion * prev_clip_editor_region = clip_editor_get_region (CLIP_EDITOR);

  if (_do)
    {
      Track * foldable_tr = NULL;
      self->num_fold_change_tracks = 0;
      if (inside)
        {
          foldable_tr = TRACKLIST->tracks[self->track_pos];
          g_return_val_if_fail (track_type_is_foldable (foldable_tr->type), -1);
        }

      Track * prev_track = NULL;
      if (move)
        {
          /* calculate how many tracks are not
           * already in the folder */
          for (int i = 0; i < self->tls_before->num_tracks; i++)
            {
              Track * prj_track =
                track_find_by_name (self->tls_before->tracks[i]->name);
              g_return_val_if_fail (prj_track, -1);
              if (inside)
                {
                  GPtrArray * parents = g_ptr_array_new ();
                  track_add_folder_parents (prj_track, parents, false);
                  if (!g_ptr_array_find (parents, foldable_tr, NULL))
                    self->num_fold_change_tracks++;
                  g_ptr_array_unref (parents);
                }
            }

          for (int i = 0; i < self->tls_before->num_tracks; i++)
            {
              Track * prj_track =
                track_find_by_name (self->tls_before->tracks[i]->name);
              g_return_val_if_fail (prj_track, -1);

              int target_pos = -1;
              /* if not first track to be moved */
              if (prev_track)
                {
                  /* move to last track's
                   * index + 1 */
                  target_pos = prev_track->pos + 1;
                }
              /* else if first track to be moved */
              else
                {
                  /* move to given pos */
                  target_pos = self->track_pos;

                  /* if moving inside, skip folder
                   * track */
                  if (inside)
                    target_pos++;
                }

              /* save index */
              Track * own_track = self->tls_before->tracks[i];
              own_track->pos = prj_track->pos;

              GPtrArray * parents = g_ptr_array_new ();
              track_add_folder_parents (prj_track, parents, false);

              tracklist_move_track (
                TRACKLIST, prj_track, target_pos, true, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
              prev_track = prj_track;

              /* adjust parent sizes */
              for (size_t k = 0; k < parents->len; k++)
                {
                  Track * parent =
                    static_cast<Track *> (g_ptr_array_index (parents, k));

                  /* if new pos is outside
                   * parent */
                  if (
                    prj_track->pos < parent->pos
                    || prj_track->pos >= parent->pos + parent->size)
                    {
                      g_debug (
                        "new pos of %s (%d) is "
                        "outside parent %s: "
                        "parent--",
                        prj_track->name, prj_track->pos, parent->name);
                      parent->size--;
                    }

                  /* if foldable track is child of
                   * parent (size will be readded
                   * later) */
                  if (inside && foldable_track_is_child (parent, foldable_tr))
                    {
                      g_debug (
                        "foldable track %s is "
                        "child of parent %s: "
                        "parent--",
                        foldable_tr->name, parent->name);
                      parent->size--;
                    }
                }
              g_ptr_array_unref (parents);

              if (i == 0)
                tracklist_selections_select_single (
                  TRACKLIST_SELECTIONS, prj_track, F_NO_PUBLISH_EVENTS);
              else
                tracklist_selections_add_track (
                  TRACKLIST_SELECTIONS, prj_track, 0);

              tracklist_print_tracks (TRACKLIST);
            }

          EVENTS_PUSH (EventType::ET_TRACKS_MOVED, NULL);
        }
      else if (copy)
        {
          int num_tracks = self->tls_before->num_tracks;

          if (inside)
            {
              self->num_fold_change_tracks = self->tls_before->num_tracks;
            }

          /* get outputs & sends */
          Track *       outputs[num_tracks];
          ChannelSend * sends[num_tracks][STRIP_SIZE];
          GPtrArray *   send_conns[num_tracks][STRIP_SIZE];
          for (int i = 0; i < num_tracks; i++)
            {
              Track * own_track = self->tls_before->tracks[i];
              if (own_track->channel)
                {
                  outputs[i] = channel_get_output_track (own_track->channel);

                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      ChannelSend * send = own_track->channel->sends[j];
                      sends[i][j] = channel_send_clone (send);
                      send_conns[i][j] = g_ptr_array_new ();

                      channel_send_append_connection (
                        send, self->connections_mgr_before, send_conns[i][j]);
                    }
                }
              else
                {
                  outputs[i] = NULL;
                }
            }

          tracklist_selections_clear (TRACKLIST_SELECTIONS, F_NO_PUBLISH_EVENTS);

          /* create new tracks routed to master */
          Track * new_tracks[num_tracks];
          for (int i = 0; i < num_tracks; i++)
            {
              Track * own_track = self->tls_before->tracks[i];

              /* create a new clone to use in the
               * project */
              GError * err = NULL;
              Track *  track = track_clone (own_track, &err);
              if (!track)
                {
                  PROPAGATE_PREFIXED_ERROR_LITERAL (
                    error, err, _ ("Failed to clone track"));
                  return -1;
                }
              new_tracks[i] = track;

              if (track->channel)
                {
                  /* remove output */
                  track->channel->has_output = false;
                  track->channel->output_name_hash = 0;

                  /* remove sends */
                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      ChannelSend * send = track->channel->sends[j];
                      send->enabled->control = 0.f;
                    }
                }

              /* remove children */
              track->num_children = 0;

              int target_pos = self->track_pos + i;
              if (inside)
                target_pos++;

              /* add to tracklist at given pos */
              tracklist_insert_track (
                TRACKLIST, track, target_pos, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              /* select it */
              track_select (
                track, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
            }

          /* reroute new tracks to correct
           * outputs & sends */
          for (int i = 0; i < num_tracks; i++)
            {
              Track * track = new_tracks[i];
              if (outputs[i])
                {
                  Track * out_track = channel_get_output_track (track->channel);
                  group_target_track_remove_child (
                    out_track, track_get_name_hash (track), F_DISCONNECT,
                    F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                  group_target_track_add_child (
                    outputs[i], track_get_name_hash (track), F_CONNECT,
                    F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
                }

              if (track->channel)
                {
                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      ChannelSend * own_send = sends[i][j];
                      GPtrArray *   own_conns = send_conns[i][j];
                      ChannelSend * track_send = track->channel->sends[j];
                      channel_send_copy_values (track_send, own_send);
                      if (
                        own_conns->len > 0
                        && track->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
                        {
                          PortConnection * conn = static_cast<PortConnection *> (
                            g_ptr_array_index (own_conns, 0));
                          port_connections_manager_ensure_connect (
                            PORT_CONNECTIONS_MGR, &track_send->stereo_out->l->id,
                            conn->dest_id, 1.f, F_LOCKED, F_ENABLE);
                          conn = static_cast<PortConnection *> (
                            g_ptr_array_index (own_conns, 1));
                          port_connections_manager_ensure_connect (
                            PORT_CONNECTIONS_MGR, &track_send->stereo_out->r->id,
                            conn->dest_id, 1.f, F_LOCKED, F_ENABLE);
                        }
                      else if (
                        own_conns->len > 0
                        && track->out_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
                        {
                          PortConnection * conn = static_cast<PortConnection *> (
                            g_ptr_array_index (own_conns, 0));
                          port_connections_manager_ensure_connect (
                            PORT_CONNECTIONS_MGR, &track_send->midi_out->id,
                            conn->dest_id, 1.f, F_LOCKED, F_ENABLE);
                        }

                      channel_send_free (own_send);
                      g_ptr_array_unref (own_conns);
                    }

                } /* endif track has channel */

            } /* endforeach track */

          EVENTS_PUSH (EventType::ET_TRACK_ADDED, NULL);
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, NULL);

        } /* endif copy */

      if (inside)
        {
          /* update foldable track sizes (incl.
           * parents) */
          foldable_track_add_to_size (foldable_tr, self->num_fold_change_tracks);
        }
    }
  /* if undoing */
  else
    {
      if (move)
        {
          for (int i = self->tls_before->num_tracks - 1; i >= 0; i--)
            {
              Track * own_track = self->tls_before->tracks[i];

              Track * prj_track = track_find_by_name (own_track->name);
              g_return_val_if_fail (IS_TRACK_AND_NONNULL (prj_track), -1);

              int target_pos = own_track->pos;
              tracklist_move_track (
                TRACKLIST, prj_track, target_pos, false, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);

              if (i == 0)
                {
                  tracklist_selections_select_single (
                    TRACKLIST_SELECTIONS, prj_track, F_NO_PUBLISH_EVENTS);
                }
              else
                {
                  tracklist_selections_add_track (
                    TRACKLIST_SELECTIONS, prj_track, 0);
                }
            }

          EVENTS_PUSH (EventType::ET_TRACKS_MOVED, NULL);
        }
      else if (copy)
        {
          for (int i = self->tls_before->num_tracks - 1; i >= 0; i--)
            {
              /* get the track from the inserted
               * pos */
              int target_pos = self->track_pos + i;
              if (inside)
                target_pos++;

              Track * track = TRACKLIST->tracks[target_pos];
              g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), -1);

              /* remove it */
              tracklist_remove_track (
                TRACKLIST, track, F_REMOVE_PL, F_FREE, F_NO_PUBLISH_EVENTS,
                F_NO_RECALC_GRAPH);
            }
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
          EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, NULL);
        }

      Track * foldable_tr = NULL;
      if (inside)
        {
          /* update foldable track sizes (incl.
           * parents) */
          foldable_tr = TRACKLIST->tracks[self->track_pos];
          g_return_val_if_fail (track_type_is_foldable (foldable_tr->type), -1);

          foldable_track_add_to_size (
            foldable_tr, -self->num_fold_change_tracks);
        }

      /* reset foldable track sizes */
      reset_foldable_track_sizes (self);
    }

  if ((pin && _do) || (unpin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff += self->tls_before->num_tracks;
    }
  else if ((unpin && _do) || (pin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff -= self->tls_before->num_tracks;
    }

  if (move)
    {
      clip_editor_set_region (
        CLIP_EDITOR, prev_clip_editor_region, F_NO_PUBLISH_EVENTS);
    }

  /* restore connections */
  save_or_load_port_connections (self, _do);

  tracklist_set_caches (TRACKLIST, CACHE_TYPE_ALL);
  tracklist_validate (TRACKLIST);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  return 0;
}

static int
do_or_undo_edit (TracklistSelectionsAction * self, bool _do, GError ** error)
{
  if (_do && self->already_edited)
    {
      self->already_edited = false;
      return 0;
    }

  int * tracks = self->tracks_before;
  int   num_tracks = self->num_tracks;

  bool need_recalc_graph = false;
  bool need_tracklist_cache_update = false;

  for (int i = 0; i < num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[tracks[i]];
      g_return_val_if_fail (track, -1);
      Channel * ch = track->channel;

      switch (self->edit_type)
        {
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO:
          if (track_type_has_channel (track->type))
            {
              int soloed = track_get_soloed (track);
              track_set_soloed (
                track, _do ? self->ival_after : self->ival_before[i],
                F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT, F_NO_PUBLISH_EVENTS);

              self->ival_before[i] = soloed;
            }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO_LANE:
          {
            TrackLane * lane = track->lanes[self->lane_pos];
            int         soloed = track_lane_get_soloed (lane);
            track_lane_set_soloed (
              lane, _do ? self->ival_after : self->ival_before[i],
              F_NO_TRIGGER_UNDO, F_NO_PUBLISH_EVENTS);

            self->ival_before[i] = soloed;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE:
          if (track_type_has_channel (track->type))
            {
              int muted = track_get_muted (track);
              track_set_muted (
                track, _do ? self->ival_after : self->ival_before[i],
                F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT, F_NO_PUBLISH_EVENTS);

              self->ival_before[i] = muted;
            }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE_LANE:
          {
            TrackLane * lane = track->lanes[self->lane_pos];
            int         muted = track_lane_get_muted (lane);
            track_lane_set_muted (
              lane, _do ? self->ival_after : self->ival_before[i],
              F_NO_TRIGGER_UNDO, F_NO_PUBLISH_EVENTS);

            self->ival_before[i] = muted;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_LISTEN:
          if (track_type_has_channel (track->type))
            {
              int listened = track_get_listened (track);
              track_set_listened (
                track, _do ? self->ival_after : self->ival_before[i],
                F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT, F_NO_PUBLISH_EVENTS);

              self->ival_before[i] = listened;
            }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ENABLE:
          {
            int enabled = track_is_enabled (track);
            track_set_enabled (
              track, _do ? self->ival_after : self->ival_before[i],
              F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT, F_NO_PUBLISH_EVENTS);

            self->ival_before[i] = enabled;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_FOLD:
          {
            int folded = track->folded;
            track_set_folded (
              track, _do ? self->ival_after : self->ival_before[i],
              F_NO_TRIGGER_UNDO, F_NO_AUTO_SELECT, F_PUBLISH_EVENTS);

            self->ival_before[i] = folded;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME:
          g_return_val_if_fail (ch, -1);
          fader_set_amp (ch->fader, _do ? self->val_after : self->val_before);
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN:
          g_return_val_if_fail (ch, -1);
          channel_set_balance_control (
            ch, _do ? self->val_after : self->val_before);
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE:
          g_return_val_if_fail (ch, -1);
          if (_do)
            {
              self->ival_before[i] = ENUM_VALUE_TO_INT (ch->fader->midi_mode);
              fader_set_midi_mode (
                ch->fader, ENUM_INT_TO_VALUE (MidiFaderMode, self->ival_after),
                false, F_PUBLISH_EVENTS);
            }
          else
            {
              fader_set_midi_mode (
                ch->fader,
                ENUM_INT_TO_VALUE (MidiFaderMode, self->ival_before[i]), false,
                F_PUBLISH_EVENTS);
            }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
          {
            g_return_val_if_fail (ch, -1);

            int cur_direct_out_pos = -1;
            if (ch->has_output)
              {
                Track * cur_direct_out_track = channel_get_output_track (ch);
                cur_direct_out_pos = cur_direct_out_track->pos;
              }

            /* disconnect from the current track */
            if (ch->has_output)
              {
                Track * target_track = tracklist_find_track_by_name_hash (
                  TRACKLIST, ch->output_name_hash);
                group_target_track_remove_child (
                  target_track, track_get_name_hash (ch->track), F_DISCONNECT,
                  F_NO_RECALC_GRAPH, F_PUBLISH_EVENTS);
              }

            int target_pos = _do ? self->ival_after : self->ival_before[i];

            /* reconnect to the new track */
            if (target_pos != -1)
              {
                z_return_val_if_fail_cmp (target_pos, !=, ch->track->pos, -1);
                group_target_track_add_child (
                  TRACKLIST->tracks[target_pos], track_get_name_hash (ch->track),
                  F_CONNECT, F_NO_RECALC_GRAPH, F_PUBLISH_EVENTS);
              }

            /* remember previous pos */
            self->ival_before[i] = cur_direct_out_pos;

            need_recalc_graph = true;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME:
          {
            char * cur_name = g_strdup (track->name);
            track_set_name (track, self->new_txt, F_NO_PUBLISH_EVENTS);

            /* remember the new name */
            g_free (self->new_txt);
            self->new_txt = cur_name;

            need_tracklist_cache_update = true;
            need_recalc_graph = true;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME_LANE:
          {
            TrackLane * lane = track->lanes[self->lane_pos];
            char *      cur_name = g_strdup (lane->name);
            track_lane_rename (lane, self->new_txt, false);

            /* remember the new name */
            g_free (self->new_txt);
            self->new_txt = cur_name;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COLOR:
          {
            GdkRGBA cur_color = track->color;
            track_set_color (
              track, _do ? &self->new_color : &self->colors_before[i],
              F_NOT_UNDOABLE, F_PUBLISH_EVENTS);

            /* remember color */
            self->colors_before[i] = cur_color;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ICON:
          {
            char * cur_icon = g_strdup (track->icon_name);
            track_set_icon (
              track, self->new_txt, F_NOT_UNDOABLE, F_PUBLISH_EVENTS);

            g_free (self->new_txt);
            self->new_txt = cur_icon;
          }
          break;
        case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COMMENT:
          {
            char * cur_comment = g_strdup (track->comment);
            track_set_comment (track, self->new_txt, F_NOT_UNDOABLE);

            g_free (self->new_txt);
            self->new_txt = cur_comment;
          }
          break;
        }

      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
    }

  /* restore connections */
  save_or_load_port_connections (self, _do);

  if (need_tracklist_cache_update)
    {
      tracklist_set_caches (TRACKLIST, CACHE_TYPE_ALL);
    }

  if (need_recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  return 0;
}

static int
do_or_undo (TracklistSelectionsAction * self, bool _do, GError ** error)
{
  bool ret = -1;
  switch (self->type)
    {
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE:
      ret = do_or_undo_move_or_copy (
        self, _do, true,
        self->type
          == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE,
        error);
      break;
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_CREATE:
      ret = do_or_undo_create_or_delete (self, _do, true, error);
      break;
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_DELETE:
      ret = do_or_undo_create_or_delete (self, _do, false, error);
      break;
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_EDIT:
      ret = do_or_undo_edit (self, _do, error);
      break;
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_PIN:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_UNPIN:
      ret = do_or_undo_move_or_copy (
        self, _do, false,
        self->type
          == TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE,
        error);
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }

  if (ZRYTHM_TESTING)
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];
          z_return_val_if_fail_cmp (
            (size_t) track->num_lanes, <=, track->lanes_size, -1);
        }
    }

  return ret;
}

int
tracklist_selections_action_do (TracklistSelectionsAction * self, GError ** error)
{
  int ret = do_or_undo (self, true, error);
  return ret;
}

int
tracklist_selections_action_undo (
  TracklistSelectionsAction * self,
  GError **                   error)
{
  int ret = do_or_undo (self, false, error);
  return ret;
}

char *
tracklist_selections_action_stringize (TracklistSelectionsAction * self)
{
  switch (self->type)
    {
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE:
      if (self->tls_before->num_tracks == 1)
        {
          if (
            self->type
            == TracklistSelectionsActionType::
              TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE)
            return g_strdup (_ ("Copy Track inside"));
          else
            return g_strdup (_ ("Copy Track"));
        }
      else
        {
          if (
            self->type
            == TracklistSelectionsActionType::
              TRACKLIST_SELECTIONS_ACTION_COPY_INSIDE)
            return g_strdup_printf (
              _ ("Copy %d Tracks inside"), self->tls_before->num_tracks);
          else
            return g_strdup_printf (
              _ ("Copy %d Tracks"), self->tls_before->num_tracks);
        }
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_CREATE:
      {
        const char * type = track_stringize_type (self->track_type);
        if (self->num_tracks == 1)
          {
            return g_strdup_printf (_ ("Create %s Track"), type);
          }
        else
          {
            return g_strdup_printf (
              _ ("Create %d %s Tracks"), self->num_tracks, type);
          }
      }
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_DELETE:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (_ ("Delete Track"));
        }
      else
        {
          return g_strdup_printf (
            _ ("Delete %d Tracks"), self->tls_before->num_tracks);
        }
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_EDIT:
      if (
        (self->tls_before && self->tls_before->num_tracks == 1)
        || (self->num_tracks == 1))
        {
          switch (self->edit_type)
            {
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO:
              if (self->ival_after)
                return g_strdup (_ ("Solo Track"));
              else
                return g_strdup (_ ("Unsolo Track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO_LANE:
              if (self->ival_after)
                return g_strdup (_ ("Solo Lane"));
              else
                return g_strdup (_ ("Unsolo Lane"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE:
              if (self->ival_after)
                return g_strdup (_ ("Mute Track"));
              else
                return g_strdup (_ ("Unmute Track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE_LANE:
              if (self->ival_after)
                return g_strdup (_ ("Mute Lane"));
              else
                return g_strdup (_ ("Unmute Lane"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_LISTEN:
              if (self->ival_after)
                return g_strdup (_ ("Listen Track"));
              else
                return g_strdup (_ ("Unlisten Track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ENABLE:
              if (self->ival_after)
                return g_strdup (_ ("Enable Track"));
              else
                return g_strdup (_ ("Disable Track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_FOLD:
              if (self->ival_after)
                return g_strdup (_ ("Fold Track"));
              else
                return g_strdup (_ ("Unfold Track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME:
              return g_strdup_printf (
                _ ("Change Fader from %.1f to %.1f"), self->val_before,
                self->val_after);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN:
              return g_strdup_printf (
                _ ("Change Pan from %.1f to %.1f"), self->val_before,
                self->val_after);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
              return g_strdup (_ ("Change direct out"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME:
              return g_strdup (_ ("Rename track"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME_LANE:
              return g_strdup (_ ("Rename lane"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COLOR:
              return g_strdup (_ ("Change color"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ICON:
              return g_strdup (_ ("Change icon"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COMMENT:
              return g_strdup (_ ("Change comment"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE:
              return g_strdup (_ ("Change MIDI fader mode"));
            default:
              g_return_val_if_reached (g_strdup (""));
            }
        }
      else
        {
          switch (self->edit_type)
            {
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO:
              if (self->ival_after)
                return g_strdup_printf (_ ("Solo %d Tracks"), self->num_tracks);
              else
                return g_strdup_printf (
                  _ ("Unsolo %d Tracks"), self->num_tracks);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE:
              if (self->ival_after)
                return g_strdup_printf (_ ("Mute %d Tracks"), self->num_tracks);
              else
                return g_strdup_printf (
                  _ ("Unmute %d Tracks"), self->num_tracks);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_LISTEN:
              if (self->ival_after)
                return g_strdup_printf (
                  _ ("Listen %d Tracks"), self->num_tracks);
              else
                return g_strdup_printf (
                  _ ("Unlisten %d Tracks"), self->num_tracks);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ENABLE:
              if (self->ival_after)
                return g_strdup_printf (
                  _ ("Enable %d Tracks"), self->num_tracks);
              else
                return g_strdup_printf (
                  _ ("Disable %d Tracks"), self->num_tracks);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_FOLD:
              if (self->ival_after)
                return g_strdup_printf (_ ("Fold %d Tracks"), self->num_tracks);
              else
                return g_strdup_printf (
                  _ ("Unfold %d Tracks"), self->num_tracks);
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COLOR:
              return g_strdup (_ ("Change color"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MIDI_FADER_MODE:
              return g_strdup (_ ("Change MIDI fader mode"));
            case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
              return g_strdup (_ ("Change direct out"));
            default:
              g_return_val_if_reached (g_strdup (""));
            }
        }
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE:
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE:
      if (self->tls_before->num_tracks == 1)
        {
          if (
            self->type
            == TracklistSelectionsActionType::
              TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE)
            {
              return g_strdup (_ ("Move Track inside"));
            }
          else
            {
              return g_strdup (_ ("Move Track"));
            }
        }
      else
        {
          if (
            self->type
            == TracklistSelectionsActionType::
              TRACKLIST_SELECTIONS_ACTION_MOVE_INSIDE)
            {
              return g_strdup_printf (
                _ ("Move %d Tracks inside"), self->tls_before->num_tracks);
            }
          else
            {
              return g_strdup_printf (
                _ ("Move %d Tracks"), self->tls_before->num_tracks);
            }
        }
      break;
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_PIN:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (_ ("Pin Track"));
        }
      else
        {
          return g_strdup_printf (
            _ ("Pin %d Tracks"), self->tls_before->num_tracks);
        }
    case TracklistSelectionsActionType::TRACKLIST_SELECTIONS_ACTION_UNPIN:
      if (self->tls_before->num_tracks == 1)
        {
          return g_strdup (_ ("Unpin Track"));
        }
      else
        {
          return g_strdup_printf (
            _ ("Unpin %d Tracks"), self->tls_before->num_tracks);
        }
      break;
    }

  g_return_val_if_reached (g_strdup (""));
}

void
tracklist_selections_action_free (TracklistSelectionsAction * self)
{
  object_free_w_func_and_null (tracklist_selections_free, self->tls_before);
  object_free_w_func_and_null (tracklist_selections_free, self->tls_after);
  object_free_w_func_and_null (plugin_setting_free, self->pl_setting);

  for (int i = 0; i < self->num_src_sends; i++)
    {
      object_free_w_func_and_null (channel_send_free, self->src_sends[i]);
    }
  object_zero_and_free (self->src_sends);

  object_zero_and_free (self->out_tracks);
  object_zero_and_free (self->colors_before);
  object_zero_and_free (self->ival_before);

  g_free_and_null (self->base64_midi);
  g_free_and_null (self->file_basename);

  object_free_w_func_and_null (
    port_connections_manager_free, self->connections_mgr_before);
  object_free_w_func_and_null (
    port_connections_manager_free, self->connections_mgr_after);

  object_zero_and_free (self);
}
