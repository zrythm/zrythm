// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Transport action.
 */

#ifndef __ACTIONS_TRANSPORT_ACTION_H__
#define __ACTIONS_TRANSPORT_ACTION_H__

#include <stdbool.h>

#include "actions/undoable_action.h"
#include "dsp/transport.h"
#include "utils/types.h"
#include "utils/yaml.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum TransportActionType
{
  TRANSPORT_ACTION_BPM_CHANGE,
  TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE,
  TRANSPORT_ACTION_BEAT_UNIT_CHANGE,
} TransportActionType;

static const cyaml_strval_t transport_action_type_strings[] = {
  { "BPM change",           TRANSPORT_ACTION_BPM_CHANGE           },
  { "beats per bar change", TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE },
  { "beat unit change",     TRANSPORT_ACTION_BEAT_UNIT_CHANGE     },
};

/**
 * Transport action.
 */
typedef struct TransportAction
{
  UndoableAction parent_instance;

  TransportActionType type;

  bpm_t bpm_before;
  bpm_t bpm_after;

  int int_before;
  int int_after;

  /** Flag whether the action was already performed
   * the first time. */
  bool already_done;

  /** Whether musical mode was enabled when this
   * action was made. */
  bool musical_mode;
} TransportAction;

void
transport_action_init_loaded (TransportAction * self);

WARN_UNUSED_RESULT UndoableAction *
transport_action_new_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error);

WARN_UNUSED_RESULT UndoableAction *
transport_action_new_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error);

NONNULL TransportAction *
transport_action_clone (const TransportAction * src);

bool
transport_action_perform_bpm_change (
  bpm_t     bpm_before,
  bpm_t     bpm_after,
  bool      already_done,
  GError ** error);

bool
transport_action_perform_time_sig_change (
  TransportActionType type,
  int                 before,
  int                 after,
  bool                already_done,
  GError **           error);

int
transport_action_do (TransportAction * self, GError ** error);

int
transport_action_undo (TransportAction * self, GError ** error);

char *
transport_action_stringize (TransportAction * self);

void
transport_action_free (TransportAction * self);

/**
 * @}
 */

#endif
