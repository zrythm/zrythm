// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_MIXER_SELECTIONS_ACTION_H__
#define __UNDO_MIXER_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/automation_track.h"
#include "dsp/port_connections_manager.h"
#include "gui/backend/mixer_selections.h"
#include "utils/yaml.h"

typedef struct Plugin          Plugin;
typedef struct Track           Track;
typedef struct MixerSelections MixerSelections;

typedef enum MixerSelectionsActionType
{
  /** Duplicate from existing plugins. */
  MIXER_SELECTIONS_ACTION_COPY,
  /** Create new from clipboard. */
  MIXER_SELECTIONS_ACTION_PASTE,
  /** Create new from PluginSetting. */
  MIXER_SELECTIONS_ACTION_CREATE,
  MIXER_SELECTIONS_ACTION_DELETE,
  MIXER_SELECTIONS_ACTION_MOVE,
} MixerSelectionsActionType;

static const cyaml_strval_t mixer_selections_action_type_strings[] = {
  {"Copy",    MIXER_SELECTIONS_ACTION_COPY  },
  { "Paste",  MIXER_SELECTIONS_ACTION_PASTE },
  { "Create", MIXER_SELECTIONS_ACTION_CREATE},
  { "Delete", MIXER_SELECTIONS_ACTION_DELETE},
  { "Move",   MIXER_SELECTIONS_ACTION_MOVE  },
};

/**
 * Restrict selections to a channel.
 */
typedef struct MixerSelectionsAction
{
  UndoableAction parent_instance;

  MixerSelectionsActionType type;

  /** Type of starting slot to move plugins to. */
  PluginSlotType slot_type;

  /**
   * Starting target slot.
   *
   * The rest of the slots will start from this so
   * they can be calculated when doing/undoing.
   */
  int to_slot;

  /** To track position. */
  unsigned int to_track_name_hash;

  /** Whether the plugins will be copied/moved into
   * a new channel, if applicable. */
  bool new_channel;

  /** Number of plugins to create, when creating
   * new plugins. */
  int num_plugins;

  /**
   * PluginSetting to use when creating.
   */
  PluginSetting * setting;

  /**
   * Clone of mixer selections at start.
   */
  MixerSelections * ms_before;

  /**
   * Deleted plugins (ie, plugins replaced during
   * move/copy).
   *
   * Used during undo to bring them back.
   */
  MixerSelections * deleted_ms;

  /**
   * Automation tracks associated with the deleted
   * plugins.
   *
   * These are used when undoing so we can readd
   * the automation events, if applicable.
   */
  AutomationTrack * deleted_ats[16000];
  int               num_deleted_ats;

  /**
   * Automation tracks associated with the plugins.
   *
   * These are used when undoing so we can readd
   * the automation events, if applicable.
   */
  AutomationTrack * ats[16000];
  int               num_ats;

  /** A clone of the port connections at the
   * start of the action. */
  PortConnectionsManager * connections_mgr_before;

  /** A clone of the port connections after
   * applying the action. */
  PortConnectionsManager * connections_mgr_after;
} MixerSelectionsAction;

static const cyaml_schema_field_t mixer_selections_action_fields_schema[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    MixerSelectionsAction,
    parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    MixerSelectionsAction,
    type,
    mixer_selections_action_type_strings),
  YAML_FIELD_ENUM (MixerSelectionsAction, slot_type, plugin_slot_type_strings),
  YAML_FIELD_INT (MixerSelectionsAction, to_slot),
  YAML_FIELD_UINT (MixerSelectionsAction, to_track_name_hash),
  YAML_FIELD_INT (MixerSelectionsAction, new_channel),
  YAML_FIELD_INT (MixerSelectionsAction, num_plugins),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MixerSelectionsAction,
    setting,
    plugin_setting_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MixerSelectionsAction,
    ms_before,
    mixer_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MixerSelectionsAction,
    deleted_ms,
    mixer_selections_fields_schema),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    MixerSelectionsAction,
    ats,
    automation_track_schema),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    MixerSelectionsAction,
    deleted_ats,
    automation_track_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MixerSelectionsAction,
    connections_mgr_before,
    port_connections_manager_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MixerSelectionsAction,
    connections_mgr_after,
    port_connections_manager_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t mixer_selections_action_schema = {
  YAML_VALUE_PTR (MixerSelectionsAction, mixer_selections_action_fields_schema),
};

void
mixer_selections_action_init_loaded (MixerSelectionsAction * self);

/**
 * Create a new action.
 *
 * @param ms The mixer selections before the action
 *   is performed.
 * @param slot_type Target slot type.
 * @param to_track_name_hash Target track name hash,
 *   or 0 for new channel.
 * @param to_slot Target slot.
 * @param setting The plugin setting, if creating
 *   plugins.
 * @param num_plugins The number of plugins to create,
 *   if creating plugins.
 */
WARN_UNUSED_RESULT UndoableAction *
mixer_selections_action_new (
  MixerSelections *              ms,
  const PortConnectionsManager * connections_mgr,
  MixerSelectionsActionType      type,
  PluginSlotType                 slot_type,
  unsigned int                   to_track_name_hash,
  int                            to_slot,
  PluginSetting *                setting,
  int                            num_plugins,
  GError **                      error);

#define mixer_selections_action_new_create( \
  slot_type, to_tr, to_slot, setting, num_plugins, error) \
  mixer_selections_action_new ( \
    NULL, NULL, MIXER_SELECTIONS_ACTION_CREATE, slot_type, to_tr, to_slot, \
    setting, num_plugins, error)

#define mixer_selections_action_new_copy( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_new ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_COPY, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_new_paste( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_new ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_PASTE, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_new_move( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_new ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_MOVE, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_new_delete(ms, port_connections_mgr, error) \
  mixer_selections_action_new ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_DELETE, 0, 0, 0, NULL, \
    0, error)

NONNULL MixerSelectionsAction *
mixer_selections_action_clone (const MixerSelectionsAction * src);

bool
mixer_selections_action_perform (
  MixerSelections *              ms,
  const PortConnectionsManager * connections_mgr,
  MixerSelectionsActionType      type,
  PluginSlotType                 slot_type,
  unsigned int                   to_track_name_hash,
  int                            to_slot,
  PluginSetting *                setting,
  int                            num_plugins,
  GError **                      error);

#define mixer_selections_action_perform_create( \
  slot_type, to_tr, to_slot, setting, num_plugins, error) \
  mixer_selections_action_perform ( \
    NULL, NULL, MIXER_SELECTIONS_ACTION_CREATE, slot_type, to_tr, to_slot, \
    setting, num_plugins, error)

#define mixer_selections_action_perform_copy( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_perform ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_COPY, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_perform_paste( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_perform ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_PASTE, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_perform_move( \
  ms, port_connections_mgr, slot_type, to_tr, to_slot, error) \
  mixer_selections_action_perform ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_MOVE, slot_type, to_tr, \
    to_slot, NULL, 0, error)

#define mixer_selections_action_perform_delete(ms, port_connections_mgr, error) \
  mixer_selections_action_perform ( \
    ms, port_connections_mgr, MIXER_SELECTIONS_ACTION_DELETE, 0, 0, 0, NULL, \
    0, error)

int
mixer_selections_action_do (MixerSelectionsAction * self, GError ** error);

int
mixer_selections_action_undo (MixerSelectionsAction * self, GError ** error);

char *
mixer_selections_action_stringize (MixerSelectionsAction * self);

void
mixer_selections_action_free (MixerSelectionsAction * self);

#endif
