// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * AUDIO functions.
 *
 * TODO move to a more appropriate directory.
 */

#ifndef __AUDIO_AUDIO_FUNCTION_H__
#define __AUDIO_AUDIO_FUNCTION_H__

#include "gui/dsp/arranger_object.h"
#include "gui/dsp/plugin.h"
#include "utils/format.h"
#include "utils/logger.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

enum class AudioFunctionType
{
  Invert,
  NormalizePeak,
  NormalizeRMS,
  NormalizeLUFS,
  LinearFadeIn,
  LinearFadeOut,
  NudgeLeft,
  NudgeRight,
  Reverse,
  PitchShift,
  CopyLtoR,

  /** External program. */
  ExternalProgram,

  /** Guile script (currently unavailable). */
  Script,

  /** Custom plugin. */
  CustomPlugin,

  /* reserved */
  Invalid,
};

class AudioFunctionOpts
{
public:
  /**
   * Amount related to the current function (e.g. pitch shift).
   */
  double amount_ = 0;
};

std::string
audio_function_get_action_target_for_type (AudioFunctionType type);

/**
 * Returns a detailed action name to be used for
 * actionable widgets or menus.
 *
 * @param base_action Base action to use.
 */
std::string
audio_function_get_detailed_action_for_type (
  AudioFunctionType  type,
  const std::string &base_action);

#define audio_function_get_detailed_action_for_type_default(type) \
  audio_function_get_detailed_action_for_type (type, "app.editor-function")

const char *
audio_function_get_icon_name_for_type (AudioFunctionType type);

/**
 * Returns the URI of the plugin responsible for handling the
 * type, if any.
 */
static inline const char *
audio_function_get_plugin_uri_for_type (AudioFunctionType type)
{
  switch (type)
    {
    default:
      break;
    }

  return NULL;
}

/**
 * Applies the given action to the given selections.
 *
 * This will save a file in the pool and store the pool ID in the selections.
 *
 * @param sel Selections to edit.
 * @param type Function type. If invalid is passed, this will simply add the
 * audio file in the pool for the unchanged audio material (used in audio
 * selection actions for the selections before the change).
 * @throw ZrythmException on error.
 */
void
audio_function_apply (
  ArrangerObject::Uuid       region_id,
  const dsp::Position       &sel_start,
  const dsp::Position       &sel_end,
  AudioFunctionType          type,
  AudioFunctionOpts          opts,
  std::optional<std::string> uri);

DEFINE_ENUM_FORMATTER (
  AudioFunctionType,
  AudioFunctionType,
  QT_TR_NOOP_UTF8 ("Invert"),
  QT_TR_NOOP_UTF8 ("Normalize peak"),
  QT_TR_NOOP_UTF8 ("Normalize RMS"),
  QT_TR_NOOP_UTF8 ("Normalize LUFS"),
  QT_TR_NOOP_UTF8 ("Linear fade in"),
  QT_TR_NOOP_UTF8 ("Linear fade out"),
  QT_TR_NOOP_UTF8 ("Nudge left"),
  QT_TR_NOOP_UTF8 ("Nudge right"),
  QT_TR_NOOP_UTF8 ("Reverse"),
  QT_TR_NOOP_UTF8 ("Pitch shift"),
  QT_TR_NOOP_UTF8 ("Copy L to R"),
  QT_TR_NOOP_UTF8 ("External program"),
  QT_TR_NOOP_UTF8 ("Guile script"),
  QT_TR_NOOP_UTF8 ("Custom plugin"),
  QT_TR_NOOP_UTF8 ("Invalid"));

/**
 * @}
 */

#endif
