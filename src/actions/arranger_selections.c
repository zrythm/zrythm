// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/marker_track.h"
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

typedef enum
{
  Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED,
} ZActionsArrangerSelectionsError;

#define Z_ACTIONS_ARRANGER_SELECTIONS_ERROR \
  z_actions_arranger_selections_error_quark ()
GQuark
z_actions_arranger_selections_error_quark (void);
G_DEFINE_QUARK (
  z - actions - arranger - selections - error - quark,
  z_actions_arranger_selections_error)

static void
move_obj_by_tracks_and_lanes (
  ArrangerObject * obj,
  const int        tracks_diff,
  const int        lanes_diff,
  bool             use_index_in_prev_lane,
  int              index_in_prev_lane);

void
arranger_selections_action_init_loaded (
  ArrangerSelectionsAction * self)
{
#define DO_SELECTIONS(sc) \
  if (self->sc##_sel) \
    { \
      self->sel = (ArrangerSelections *) self->sc##_sel; \
      self->sel_after = \
        (ArrangerSelections *) self->sc##_sel_after; \
      arranger_selections_init_loaded ( \
        self->sel, false, (UndoableAction *) self); \
      if (self->sel_after) \
        { \
          arranger_selections_init_loaded ( \
            self->sel_after, false, (UndoableAction *) self); \
        } \
    }
  DO_SELECTIONS (chord);
  DO_SELECTIONS (tl);
  DO_SELECTIONS (ma);
  DO_SELECTIONS (automation);
  DO_SELECTIONS (audio);

  for (int j = 0; j < self->num_split_objs; j++)
    {
      if (self->region_r1[j])
        {
          arranger_object_init_loaded (
            (ArrangerObject *) self->region_r1[j]);
          arranger_object_init_loaded (
            (ArrangerObject *) self->region_r2[j]);
          self->r1[j] = (ArrangerObject *) self->region_r1[j];
          self->r2[j] = (ArrangerObject *) self->region_r2[j];
        }
      else if (self->mn_r1[j])
        {
          arranger_object_init_loaded (
            (ArrangerObject *) self->mn_r1[j]);
          arranger_object_init_loaded (
            (ArrangerObject *) self->mn_r2[j]);
          self->r1[j] = (ArrangerObject *) self->mn_r1[j];
          self->r2[j] = (ArrangerObject *) self->mn_r2[j];
        }
    }

  if (self->region_before)
    {
      arranger_object_init_loaded (
        (ArrangerObject *) self->region_before);
    }
  if (self->region_after)
    {
      arranger_object_init_loaded (
        (ArrangerObject *) self->region_after);
    }
}

/**
 * Sets the selections used when serializing.
 *
 * @param clone Clone the given selections first.
 * @param is_after If the selections should be set
 *   to \ref ArrangerSelectionsAction.sel_after.
 */
static void
set_selections (
  ArrangerSelectionsAction * self,
  ArrangerSelections *       _sel,
  int                        clone,
  int                        is_after)
{
  ArrangerSelections * sel = _sel;
  if (clone)
    {
      sel = arranger_selections_clone (_sel);
    }

  if (ZRYTHM_TESTING)
    {
      arranger_selections_verify (_sel);
      if (_sel != sel)
        arranger_selections_verify (sel);
    }

  if (is_after)
    {
      self->sel_after = sel;
    }
  else
    {
      self->sel = sel;
    }

#define SET_SEL(cc, sc) \
  if (is_after) \
    self->sc##_sel_after = (cc##Selections *) sel; \
  else \
    self->sc##_sel = (cc##Selections *) sel; \
  break

  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      SET_SEL (Chord, chord);
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      SET_SEL (Timeline, tl);
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      SET_SEL (MidiArranger, ma);
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      SET_SEL (Automation, automation);
    case ARRANGER_SELECTIONS_TYPE_AUDIO:
      SET_SEL (Audio, audio);
    default:
      g_return_if_reached ();
    }

#undef SET_SEL
}

/**
 * Optionally clones the given objects and saves
 * them to self->r1 and self->r2.
 */
NONNULL
static void
set_split_objects (
  ArrangerSelectionsAction * self,
  int                        i,
  ArrangerObject *           r1,
  ArrangerObject *           r2,
  bool                       clone)
{
  if (clone)
    {
      r1 = arranger_object_clone (r1);
      r2 = arranger_object_clone (r2);
    }
  self->r1[i] = r1;
  self->r2[i] = r2;

  switch (r1->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      self->region_r1[i] = (ZRegion *) r1;
      self->region_r2[i] = (ZRegion *) r2;
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      self->mn_r1[i] = (MidiNote *) r1;
      self->mn_r2[i] = (MidiNote *) r2;
      break;
    default:
      g_return_if_reached ();
    }
}

static void
free_split_objects (ArrangerSelectionsAction * self, int i)
{
  arranger_object_free (self->r1[i]);
  arranger_object_free (self->r2[i]);

  self->r1[i] = NULL;
  self->r2[i] = NULL;
  self->mn_r1[i] = NULL;
  self->mn_r2[i] = NULL;
  self->region_r1[i] = NULL;
  self->region_r2[i] = NULL;
}

static ArrangerSelectionsAction *
_create_action (ArrangerSelections * sel)
{
  ArrangerSelectionsAction * self =
    object_new (ArrangerSelectionsAction);

  set_selections (self, sel, true, false);
  self->first_run = true;

  undoable_action_init (
    (UndoableAction *) self, UA_ARRANGER_SELECTIONS);

  return self;
}

static ArrangerSelections *
get_actual_arranger_selections (
  ArrangerSelectionsAction * self)
{
  switch (self->sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      return (ArrangerSelections *) TL_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      return (ArrangerSelections *) MA_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      return (ArrangerSelections *) AUTOMATION_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      return (ArrangerSelections *) CHORD_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_AUDIO:
      return (ArrangerSelections *) AUDIO_SELECTIONS;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Creates a new action for moving or duplicating
 * objects.
 *
 * @param move True to move, false to duplicate.
 * @param already_moved If this is true, the first
 *   DO will do nothing.
 * @param delta_normalized_amount Difference in a
 *   normalized amount, such as automation point
 *   normalized value.
 */
UndoableAction *
arranger_selections_action_new_move_or_duplicate (
  ArrangerSelections * sel,
  const bool           move,
  const double         ticks,
  const int            delta_chords,
  const int            delta_pitch,
  const int            delta_tracks,
  const int            delta_lanes,
  const double         delta_normalized_amount,
  const bool           already_moved,
  GError **            error)
{
  g_return_val_if_fail (
    IS_ARRANGER_SELECTIONS (sel)
      && arranger_selections_has_any (sel),
    NULL);

  /* validate */
  if (!move)
    {
      if (arranger_selections_contains_unclonable_object (sel))
        {
          g_set_error (
            error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
            Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED, "%s",
            _ ("Arranger selections contain an object that "
               "cannot be duplicated"));
          return NULL;
        }
    }

  ArrangerSelectionsAction * self = _create_action (sel);
  UndoableAction *           ua = (UndoableAction *) self;
  if (move)
    {
      self->type = AS_ACTION_MOVE;
    }
  else
    {
      self->type = AS_ACTION_DUPLICATE;
      set_selections (self, sel, true, true);
    }

  if (!already_moved)
    {
      self->first_run = 0;
    }

  self->ticks = ticks;
  self->delta_chords = delta_chords;
  self->delta_lanes = delta_lanes;
  self->delta_tracks = delta_tracks;
  self->delta_pitch = delta_pitch;
  self->delta_normalized_amount = delta_normalized_amount;

  return ua;
}

/**
 * Creates a new action for linking regions.
 *
 * @param already_moved If this is true, the first
 *   DO will do nothing.
 * @param sel_before Original selections.
 * @param sel_after Selections after duplication.
 */
UndoableAction *
arranger_selections_action_new_link (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const double         ticks,
  const int            delta_tracks,
  const int            delta_lanes,
  const bool           already_moved,
  GError **            error)
{
  g_return_val_if_fail (sel_before && sel_after, NULL);

  ArrangerSelectionsAction * self =
    _create_action (sel_before);
  self->type = AS_ACTION_LINK;

  set_selections (self, sel_after, F_CLONE, F_IS_AFTER);

  if (!already_moved)
    self->first_run = false;

  self->ticks = ticks;
  self->delta_tracks = delta_tracks;
  self->delta_lanes = delta_lanes;

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

/**
 * Creates a new action for creating/deleting objects.
 *
 * @param create 1 to create 0 to delete.
 */
UndoableAction *
arranger_selections_action_new_create_or_delete (
  ArrangerSelections * sel,
  const bool           create,
  GError **            error)
{
  g_return_val_if_fail (
    IS_ARRANGER_SELECTIONS (sel)
      && arranger_selections_has_any (sel),
    NULL);

  if (arranger_selections_contains_undeletable_object (sel))
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Arranger selections contain an "
           "undeletable object"));
      return NULL;
    }

  ArrangerSelectionsAction * self = _create_action (sel);
  if (create)
    {
      self->type = AS_ACTION_CREATE;
    }
  else
    {
      self->type = AS_ACTION_DELETE;
    }

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

UndoableAction *
arranger_selections_action_new_record (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const bool           already_recorded,
  GError **            error)
{
  ArrangerSelectionsAction * self =
    _create_action (sel_before);
  self->type = AS_ACTION_RECORD;

  set_selections (self, sel_after, 1, 1);

  if (!already_recorded)
    self->first_run = 0;

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

/**
 * Creates a new action for editing properties
 * of an object.
 *
 * @param sel_before The selections before the
 *   change.
 * @param sel_after The selections after the
 *   change, or NULL if not already edited.
 * @param type Indication of which field has changed.
 */
UndoableAction *
arranger_selections_action_new_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error)
{
  /* validate */
  if (
    type == ARRANGER_SELECTIONS_ACTION_EDIT_NAME
    && arranger_selections_contains_unrenamable_object (
      sel_before))
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Cannot rename selected object(s)"));
      return NULL;
    }

  ArrangerSelectionsAction * self =
    _create_action (sel_before);
  self->type = AS_ACTION_EDIT;

  self->edit_type = type;

  set_selections (self, sel_before, F_CLONE, F_NOT_IS_AFTER);

  if (sel_after)
    {
      set_selections (self, sel_after, F_CLONE, F_IS_AFTER);
    }
  else
    {
      if (already_edited)
        {
          g_critical (
            "sel_after must be passed or already "
            "edited must be false");
          return NULL;
        }

      set_selections (self, sel_before, F_CLONE, F_IS_AFTER);
    }

  if (!already_edited)
    {
      self->first_run = 0;
    }

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

UndoableAction *
arranger_selections_action_new_edit_single_obj (
  const ArrangerObject *           obj_before,
  const ArrangerObject *           obj_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error)
{
  g_return_val_if_fail (obj_before, NULL);
  g_return_val_if_fail (obj_after, NULL);

  /* validate */
  if (
    type == ARRANGER_SELECTIONS_ACTION_EDIT_NAME
    && arranger_object_is_renamable (obj_before))
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Cannot rename selected object(s)"));
      return NULL;
    }

  ArrangerSelections * prj_sel =
    arranger_object_get_selections_for_type (obj_before->type);
  ArrangerSelectionsType sel_type = prj_sel->type;
  ArrangerSelections *   sel_before =
    arranger_selections_new (sel_type);
  ArrangerObject * obj_before_clone =
    arranger_object_clone (obj_before);
  arranger_selections_add_object (
    sel_before, obj_before_clone);
  ArrangerSelections * sel_after =
    arranger_selections_new (sel_type);
  ArrangerObject * obj_after_clone =
    arranger_object_clone (obj_after);
  arranger_selections_add_object (sel_after, obj_after_clone);

  UndoableAction * ua = arranger_selections_action_new_edit (
    sel_before, sel_after, type, already_edited, error);

  arranger_selections_free_full (sel_before);
  arranger_selections_free_full (sel_after);

  return ua;
}

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for MIDI
 * functions.
 */
UndoableAction *
arranger_selections_action_new_edit_midi_function (
  ArrangerSelections * sel_before,
  MidiFunctionType     midi_func_type,
  GError **            error)
{
  ArrangerSelections * sel_after =
    arranger_selections_clone (sel_before);

  GError * err = NULL;
  int      ret =
    midi_function_apply (sel_after, midi_func_type, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to apply MIDI function"));
      arranger_selections_free_full (sel_after);
      return NULL;
    }

  UndoableAction * ua = arranger_selections_action_new_edit (
    sel_before, sel_after,
    ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION,
    F_NOT_ALREADY_EDITED, error);

  arranger_selections_free_full (sel_after);

  return ua;
}

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for
 * automation functions.
 */
UndoableAction *
arranger_selections_action_new_edit_automation_function (
  ArrangerSelections *   sel_before,
  AutomationFunctionType automation_func_type,
  GError **              error)
{
  ArrangerSelections * sel_after =
    arranger_selections_clone (sel_before);

  GError * err = NULL;
  int      ret = automation_function_apply (
         sel_after, automation_func_type, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s",
        _ ("Failed to apply automation function"));
      arranger_selections_free_full (sel_after);
      return NULL;
    }

  UndoableAction * ua = arranger_selections_action_new_edit (
    sel_before, sel_after,
    ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION,
    F_NOT_ALREADY_EDITED, error);

  arranger_selections_free_full (sel_after);

  return ua;
}

/**
 * Wrapper over
 * arranger_selections_action_new_edit() for
 * automation functions.
 */
UndoableAction *
arranger_selections_action_new_edit_audio_function (
  ArrangerSelections * sel_before,
  AudioFunctionType    audio_func_type,
  const char *         uri,
  GError **            error)
{
  /* prepare selections before */
  ArrangerSelections * sel_before_clone =
    arranger_selections_clone (sel_before);

  g_debug ("saving file before applying audio func...");
  GError * err = NULL;
  int      ret = audio_function_apply (
         sel_before_clone, AUDIO_FUNCTION_INVALID, NULL, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s",
        _ ("Failed to apply audio function"));
      arranger_selections_free_full (sel_before_clone);
      return NULL;
    }

  ArrangerSelections * sel_after =
    arranger_selections_clone (sel_before);
  g_debug ("applying actual audio func...");
  ret = audio_function_apply (
    sel_after, audio_func_type, uri, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s",
        _ ("Failed to apply audio function"));
      arranger_selections_free_full (sel_after);
      return NULL;
    }

  UndoableAction * ua = arranger_selections_action_new_edit (
    sel_before_clone, sel_after,
    ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION,
    F_NOT_ALREADY_EDITED, error);

  arranger_selections_free_full (sel_after);
  arranger_selections_free_full (sel_before_clone);

  return ua;
}

/**
 * Creates a new action for automation autofill.
 *
 * @param region_before The region before the
 *   change.
 * @param region_after The region after the
 *   change.
 * @param already_changed Whether the change was
 *   already made.
 */
UndoableAction *
arranger_selections_action_new_automation_fill (
  ZRegion * region_before,
  ZRegion * region_after,
  bool      already_changed,
  GError ** error)
{
  ArrangerSelectionsAction * self =
    object_new (ArrangerSelectionsAction);

  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_ARRANGER_SELECTIONS);

  self->first_run = true;

  self->type = AS_ACTION_AUTOMATION_FILL;

  self->region_before = (ZRegion *) arranger_object_clone (
    (ArrangerObject *) region_before);
  self->region_after = (ZRegion *) arranger_object_clone (
    (ArrangerObject *) region_after);

  if (!already_changed)
    {
      self->first_run = 0;
    }

  return ua;
}

/**
 * Creates a new action for splitting
 * ArrangerObject's.
 *
 * @param pos Global position to split at.
 */
UndoableAction *
arranger_selections_action_new_split (
  ArrangerSelections * sel,
  const Position *     pos,
  GError **            error)
{
  ArrangerSelectionsAction * self = _create_action (sel);
  self->type = AS_ACTION_SPLIT;

  GPtrArray * split_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel, split_objs_arr);
  self->num_split_objs = (int) split_objs_arr->len;
  g_ptr_array_unref (split_objs_arr);

  self->pos = *pos;
  position_update_frames_from_ticks (&self->pos, 0.0);

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

/**
 * Creates a new action for merging
 * ArrangerObject's.
 */
UndoableAction *
arranger_selections_action_new_merge (
  ArrangerSelections * sel,
  GError **            error)
{
  ArrangerSelectionsAction * self = _create_action (sel);
  self->type = AS_ACTION_MERGE;

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

/**
 * Creates a new action for resizing
 * ArrangerObject's.
 *
 * @param ticks How many ticks to add to the resizing
 *   edge.
 */
UndoableAction *
arranger_selections_action_new_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const double                       ticks,
  const bool                         already_resized,
  GError **                          error)
{
  /* validate */
  bool have_unresizable =
    arranger_selections_contains_object_with_property (
      sel, ARRANGER_SELECTIONS_PROPERTY_HAS_LENGTH, false);
  if (have_unresizable)
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED, "%s",
        _ ("Attempted to resize unresizable "
           "objects"));
      return NULL;
    }

  bool have_looped =
    arranger_selections_contains_object_with_property (
      sel, ARRANGER_SELECTIONS_PROPERTY_HAS_LOOPED, true);
  if (
    have_looped
    && (type == ARRANGER_SELECTIONS_ACTION_RESIZE_L || type == ARRANGER_SELECTIONS_ACTION_RESIZE_R || type == ARRANGER_SELECTIONS_ACTION_STRETCH_L || type == ARRANGER_SELECTIONS_ACTION_STRETCH_R))
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED,
        _ ("Cannot perform %s resize - selections "
           "contain looped objects"),
        arranger_selections_action_resize_type_strings[type]
          .str);
      return NULL;
    }

  bool have_unloopable =
    arranger_selections_contains_object_with_property (
      sel, ARRANGER_SELECTIONS_PROPERTY_CAN_LOOP, false);
  if (
    have_unloopable
    && (type == ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP || type == ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP))
    {
      g_set_error (
        error, Z_ACTIONS_ARRANGER_SELECTIONS_ERROR,
        Z_ACTIONS_ARRANGER_SELECTIONS_ERROR_FAILED,
        _ ("Cannot perform %s resize - selections "
           "contain unloopable objects"),
        arranger_selections_action_resize_type_strings[type]
          .str);
      return NULL;
    }

  ArrangerSelectionsAction * self = _create_action (sel);
  self->type = AS_ACTION_RESIZE;

  self->resize_type = type;
  self->ticks = ticks;

  if (!already_resized)
    {
      self->first_run = 0;
    }

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

/**
 * Creates a new action for quantizing
 * ArrangerObject's.
 *
 * @param opts Quantize options.
 */
UndoableAction *
arranger_selections_action_new_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts,
  GError **            error)
{
  ArrangerSelectionsAction * self = _create_action (sel);
  self->type = AS_ACTION_QUANTIZE;

  set_selections (self, sel, 1, 1);
  self->opts = quantize_options_clone (opts);

  UndoableAction * ua = (UndoableAction *) self;
  return ua;
}

ArrangerSelectionsAction *
arranger_selections_action_clone (
  const ArrangerSelectionsAction * src)
{
  ArrangerSelectionsAction * self =
    object_new (ArrangerSelectionsAction);

  self->parent_instance = src->parent_instance;
  self->type = src->type;

  if (src->sel)
    self->sel = arranger_selections_clone (src->sel);
  if (src->sel_after)
    self->sel_after =
      arranger_selections_clone (src->sel_after);

#define SET_SEL(cc, sc, after) \
  if (after) \
    self->sc##_sel_after = \
      (cc##Selections *) self->sel_after; \
  else \
    self->sc##_sel = (cc##Selections *) self->sel; \
  break

  if (self->sel)
    {
      switch (self->sel->type)
        {
        case ARRANGER_SELECTIONS_TYPE_CHORD:
          SET_SEL (Chord, chord, false);
        case ARRANGER_SELECTIONS_TYPE_TIMELINE:
          SET_SEL (Timeline, tl, false);
        case ARRANGER_SELECTIONS_TYPE_MIDI:
          SET_SEL (MidiArranger, ma, false);
        case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
          SET_SEL (Automation, automation, false);
        case ARRANGER_SELECTIONS_TYPE_AUDIO:
          SET_SEL (Audio, audio, false);
        default:
          g_return_val_if_reached (NULL);
        }
    }
  if (self->sel_after)
    {
      switch (self->sel_after->type)
        {
        case ARRANGER_SELECTIONS_TYPE_CHORD:
          SET_SEL (Chord, chord, true);
        case ARRANGER_SELECTIONS_TYPE_TIMELINE:
          SET_SEL (Timeline, tl, true);
        case ARRANGER_SELECTIONS_TYPE_MIDI:
          SET_SEL (MidiArranger, ma, true);
        case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
          SET_SEL (Automation, automation, true);
        case ARRANGER_SELECTIONS_TYPE_AUDIO:
          SET_SEL (Audio, audio, true);
        default:
          g_return_val_if_reached (NULL);
        }
    }

#undef SET_SEL

  self->edit_type = src->edit_type;
  self->resize_type = src->resize_type;
  self->ticks = src->ticks;
  self->delta_tracks = src->delta_tracks;
  self->delta_lanes = src->delta_lanes;
  self->delta_chords = src->delta_chords;
  self->delta_pitch = src->delta_pitch;
  self->delta_vel = src->delta_vel;
  self->delta_normalized_amount = src->delta_normalized_amount;
  self->str = g_strdup (src->str);
  self->pos = src->pos;

  for (int i = 0; i < src->num_split_objs; i++)
    {
      g_return_val_if_fail (src->r1[i], NULL);
      g_return_val_if_fail (src->r2[i], NULL);

      ArrangerObject * r1 = arranger_object_clone (src->r1[i]);
      ArrangerObject * r2 = arranger_object_clone (src->r2[i]);
      self->r1[i] = r1;
      self->r2[i] = r2;
      switch (r1->type)
        {
        case ARRANGER_OBJECT_TYPE_REGION:
          self->region_r1[i] = (ZRegion *) r1;
          self->region_r2[i] = (ZRegion *) r2;
          break;
        case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
          self->mn_r1[i] = (MidiNote *) r1;
          self->mn_r2[i] = (MidiNote *) r2;
          break;
        default:
          g_return_val_if_reached (NULL);
        }
    }
  self->num_split_objs = src->num_split_objs;

  self->first_run = src->first_run;
  if (src->opts)
    self->opts = quantize_options_clone (src->opts);

  return self;
}

bool
arranger_selections_action_perform_create_or_delete (
  ArrangerSelections * sel,
  const bool           create,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_create_or_delete, error,
    sel, create, error);
}

bool
arranger_selections_action_perform_record (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const bool           already_recorded,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_record, error, sel_before,
    sel_after, already_recorded, error);
}

bool
arranger_selections_action_perform_move_or_duplicate (
  ArrangerSelections * sel,
  const bool           move,
  const double         ticks,
  const int            delta_chords,
  const int            delta_pitch,
  const int            delta_tracks,
  const int            delta_lanes,
  const double         delta_normalized_amount,
  const bool           already_moved,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_move_or_duplicate, error,
    sel, move, ticks, delta_chords, delta_pitch, delta_tracks,
    delta_lanes, delta_normalized_amount, already_moved,
    error);
}

bool
arranger_selections_action_perform_link (
  ArrangerSelections * sel_before,
  ArrangerSelections * sel_after,
  const double         ticks,
  const int            delta_tracks,
  const int            delta_lanes,
  const bool           already_moved,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_link, error, sel_before,
    sel_after, ticks, delta_tracks, delta_lanes,
    already_moved, error);
}

bool
arranger_selections_action_perform_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_edit, error, sel_before,
    sel_after, type, already_edited, error);
}

bool
arranger_selections_action_perform_edit_single_obj (
  const ArrangerObject *           obj_before,
  const ArrangerObject *           obj_after,
  ArrangerSelectionsActionEditType type,
  bool                             already_edited,
  GError **                        error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_edit_single_obj, error,
    obj_before, obj_after, type, already_edited, error);
}

bool
arranger_selections_action_perform_edit_midi_function (
  ArrangerSelections * sel_before,
  MidiFunctionType     midi_func_type,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_edit_midi_function, error,
    sel_before, midi_func_type, error);
}

bool
arranger_selections_action_perform_edit_automation_function (
  ArrangerSelections *   sel_before,
  AutomationFunctionType automation_func_type,
  GError **              error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_edit_automation_function,
    error, sel_before, automation_func_type, error);
}

bool
arranger_selections_action_perform_edit_audio_function (
  ArrangerSelections * sel_before,
  AudioFunctionType    audio_func_type,
  const char *         uri,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_edit_audio_function, error,
    sel_before, audio_func_type, uri, error);
}

bool
arranger_selections_action_perform_automation_fill (
  ZRegion * region_before,
  ZRegion * region_after,
  bool      already_changed,
  GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_automation_fill, error,
    region_before, region_after, already_changed, error);
}

bool
arranger_selections_action_perform_split (
  ArrangerSelections * sel,
  const Position *     pos,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_split, error, sel, pos,
    error);
}

bool
arranger_selections_action_perform_merge (
  ArrangerSelections * sel,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_merge, error, sel, error);
}

bool
arranger_selections_action_perform_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const double                       ticks,
  const bool                         already_resized,
  GError **                          error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_resize, error, sel, type,
    ticks, already_resized, error);
}

bool
arranger_selections_action_perform_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts,
  GError **            error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    arranger_selections_action_new_quantize, error, sel, opts,
    error);
}

static void
update_region_link_groups (GPtrArray * objs_arr)
{
  /* handle children of linked regions */
  for (size_t i = 0; i < objs_arr->len; i++)
    {
      /* get the actual object from the
       * project */
      ArrangerObject * obj = arranger_object_find (
        (ArrangerObject *) g_ptr_array_index (objs_arr, i));
      g_return_if_fail (obj);

      if (arranger_object_owned_by_region (obj))
        {
          ZRegion * region = arranger_object_get_region (obj);
          g_return_if_fail (region);

          /* shift all linked objects */
          region_update_link_group (region);
        }
    }
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_move (
  ArrangerSelectionsAction * self,
  const bool                 _do,
  GError **                  error)
{
  arranger_selections_sort_by_indices (self->sel, !_do);
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (self->sel, objs_arr);

  double ticks = _do ? self->ticks : -self->ticks;
  int    delta_tracks =
    _do ? self->delta_tracks : -self->delta_tracks;
  int delta_lanes =
    _do ? self->delta_lanes : -self->delta_lanes;
  int delta_chords =
    _do ? self->delta_chords : -self->delta_chords;
  int delta_pitch =
    _do ? self->delta_pitch : -self->delta_pitch;
  double delta_normalized_amt =
    _do ? self->delta_normalized_amount
        : -self->delta_normalized_amount;

  /* this is used for automation points to
   * keep track of which automation point in the
   * project matches which automation point in
   * the cached selections */
  GHashTable * ht = g_hash_table_new (NULL, NULL);

  if (!self->first_run)
    {
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * own_obj = (ArrangerObject *)
            g_ptr_array_index (objs_arr, i);
          own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;

          /* get the actual object from the
           * project */
          ArrangerObject * obj =
            arranger_object_find (own_obj);
          g_return_val_if_fail (obj, -1);

          g_hash_table_insert (ht, obj, own_obj);

          if (!math_doubles_equal (ticks, 0.0))
            {
              /* shift the actual object */
              arranger_object_move (obj, ticks);

              /* also shift the copy */
              arranger_object_move (own_obj, ticks);
            }

          if (delta_tracks != 0)
            {
              g_return_val_if_fail (
                obj->type == ARRANGER_OBJECT_TYPE_REGION, -1);

              ZRegion * r = (ZRegion *) obj;

              Track * track_to_move_to =
                tracklist_get_visible_track_after_delta (
                  TRACKLIST, arranger_object_get_track (obj),
                  delta_tracks);
              g_warn_if_fail (track_to_move_to);

              /* shift the actual object by
               * tracks */
              region_move_to_track (r, track_to_move_to, -1);

              /* remember info in identifier */
              ZRegion * r_clone = (ZRegion *) own_obj;
              region_identifier_copy (&r_clone->id, &r->id);
            }

          if (delta_pitch != 0)
            {
              g_return_val_if_fail (
                obj->type == ARRANGER_OBJECT_TYPE_MIDI_NOTE,
                -1);

              MidiNote * mn = (MidiNote *) obj;

              /* shift the actual object */
              midi_note_shift_pitch (mn, delta_pitch);

              /* also shift the copy so they can
               * match */
              midi_note_shift_pitch (
                (MidiNote *) own_obj, delta_pitch);
            }

          if (delta_lanes != 0)
            {
              g_return_val_if_fail (
                obj->type == ARRANGER_OBJECT_TYPE_REGION, -1);

              ZRegion * r = (ZRegion *) obj;

              Track * region_track =
                arranger_object_get_track (obj);
              int new_lane_pos = r->id.lane_pos + delta_lanes;
              g_return_val_if_fail (new_lane_pos >= 0, -1);
              track_create_missing_lanes (
                region_track, new_lane_pos);
              TrackLane * lane_to_move_to =
                region_track->lanes[new_lane_pos];

              /* shift the actual object by
               * lanes */
              region_move_to_lane (r, lane_to_move_to, -1);

              /* remember info in identifier */
              ZRegion * r_own = (ZRegion *) own_obj;
              region_identifier_copy (&r_own->id, &r->id);
            }

          if (delta_chords != 0)
            {
              /* TODO */
            }

          if (!math_doubles_equal (delta_normalized_amt, 0.0))
            {
              if (
                obj->type
                == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
                {
                  AutomationPoint * ap =
                    (AutomationPoint *) obj;

                  /* shift the actual object */
                  automation_point_set_fvalue (
                    ap,
                    ap->normalized_val
                      + (float) delta_normalized_amt,
                    F_NORMALIZED, F_NO_PUBLISH_EVENTS);

                  /* also shift the copy so they
                   * can match */
                  AutomationPoint * copy_ap =
                    (AutomationPoint *) own_obj;
                  automation_point_set_fvalue (
                    copy_ap,
                    copy_ap->normalized_val
                      + (float) delta_normalized_amt,
                    F_NORMALIZED, F_NO_PUBLISH_EVENTS);
                }
            }
        }

      /* if moving automation points, re-sort
       * the region and remember the new
       * indices */
      ArrangerObject * first_own_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, 0);
      if (
        first_own_obj->type
        == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
        {
          ArrangerObject * obj =
            /* get the actual object from the
             * project */
            arranger_object_find (first_own_obj);
          g_return_val_if_fail (obj, -1);

          ZRegion * region = arranger_object_get_region (obj);
          g_return_val_if_fail (region, -1);
          automation_region_force_sort (region);

          GHashTableIter iter;
          gpointer       key, value;

          g_hash_table_iter_init (&iter, ht);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              AutomationPoint * prj_ap =
                (AutomationPoint *) key;
              AutomationPoint * cached_ap =
                (AutomationPoint *) value;
              automation_point_set_region_and_index (
                cached_ap, region, prj_ap->index);
            }
        }
    }

  update_region_link_groups (objs_arr);

  g_ptr_array_unref (objs_arr);
  g_hash_table_destroy (ht);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, sel);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_MOVED, sel);

  self->first_run = 0;

  return 0;
}

static void
move_obj_by_tracks_and_lanes (
  ArrangerObject * obj,
  const int        tracks_diff,
  const int        lanes_diff,
  bool             use_index_in_prev_lane,
  int              index_in_prev_lane)
{
  g_return_if_fail (IS_ARRANGER_OBJECT (obj));
  if (tracks_diff)
    {
      g_return_if_fail (
        obj->type == ARRANGER_OBJECT_TYPE_REGION);

      ZRegion * r = (ZRegion *) obj;

      Track * track_to_move_to =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, arranger_object_get_track (obj),
          tracks_diff);

      /* shift the actual object by tracks */
      if (obj->flags & ARRANGER_OBJECT_FLAG_NON_PROJECT)
        {
          r->id.track_name_hash =
            track_get_name_hash (track_to_move_to);
        }
      else
        {
          region_move_to_track (
            r, track_to_move_to,
            use_index_in_prev_lane ? index_in_prev_lane : -1);
        }
    }
  if (lanes_diff)
    {
      ZRegion * r = (ZRegion *) obj;

      Track * region_track = arranger_object_get_track (obj);
      int     new_lane_pos = r->id.lane_pos + lanes_diff;
      g_return_if_fail (new_lane_pos >= 0);

      /* shift the actual object by lanes */
      if (obj->flags & ARRANGER_OBJECT_FLAG_NON_PROJECT)
        {
          r->id.lane_pos = new_lane_pos;
        }
      else
        {
          track_create_missing_lanes (
            region_track, new_lane_pos);
          TrackLane * lane_to_move_to =
            region_track->lanes[new_lane_pos];
          region_move_to_lane (
            r, lane_to_move_to,
            use_index_in_prev_lane ? index_in_prev_lane : -1);
        }
    }
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_duplicate_or_link (
  ArrangerSelectionsAction * self,
  const bool                 link,
  const bool                 _do,
  GError **                  error)
{
  arranger_selections_sort_by_indices (self->sel, !_do);
  arranger_selections_sort_by_indices (self->sel_after, !_do);
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel_after, objs_arr);
  if (ZRYTHM_TESTING)
    {
      arranger_selections_verify (self->sel_after);
    }
  /* objects the duplication/link was based from */
  GPtrArray * orig_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel, orig_objs_arr);
  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  g_return_val_if_fail (sel, -1);

  double ticks = _do ? self->ticks : -self->ticks;
  int    delta_tracks =
    _do ? self->delta_tracks : -self->delta_tracks;
  int delta_lanes =
    _do ? self->delta_lanes : -self->delta_lanes;
  int delta_chords =
    _do ? self->delta_chords : -self->delta_chords;
  int delta_pitch =
    _do ? self->delta_pitch : -self->delta_pitch;
  float delta_normalized_amount =
    _do ? (float) self->delta_normalized_amount
        : (float) -self->delta_normalized_amount;

  /* clear current selections in the project */
  arranger_selections_clear (
    sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);

  /* this is used for automation points to
   * keep track of which automation point in the
   * project matches which automation point in
   * the cached selections */
  GHashTable * ht = g_hash_table_new (NULL, NULL);

  for (int i = (int) objs_arr->len - 1; i >= 0; i--)
    {
      ArrangerObject * own_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      ArrangerObject * own_orig_obj = (ArrangerObject *)
        g_ptr_array_index (orig_objs_arr, i);
      own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;
      own_orig_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;
      g_warn_if_fail (IS_ARRANGER_OBJECT (own_obj));

      /* on first run, we need to first move
       * the original object backwards (the
       * project object too) */
      if (_do && self->first_run)
        {
          if (own_obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              g_debug ("our:");
              region_print ((ZRegion *) own_obj);
            }
          ZRegion * r_orig = NULL;
          ZRegion * r_our = NULL;
          if (own_obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              r_orig = (ZRegion *) own_orig_obj;
              r_our = (ZRegion *) own_obj;
              g_return_val_if_fail (
                IS_REGION_AND_NONNULL (r_orig)
                  && IS_REGION_AND_NONNULL (r_our),
                -1);
            }
          ArrangerObject * obj =
            arranger_object_find (own_obj);
          g_return_val_if_fail (IS_ARRANGER_OBJECT (obj), -1);

          g_debug (
            "[%d] moving original object "
            "backwards",
            i);

          /* ticks */
          if (!math_doubles_equal (self->ticks, 0.0))
            {
              arranger_object_move (obj, -self->ticks);
              arranger_object_move (own_obj, -self->ticks);
            }

          /* tracks & lanes */
          if (delta_tracks || delta_lanes)
            {
              g_return_val_if_fail (r_our, -1);

              g_message ("moving prj obj");
              move_obj_by_tracks_and_lanes (
                obj, -delta_tracks, -delta_lanes, true,
                own_obj->index_in_prev_lane);

              g_message ("moving own obj");
              RegionIdentifier own_id_before_move = r_our->id;
              move_obj_by_tracks_and_lanes (
                own_obj, -delta_tracks, -delta_lanes, true,
                own_obj->index_in_prev_lane);

              /* since the object moved outside
               * of its lane, decrement the index
               * inside the lane for all of our
               * cached objects in the same lane */
              for (int j = i + 1; j < (int) objs_arr->len; j++)
                {
                  ZRegion * own_r = (ZRegion *) own_obj;
                  if (
                    own_id_before_move.track_name_hash
                      == own_r->id.track_name_hash
                    && own_id_before_move.lane_pos
                         == own_r->id.lane_pos
                    && own_id_before_move.at_idx
                         == own_r->id.at_idx)
                    {
                      own_r->id.idx--;
                    }
                }
            }

          /* chords */
          /* TODO */

          /* pitch */
          if (delta_pitch)
            {
              midi_note_shift_pitch (
                (MidiNote *) obj, -delta_pitch);
              midi_note_shift_pitch (
                (MidiNote *) own_obj, -delta_pitch);
            }

          /* automation value */
          if (!math_floats_equal (delta_normalized_amount, 0.f))
            {
              AutomationPoint * ap = (AutomationPoint *) obj;
              AutomationPoint * cached_ap =
                (AutomationPoint *) own_obj;
              automation_point_set_fvalue (
                ap,
                ap->normalized_val - delta_normalized_amount,
                F_NORMALIZED, F_NO_PUBLISH_EVENTS);
              automation_point_set_fvalue (
                cached_ap,
                cached_ap->normalized_val
                  - delta_normalized_amount,
                F_NORMALIZED, F_NO_PUBLISH_EVENTS);
            }
        } /* if do and first run */
    }

  g_debug ("moved original objects back");

  for (size_t i = 0; i < objs_arr->len; i++)
    {
      ArrangerObject * own_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      ArrangerObject * own_orig_obj = (ArrangerObject *)
        g_ptr_array_index (orig_objs_arr, i);

      ArrangerObject * obj;
      if (_do)
        {
          /* clone the clone */
          obj = arranger_object_clone (own_obj);

          /* if region, clear the remembered index
           * so that the region gets appended
           * instead of inserted */
          if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              ZRegion * r = (ZRegion *) obj;
              r->id.idx = -1;
            }

          /* add to track. */
          arranger_object_add_to_project (
            obj, F_NO_PUBLISH_EVENTS);
        }  /* endif do */
      else /* if undo */
        {
          /* find the actual object */
          obj = arranger_object_find (own_obj);
          g_return_val_if_fail (
            IS_ARRANGER_OBJECT_AND_NONNULL (obj), -1);

          /* if the object was created with linking,
           * delete the links */
          if (link && obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              /* remove link from created object
               * (this will also automatically
               * remove the link from the parent
               * region if it is the only region in
               * the link group) */
              ZRegion * region = (ZRegion *) obj;
              g_warn_if_fail (
                IS_REGION (region)
                && region_has_link_group (region));
              region_unlink (region);
              g_warn_if_fail (region->id.link_group == -1);

              /* unlink remembered link groups */
              region = (ZRegion *) own_orig_obj;
              region_unlink (region);
              region = (ZRegion *) own_obj;
              region_unlink (region);
            }

          /* remove it */
          arranger_object_remove_from_project (obj);

        } /* endif undo */

      if (!math_doubles_equal (ticks, 0.0))
        {
          if (_do)
            {
              /* shift it */
              arranger_object_move (obj, ticks);
            }

          /* also shift the copy */
          arranger_object_move (own_obj, ticks);
        }

      if (delta_tracks != 0)
        {
          if (_do)
            {
              move_obj_by_tracks_and_lanes (
                obj, delta_tracks, 0, false, -1);
            }

          /* also shift the copy */
          move_obj_by_tracks_and_lanes (
            own_obj, delta_tracks, false, -1, 0);
        }

      if (delta_pitch != 0)
        {
          if (_do)
            {
              MidiNote * mn = (MidiNote *) obj;

              /* shift the actual object */
              midi_note_shift_pitch (mn, delta_pitch);
            }

          /* also shift the copy so they can
           * match */
          midi_note_shift_pitch (
            (MidiNote *) own_obj, delta_pitch);
        }

      if (delta_lanes != 0)
        {
          if (_do)
            {
              move_obj_by_tracks_and_lanes (
                obj, 0, delta_lanes, false, -1);
            }

          /* also shift the copy */
          move_obj_by_tracks_and_lanes (
            own_obj, 0, delta_lanes, false, -1);
        }

      if (delta_chords != 0)
        {
          /* TODO */
        }

      if (!math_floats_equal (delta_normalized_amount, 0.f))
        {
          /* shift the obj */
          AutomationPoint * ap = (AutomationPoint *) obj;
          automation_point_set_fvalue (
            ap, ap->normalized_val + delta_normalized_amount,
            F_NORMALIZED, F_NO_PUBLISH_EVENTS);

          /* also shift the copy */
          AutomationPoint * cached_ap =
            (AutomationPoint *) own_obj;
          automation_point_set_fvalue (
            cached_ap,
            cached_ap->normalized_val + delta_normalized_amount,
            F_NORMALIZED, F_NO_PUBLISH_EVENTS);
        }

      if (_do)
        {
          /* if we are linking, create the
           * necessary links */
          if (
            link
            && own_orig_obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              /* add link group to original object
               * if necessary */
              ArrangerObject * orig_obj =
                arranger_object_find (own_orig_obj);
              ZRegion * orig_r = (ZRegion *) orig_obj;
              g_return_val_if_fail (orig_r->id.idx >= 0, -1);
              region_create_link_group_if_none (orig_r);
              int link_group = orig_r->id.link_group;

              /* add link group to clone */
              ZRegion * r_obj = (ZRegion *) obj;
              g_return_val_if_fail (
                r_obj->id.type == orig_r->id.type, -1);
              g_return_val_if_fail (r_obj->id.idx >= 0, -1);
              region_set_link_group (r_obj, link_group, true);

              /* remember link groups */
              ZRegion * r = (ZRegion *) own_orig_obj;
              region_set_link_group (r, link_group, true);
              r = (ZRegion *) own_obj;
              region_set_link_group (r, link_group, true);

              region_link_group_manager_validate (
                REGION_LINK_GROUP_MANAGER);
            }
          /* else if we are not linking and this
           * is a region */
          else if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              ZRegion * region = (ZRegion *) obj;

              /* remove link group if first run */
              if (self->first_run)
                {
                  if (region_has_link_group (region))
                    {
                      region_unlink (region);
                    }
                }

              /* if this is an audio region,
               * duplicate the clip */
              if (region->id.type == REGION_TYPE_AUDIO)
                {
                  AudioClip * clip =
                    audio_region_get_clip (region);
                  int id = audio_pool_duplicate_clip (
                    AUDIO_POOL, clip->pool_id, F_WRITE_FILE);
                  clip = audio_pool_get_clip (AUDIO_POOL, id);
                  g_return_val_if_fail (clip, -1);
                  region->pool_id = clip->pool_id;
                }
            }
        }

      /* add the mapping to the hashtable */
      g_hash_table_insert (ht, obj, own_obj);

      if (_do)
        {
          /* select it */
          arranger_object_select (
            obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

          /* remember the identifier */
          arranger_object_copy_identifier (own_obj, obj);
        }

      region_link_group_manager_validate (
        REGION_LINK_GROUP_MANAGER);
    }

  /* if copy-moving automation points, re-sort
   * the region and remember the new indices */
  ArrangerObject * first_own_obj =
    (ArrangerObject *) g_ptr_array_index (objs_arr, 0);
  if (first_own_obj->type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
    {
      /* get the actual object from the
       * project */
      ArrangerObject * obj =
        g_hash_table_get_keys_as_array (ht, NULL)[0];
      g_return_val_if_fail (IS_ARRANGER_OBJECT (obj), -1);
      ZRegion * region = arranger_object_get_region (obj);
      g_return_val_if_fail (region, -1);
      automation_region_force_sort (region);

      GHashTableIter iter;
      gpointer       key, value;

      g_hash_table_iter_init (&iter, ht);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          AutomationPoint * prj_ap = (AutomationPoint *) key;
          AutomationPoint * cached_ap =
            (AutomationPoint *) value;
          automation_point_set_region_and_index (
            cached_ap, region, prj_ap->index);
        }
    }

  marker_track_validate (P_MARKER_TRACK);
  chord_track_validate (P_CHORD_TRACK);
  region_link_group_manager_validate (
    REGION_LINK_GROUP_MANAGER);

  g_ptr_array_unref (objs_arr);
  g_ptr_array_unref (orig_objs_arr);
  g_hash_table_destroy (ht);

  sel = get_actual_arranger_selections (self);
  if (_do)
    {
      transport_recalculate_total_bars (
        TRANSPORT, self->sel_after);

      EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  else
    {
      EVENTS_PUSH (ET_ARRANGER_SELECTIONS_REMOVED, sel);
    }

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_create_or_delete (
  ArrangerSelectionsAction * self,
  const int                  _do,
  const int                  create,
  GError **                  error)
{
  if (create)
    {
      arranger_selections_sort_by_indices (
        self->sel, _do ? F_NOT_DESCENDING : F_DESCENDING);
    }
  else
    {
      arranger_selections_sort_by_indices (
        self->sel, _do ? F_DESCENDING : F_NOT_DESCENDING);
    }
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (self->sel, objs_arr);
  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  g_return_val_if_fail (sel, -1);

  if (!self->first_run || !create)
    {
      /* clear current selections in the project */
      arranger_selections_clear (
        sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);

      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * own_obj = (ArrangerObject *)
            g_ptr_array_index (objs_arr, i);
          own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;

          /* if doing in a create action or undoing
           * in a delete action */
          if ((_do && create) || (!_do && !create))
            {
              /* clone the clone */
              ArrangerObject * obj =
                arranger_object_clone (own_obj);

              /* add it to the project */
              if (create)
                {
                  arranger_object_add_to_project (
                    obj, F_NO_PUBLISH_EVENTS);
                }
              else
                {
                  arranger_object_insert_to_project (obj);
                }

              /* select it */
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

              /* remember new info */
              arranger_object_copy_identifier (own_obj, obj);
            }

          /* if removing */
          else
            {
              /* get the actual object from the
               * project */
              ArrangerObject * obj =
                arranger_object_find (own_obj);
              g_return_val_if_fail (obj, -1);

              /* if region, remove link */
              if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
                {
                  ZRegion * region = (ZRegion *) obj;
                  if (region_has_link_group (region))
                    {
                      region_unlink (region);

                      /* unlink remembered link
                       * groups */
                      region = (ZRegion *) own_obj;
                      region_unlink (region);
                    }
                }

              /* remove it */
              arranger_object_remove_from_project (obj);
            }
        }
    }

  /* if first time creating the object, save the
   * length for use by SnapGrid */
  if (
    ZRYTHM_HAVE_UI && self->first_run && create && _do
    && objs_arr->len == 1)
    {
      ArrangerObject * obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, 0);
      obj = arranger_object_find (obj);
      g_return_val_if_fail (obj, -1);
      if (arranger_object_type_has_length (obj->type))
        {
          double ticks =
            arranger_object_get_length_in_ticks (obj);
          ArrangerWidget * arranger =
            arranger_object_get_arranger (obj);
          if (arranger->type == ARRANGER_WIDGET_TYPE_TIMELINE)
            {
              g_settings_set_double (
                S_UI, "timeline-last-object-length", ticks);
            }
          else
            {
              g_settings_set_double (
                S_UI, "editor-last-object-length", ticks);
            }
        }
    }

  /* if creating */
  if ((_do && create) || (!_do && !create))
    {
      update_region_link_groups (objs_arr);

      transport_recalculate_total_bars (TRANSPORT, self->sel);

      EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  /* if deleting */
  else
    {
      EVENTS_PUSH (ET_ARRANGER_SELECTIONS_REMOVED, sel);
    }

  marker_track_validate (P_MARKER_TRACK);
  chord_track_validate (P_CHORD_TRACK);
  region_link_group_manager_validate (
    REGION_LINK_GROUP_MANAGER);

  g_ptr_array_unref (objs_arr);

  self->first_run = 0;

  if (self->sel->type == ARRANGER_SELECTIONS_TYPE_TIMELINE)
    {
      TimelineSelections * ts =
        (TimelineSelections *) self->sel;
      bool have_automation_region = false;
      for (int i = 0; i < ts->num_regions; i++)
        {
          ZRegion * r = ts->regions[i];
          if (r->id.type == REGION_TYPE_AUTOMATION)
            {
              have_automation_region = true;
              break;
            }
        }

      if (have_automation_region)
        {
          router_recalc_graph (ROUTER, F_NOT_SOFT);
        }
    }

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_record (
  ArrangerSelectionsAction * self,
  const bool                 _do,
  GError **                  error)
{
  arranger_selections_sort_by_indices (
    self->sel, _do ? false : true);
  arranger_selections_sort_by_indices (
    self->sel_after, _do ? false : true);
  GPtrArray * before_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel, before_objs_arr);
  GPtrArray * after_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel_after, after_objs_arr);
  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  g_return_val_if_fail (sel, -1);

  if (!self->first_run)
    {
      /* clear current selections in the project */
      arranger_selections_clear (
        sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);

      /* if do/redoing */
      if (_do)
        {
          /* create the newly recorded objects */
          for (size_t i = 0; i < after_objs_arr->len; i++)
            {
              ArrangerObject * own_after_obj =
                (ArrangerObject *) g_ptr_array_index (
                  after_objs_arr, i);
              own_after_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* clone the clone */
              ArrangerObject * obj =
                arranger_object_clone (own_after_obj);

              /* add it to the project */
              arranger_object_add_to_project (
                obj, F_NO_PUBLISH_EVENTS);

              /* select it */
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

              /* remember new info */
              arranger_object_copy_identifier (
                own_after_obj, obj);
            }

          /* delete the previous objects */
          for (size_t i = 0; i < before_objs_arr->len; i++)
            {
              ArrangerObject * own_before_obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);
              own_before_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get the actual object from the
               * project */
              ArrangerObject * obj =
                arranger_object_find (own_before_obj);
              g_return_val_if_fail (obj, -1);

              /* remove it */
              arranger_object_remove_from_project (obj);
            }
        }

      /* if undoing */
      else
        {
          /* delete the newly recorded objects */
          for (size_t i = 0; i < after_objs_arr->len; i++)
            {
              ArrangerObject * own_after_obj =
                (ArrangerObject *) g_ptr_array_index (
                  after_objs_arr, i);
              own_after_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get the actual object from the
               * project */
              ArrangerObject * obj =
                arranger_object_find (own_after_obj);
              g_return_val_if_fail (obj, -1);

              /* remove it */
              arranger_object_remove_from_project (obj);
            }

          /* add the objects before the recording */
          for (size_t i = 0; i < before_objs_arr->len; i++)
            {
              ArrangerObject * own_before_obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);
              own_before_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* clone the clone */
              ArrangerObject * obj =
                arranger_object_clone (own_before_obj);

              /* add it to the project */
              arranger_object_add_to_project (
                obj, F_NO_PUBLISH_EVENTS);

              /* select it */
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

              /* remember new info */
              arranger_object_copy_identifier (
                own_before_obj, obj);
            }
        }
    }

  g_ptr_array_unref (before_objs_arr);
  g_ptr_array_unref (after_objs_arr);

  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CREATED, sel);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_REMOVED, sel);

  self->first_run = 0;

  if (self->sel->type == ARRANGER_SELECTIONS_TYPE_TIMELINE)
    {
      TimelineSelections * ts =
        (TimelineSelections *) self->sel;
      bool have_automation_region = false;
      for (int i = 0; i < ts->num_regions; i++)
        {
          ZRegion * r = ts->regions[i];
          if (r->id.type == REGION_TYPE_AUTOMATION)
            {
              have_automation_region = true;
              break;
            }
        }

      if (have_automation_region)
        {
          router_recalc_graph (ROUTER, F_NOT_SOFT);
        }
    }

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_edit (
  ArrangerSelectionsAction * self,
  const int                  _do,
  GError **                  error)
{
  GPtrArray * objs_before_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel, objs_before_arr);
  GPtrArray * objs_after_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel_after, objs_after_arr);

  GPtrArray * src_objs_arr =
    _do ? objs_before_arr : objs_after_arr;
  GPtrArray * dest_objs_arr =
    _do ? objs_after_arr : objs_before_arr;

  if (!self->first_run)
    {
      if (
        self->edit_type
          == ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION
        && self->sel->type == ARRANGER_SELECTIONS_TYPE_AUDIO)
        {
          AudioSelections * src_audio_sel =
            (AudioSelections *) (_do ? self->sel_after : self->sel);
          ZRegion * r =
            region_find (&src_audio_sel->region_id);
          AudioClip * src_clip = audio_pool_get_clip (
            AUDIO_POOL, src_audio_sel->pool_id);

          /* adjust the positions */
          Position start, end;
          position_set_to_pos (
            &start, &src_audio_sel->sel_start);
          position_set_to_pos (&end, &src_audio_sel->sel_end);
          position_add_frames (&start, -r->base.pos.frames);
          position_add_frames (&end, -r->base.pos.frames);
          unsigned_frame_t num_frames =
            (unsigned_frame_t) (end.frames - start.frames);
          g_return_val_if_fail (
            num_frames == src_clip->num_frames, -1);

          char * src_clip_path = audio_clip_get_path_in_pool (
            src_clip, F_NOT_BACKUP);
          g_message (
            "replacing audio region %s frames with "
            "'%s' frames",
            r->name, src_clip_path);
          g_free (src_clip_path);

          /* replace the frames in the region */
          audio_region_replace_frames (
            r, src_clip->frames, (size_t) start.frames,
            num_frames, F_NO_DUPLICATE_CLIP);
        }
      else /* not audio function */
        {
          for (size_t i = 0; i < src_objs_arr->len; i++)
            {
              ArrangerObject * own_src_obj = (ArrangerObject *)
                g_ptr_array_index (src_objs_arr, i);
              ArrangerObject * own_dest_obj = (ArrangerObject *)
                g_ptr_array_index (dest_objs_arr, i);
              g_return_val_if_fail (own_src_obj, -1);
              g_return_val_if_fail (own_dest_obj, -1);
              own_src_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;
              own_dest_obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* find the actual object */
              ArrangerObject * obj =
                arranger_object_find (own_src_obj);
              g_return_val_if_fail (obj, -1);

              /* change the parameter */
              switch (self->edit_type)
                {
                case ARRANGER_SELECTIONS_ACTION_EDIT_NAME:
                  {
                    g_return_val_if_fail (
                      arranger_object_type_has_name (obj->type),
                      -1);
                    const char * name =
                      arranger_object_get_name (own_dest_obj);
                    arranger_object_set_name (
                      obj, name, F_NO_PUBLISH_EVENTS);
                  }
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_POS:
                  obj->pos = own_dest_obj->pos;
                  obj->end_pos = own_dest_obj->end_pos;
                  obj->clip_start_pos =
                    own_dest_obj->clip_start_pos;
                  obj->loop_start_pos =
                    own_dest_obj->loop_start_pos;
                  obj->loop_end_pos =
                    own_dest_obj->loop_end_pos;
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_FADES:
                  obj->fade_in_pos = own_dest_obj->fade_in_pos;
                  obj->fade_out_pos =
                    own_dest_obj->fade_out_pos;
                  obj->fade_in_opts =
                    own_dest_obj->fade_in_opts;
                  obj->fade_out_opts =
                    own_dest_obj->fade_out_opts;
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE:
#define SET_PRIMITIVE(cc, member) \
  ((cc *) obj)->member = ((cc *) own_dest_obj)->member

                  switch (obj->type)
                    {
                    case ARRANGER_OBJECT_TYPE_REGION:
                      {
                        SET_PRIMITIVE (ArrangerObject, muted);
                        SET_PRIMITIVE (ZRegion, color);
                        SET_PRIMITIVE (ZRegion, use_color);
                        SET_PRIMITIVE (ZRegion, musical_mode);
                        SET_PRIMITIVE (ZRegion, gain);
                      }
                      break;
                    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
                      {
                        SET_PRIMITIVE (MidiNote, muted);
                        SET_PRIMITIVE (MidiNote, val);

                        /* set velocity and cache vel */
                        MidiNote * mn = (MidiNote *) obj;
                        MidiNote * dest_mn =
                          (MidiNote *) own_dest_obj;
                        velocity_set_val (
                          mn->vel, dest_mn->vel->vel);
                      }
                      break;
                    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
                      {
                        SET_PRIMITIVE (
                          AutomationPoint, curve_opts);
                        SET_PRIMITIVE (
                          AutomationPoint, fvalue);
                        SET_PRIMITIVE (
                          AutomationPoint, normalized_val);
                      }
                      break;
                    default:
                      break;
                    }
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_EDITOR_FUNCTION:
                  switch (obj->type)
                    {
                    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
                      {
                        SET_PRIMITIVE (MidiNote, muted);

                        /* set velocity and cache vel */
                        MidiNote * mn = (MidiNote *) obj;
                        MidiNote * dest_mn =
                          (MidiNote *) own_dest_obj;
                        velocity_set_val (
                          mn->vel, dest_mn->vel->vel);
                      }
                      break;
                    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
                      {
                        SET_PRIMITIVE (
                          AutomationPoint, curve_opts);
                        SET_PRIMITIVE (
                          AutomationPoint, fvalue);
                        SET_PRIMITIVE (
                          AutomationPoint, normalized_val);
                      }
                      break;
                    default:
                      break;
                    }
#undef SET_PRIMITIVE
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_SCALE:
                  {
                    ScaleObject * scale = (ScaleObject *) obj;
                    ScaleObject * dest_scale =
                      (ScaleObject *) own_dest_obj;

                    /* set the new scale */
                    MusicalScale * old = scale->scale;
                    scale->scale =
                      musical_scale_clone (dest_scale->scale);
                    object_free_w_func_and_null (
                      musical_scale_free, old);
                  }
                  break;
                case ARRANGER_SELECTIONS_ACTION_EDIT_MUTE:
                  {
                    /* set the new status */
                    arranger_object_set_muted (
                      obj, !obj->muted, false);
                  }
                  break;
                default:
                  break;
                }
            }
        } /* endif audio function */
    }     /* endif not first run */

  update_region_link_groups (dest_objs_arr);

  g_ptr_array_unref (src_objs_arr);
  g_ptr_array_unref (dest_objs_arr);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING, sel);

  self->first_run = false;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_automation_fill (
  ArrangerSelectionsAction * self,
  const int                  _do,
  GError **                  error)
{
  if (!self->first_run)
    {
      /* clear current selections in the project */
      arranger_selections_clear (
        (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE,
        F_NO_PUBLISH_EVENTS);

      /* get the actual object from the
       * project */
      ArrangerObject * obj = arranger_object_find (
        _do ? (ArrangerObject *) self->region_before
            : (ArrangerObject *) self->region_after);
      g_return_val_if_fail (obj, -1);

      /* remove link */
      ZRegion * region = (ZRegion *) obj;
      if (region_has_link_group (region))
        {
          region_unlink (region);

          /* unlink remembered link
           * groups */
          region =
            _do ? self->region_before : self->region_after;
          region_unlink (region);
        }

      /* remove it */
      arranger_object_remove_from_project (obj);

      /* clone the clone */
      obj = arranger_object_clone (
        _do ? (ArrangerObject *) self->region_after
            : (ArrangerObject *) self->region_before);

      /* add it to the project */
      arranger_object_add_to_project (
        obj, F_NO_PUBLISH_EVENTS);

      /* select it */
      arranger_object_select (
        obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

      /* remember new info */
      arranger_object_copy_identifier (
        _do ? (ArrangerObject *) self->region_after
            : (ArrangerObject *) self->region_before,
        obj);
    }

  self->first_run = false;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_split (
  ArrangerSelectionsAction * self,
  const int                  _do,
  GError **                  error)
{
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (self->sel, objs_arr);

  for (size_t i = 0; i < objs_arr->len; i++)
    {
      ArrangerObject * own_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;

      ArrangerObject *obj, *r1, *r2;
      if (_do)
        {
          /* find the actual object */
          obj = arranger_object_find (own_obj);
          g_return_val_if_fail (obj, -1);

          /* split */
          arranger_object_split (
            obj, &self->pos, 0, &self->r1[i], &self->r2[i],
            true);

          /* r1 and r2 are now inside the project,
           * clone them to keep copies */
          g_return_val_if_fail (self->r1[i], -1);
          g_return_val_if_fail (self->r2[i], -1);
          set_split_objects (
            self, (int) i, self->r1[i], self->r2[i], true);
        }
      /* else if undoing split */
      else
        {
          /* find the actual objects */
          r1 = arranger_object_find (self->r1[i]);
          r2 = arranger_object_find (self->r2[i]);
          g_return_val_if_fail (r1 && r2, -1);

          /* unsplit */
          arranger_object_unsplit (
            r1, r2, &obj, F_NO_PUBLISH_EVENTS);
          if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
            {
              ZRegion * own_region = (ZRegion *) own_obj;
              arranger_object_set_name (
                obj, own_region->name, F_NO_PUBLISH_EVENTS);
            }

          /* re-insert object at its original
           * position */
          arranger_object_remove_from_project (obj);
          obj = arranger_object_clone (own_obj);
          arranger_object_insert_to_project (obj);

          /* free the copies created in _do */
          free_split_objects (self, (int) i);
        }
    }

  if (_do)
    self->num_split_objs = (int) objs_arr->len;
  else
    self->num_split_objs = 0;

  g_ptr_array_unref (objs_arr);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_merge (
  ArrangerSelectionsAction * self,
  const bool                 _do,
  GError **                  error)
{
  /* if first run, merge */
  if (self->first_run)
    {
      self->sel_after = arranger_selections_clone (self->sel);
      arranger_selections_merge (self->sel_after);
    }

  arranger_selections_sort_by_indices (self->sel, !_do);
  arranger_selections_sort_by_indices (self->sel_after, !_do);

  GPtrArray * before_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    _do ? self->sel : self->sel_after, before_objs_arr);
  GPtrArray * after_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    _do ? self->sel_after : self->sel, after_objs_arr);

  /* remove the before objects from the project */
  for (int i = (int) before_objs_arr->len - 1; i >= 0; i--)
    {
      ArrangerObject * own_before_obj = (ArrangerObject *)
        g_ptr_array_index (before_objs_arr, i);

      own_before_obj->flags |=
        ARRANGER_OBJECT_FLAG_NON_PROJECT;

      /* find the actual object */
      ArrangerObject * prj_obj =
        arranger_object_find (own_before_obj);
      g_return_val_if_fail (prj_obj, -1);

      /* remove */
      arranger_object_remove_from_project (prj_obj);
    }

  /* add the after objects to the project */
  for (int i = (int) after_objs_arr->len - 1; i >= 0; i--)
    {
      ArrangerObject * own_after_obj = (ArrangerObject *)
        g_ptr_array_index (after_objs_arr, i);
      own_after_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;

      ArrangerObject * clone_obj =
        arranger_object_clone (own_after_obj);

      arranger_object_add_to_project (
        clone_obj, F_NO_PUBLISH_EVENTS);

      /* remember positions */
      arranger_object_copy_identifier (
        own_after_obj, clone_obj);
    }

  g_ptr_array_unref (before_objs_arr);
  g_ptr_array_unref (after_objs_arr);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_resize (
  ArrangerSelectionsAction * self,
  const int                  _do,
  GError **                  error)
{
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (self->sel, objs_arr);

  double ticks = _do ? self->ticks : -self->ticks;

  if (!self->first_run)
    {
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * own_obj = (ArrangerObject *)
            g_ptr_array_index (objs_arr, i);
          own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;

          /* find the actual object */
          ArrangerObject * obj =
            arranger_object_find (own_obj);
          g_return_val_if_fail (obj, -1);

          /* resize */
          ArrangerObjectResizeType type = -1;
          int                      left = 0;
          switch (self->resize_type)
            {
            case ARRANGER_SELECTIONS_ACTION_RESIZE_L:
              type = ARRANGER_OBJECT_RESIZE_NORMAL;
              left = true;
              break;
            case ARRANGER_SELECTIONS_ACTION_STRETCH_L:
              type = ARRANGER_OBJECT_RESIZE_STRETCH;
              left = true;
              break;
            case ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP:
              left = true;
              type = ARRANGER_OBJECT_RESIZE_LOOP;
              break;
            case ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE:
              left = true;
              type = ARRANGER_OBJECT_RESIZE_FADE;
              break;
            case ARRANGER_SELECTIONS_ACTION_RESIZE_R:
              type = ARRANGER_OBJECT_RESIZE_NORMAL;
              break;
            case ARRANGER_SELECTIONS_ACTION_STRETCH_R:
              type = ARRANGER_OBJECT_RESIZE_STRETCH;
              break;
            case ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP:
              type = ARRANGER_OBJECT_RESIZE_LOOP;
              break;
            case ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE:
              type = ARRANGER_OBJECT_RESIZE_FADE;
              break;
            default:
              g_warn_if_reached ();
            }
          arranger_object_resize (
            obj, left, type, ticks, false);

          /* also resize the clone so we can find
           * the actual object next time */
          arranger_object_resize (
            own_obj, left, type, ticks, false);
        }
    }

  update_region_link_groups (objs_arr);

  g_ptr_array_unref (objs_arr);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, sel);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_ACTION_FINISHED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_quantize (
  ArrangerSelectionsAction * self,
  const int                  _do,
  GError **                  error)
{
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (self->sel, objs_arr);
  GPtrArray * quantized_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    self->sel_after, quantized_objs_arr);

  for (size_t i = 0; i < objs_arr->len; i++)
    {
      ArrangerObject * own_obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      ArrangerObject * own_quantized_obj = (ArrangerObject *)
        g_ptr_array_index (quantized_objs_arr, i);

      own_obj->flags |= ARRANGER_OBJECT_FLAG_NON_PROJECT;
      own_quantized_obj->flags |=
        ARRANGER_OBJECT_FLAG_NON_PROJECT;

      /* find the actual object */
      ArrangerObject * obj;
      if (_do)
        {
          obj = arranger_object_find (own_obj);
        }
      else
        {
          obj = arranger_object_find (own_quantized_obj);
        }
      g_return_val_if_fail (obj, -1);

      if (_do)
        {
          /* quantize it */
          if (self->opts->adj_start)
            {
              double ticks =
                quantize_options_quantize_position (
                  self->opts, &obj->pos);
              position_add_ticks (&obj->end_pos, ticks);
            }
          if (self->opts->adj_end)
            {
              quantize_options_quantize_position (
                self->opts, &obj->end_pos);
            }
          arranger_object_pos_setter (obj, &obj->pos);
          arranger_object_end_pos_setter (obj, &obj->end_pos);

          /* remember the quantized position so we
           * can find the object when undoing */
          position_set_to_pos (
            &own_quantized_obj->pos, &obj->pos);
          position_set_to_pos (
            &own_quantized_obj->end_pos, &obj->end_pos);
        }
      else
        {
          /* unquantize it */
          arranger_object_pos_setter (obj, &own_obj->pos);
          arranger_object_end_pos_setter (
            obj, &own_obj->end_pos);
        }
    }
  g_ptr_array_unref (objs_arr);
  g_ptr_array_unref (quantized_objs_arr);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_QUANTIZED, sel);

  self->first_run = 0;

  return 0;
}

static int
do_or_undo (
  ArrangerSelectionsAction * self,
  bool                       _do,
  GError **                  error)
{
  int ret = -1;
  switch (self->type)
    {
    case AS_ACTION_CREATE:
      ret =
        do_or_undo_create_or_delete (self, _do, true, error);
      break;
    case AS_ACTION_DELETE:
      ret =
        do_or_undo_create_or_delete (self, _do, false, error);
      break;
    case AS_ACTION_DUPLICATE:
      ret = do_or_undo_duplicate_or_link (
        self, false, _do, error);
      break;
    case AS_ACTION_MOVE:
      ret = do_or_undo_move (self, _do, error);
      break;
    case AS_ACTION_LINK:
      ret =
        do_or_undo_duplicate_or_link (self, true, _do, error);
      break;
    case AS_ACTION_RECORD:
      ret = do_or_undo_record (self, _do, error);
      break;
    case AS_ACTION_EDIT:
      ret = do_or_undo_edit (self, _do, error);
      break;
    case AS_ACTION_AUTOMATION_FILL:
      ret = do_or_undo_automation_fill (self, _do, error);
      break;
    case AS_ACTION_SPLIT:
      ret = do_or_undo_split (self, _do, error);
      break;
    case AS_ACTION_MERGE:
      ret = do_or_undo_merge (self, _do, error);
      break;
    case AS_ACTION_RESIZE:
      ret = do_or_undo_resize (self, _do, error);
      break;
    case AS_ACTION_QUANTIZE:
      ret = do_or_undo_quantize (self, _do, error);
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }

  /* update playback caches */
  tracklist_set_caches (
    TRACKLIST, CACHE_TYPE_PLAYBACK_SNAPSHOTS);

  return ret;
}

int
arranger_selections_action_do (
  ArrangerSelectionsAction * self,
  GError **                  error)
{
  return do_or_undo (self, true, error);
}

int
arranger_selections_action_undo (
  ArrangerSelectionsAction * self,
  GError **                  error)
{
  return do_or_undo (self, false, error);
}

bool
arranger_selections_action_contains_clip (
  ArrangerSelectionsAction * self,
  AudioClip *                clip)
{
  if (
    self->sel
    && arranger_selections_contains_clip (self->sel, clip))
    {
      return true;
    }
  if (
    self->sel_after
    && arranger_selections_contains_clip (
      self->sel_after, clip))
    {
      return true;
    }

  /* check split regions (if any) */
  for (int i = 0; i < self->num_split_objs; i++)
    {
      ZRegion * r1 = self->region_r1[i];
      ZRegion * r2 = self->region_r2[i];
      if (r1 && r2)
        {
          if (r1->id.type != REGION_TYPE_AUDIO)
            break;

          if (
            r1->pool_id == clip->pool_id
            || r2->pool_id == clip->pool_id)
            {
              return true;
            }
        }
      else
        break;
    }

  return false;
}

char *
arranger_selections_action_stringize (
  ArrangerSelectionsAction * self)
{
  switch (self->type)
    {
    case AS_ACTION_CREATE:
      switch (self->sel->type)
        {
        case ARRANGER_SELECTIONS_TYPE_TIMELINE:
          return g_strdup (_ ("Create timeline selections"));
        case ARRANGER_SELECTIONS_TYPE_AUDIO:
          return g_strdup (_ ("Create audio selections"));
        case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
          return g_strdup (_ ("Create automation selections"));
        case ARRANGER_SELECTIONS_TYPE_CHORD:
          return g_strdup (_ ("Create chord selections"));
        case ARRANGER_SELECTIONS_TYPE_MIDI:
          return g_strdup (_ ("Create MIDI selections"));
        default:
          g_return_val_if_reached (NULL);
        }
    case AS_ACTION_DELETE:
      return g_strdup (_ ("Delete arranger selections"));
    case AS_ACTION_DUPLICATE:
      return g_strdup (_ ("Duplicate arranger selections"));
    case AS_ACTION_MOVE:
      return g_strdup (_ ("Move arranger selections"));
    case AS_ACTION_LINK:
      return g_strdup (_ ("Link arranger selections"));
    case AS_ACTION_RECORD:
      return g_strdup (_ ("Record arranger selections"));
    case AS_ACTION_EDIT:
      return g_strdup (_ ("Edit arranger selections"));
    case AS_ACTION_AUTOMATION_FILL:
      return g_strdup (_ ("Automation fill"));
    case AS_ACTION_SPLIT:
      return g_strdup (_ ("Split arranger selections"));
    case AS_ACTION_MERGE:
      return g_strdup (_ ("Merge arranger selections"));
    case AS_ACTION_RESIZE:
      return g_strdup (_ ("Resize arranger selections"));
    case AS_ACTION_QUANTIZE:
      return g_strdup (_ ("Quantize arranger selections"));
    default:
      break;
    }

  g_return_val_if_reached (g_strdup (""));
}

void
arranger_selections_action_free (
  ArrangerSelectionsAction * self)
{
  object_free_w_func_and_null (
    arranger_selections_free_full, self->sel);

  object_zero_and_free (self);
}
