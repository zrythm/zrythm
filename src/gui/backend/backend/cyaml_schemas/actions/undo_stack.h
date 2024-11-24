/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Undo stack schema.
 */

#ifndef __SCHEMAS_UNDO_UNDO_STACK_H__
#define __SCHEMAS_UNDO_UNDO_STACK_H__

#include "gui/backend/backend/cyaml_schemas/actions/arranger_selections.h"
#include "gui/backend/backend/cyaml_schemas/actions/channel_send_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/midi_mapping_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/mixer_selections_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/port_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/port_connection_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/range_action.h"
#include "gui/backend/backend/cyaml_schemas/actions/tracklist_selections.h"
#include "gui/backend/backend/cyaml_schemas/actions/transport_action.h"
#include "gui/backend/backend/cyaml_schemas/utils/stack.h"

#include "utils/yaml.h"

typedef struct UndoStack_v1
{
  int                             schema_version;
  Stack_v1 *                      stack;
  ArrangerSelectionsAction_v1 **  as_actions;
  size_t                          num_as_actions;
  size_t                          as_actions_size;
  MixerSelectionsAction_v1 **     mixer_selections_actions;
  size_t                          num_mixer_selections_actions;
  size_t                          mixer_selections_actions_size;
  TracklistSelectionsAction_v1 ** tracklist_selections_actions;
  size_t                          num_tracklist_selections_actions;
  size_t                          tracklist_selections_actions_size;
  ChannelSendAction_v1 **         channel_send_actions;
  size_t                          num_channel_send_actions;
  size_t                          channel_send_actions_size;
  PortConnectionAction_v1 **      port_connection_actions;
  size_t                          num_port_connection_actions;
  size_t                          port_connection_actions_size;
  PortAction_v1 **                port_actions;
  size_t                          num_port_actions;
  size_t                          port_actions_size;
  MidiMappingAction_v1 **         midi_mapping_actions;
  size_t                          num_midi_mapping_actions;
  size_t                          midi_mapping_actions_size;
  RangeAction_v1 **               range_actions;
  size_t                          num_range_actions;
  size_t                          range_actions_size;
  TransportAction_v1 **           transport_actions;
  size_t                          num_transport_actions;
  size_t                          transport_actions_size;
} UndoStack_v1;

static const cyaml_schema_field_t undo_stack_fields_schema_v1[] = {
  YAML_FIELD_INT (UndoStack_v1, schema_version),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    as_actions,
    arranger_selections_action_schema_v1_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    mixer_selections_actions,
    mixer_selections_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    tracklist_selections_actions,
    tracklist_selections_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    channel_send_actions,
    channel_send_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    port_connection_actions,
    port_connection_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    port_actions,
    port_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    midi_mapping_actions,
    midi_mapping_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    range_actions,
    range_action_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack_v1,
    transport_actions,
    transport_action_schema_v1),
  YAML_FIELD_MAPPING_PTR (UndoStack_v1, stack, stack_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t undo_stack_schema_v1 = {
  YAML_VALUE_PTR (UndoStack_v1, undo_stack_fields_schema_v1),
};

#endif
