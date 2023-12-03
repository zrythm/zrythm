// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Quantize options.
 */

#ifndef __AUDIO_QUANTIZE_OPTIONS_H__
#define __AUDIO_QUANTIZE_OPTIONS_H__

#include "dsp/position.h"
#include "dsp/snap_grid.h"
#include "utils/yaml.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define QUANTIZE_OPTIONS_SCHEMA_VERSION 1

#define QUANTIZE_OPTIONS_IS_EDITOR(qo) (PROJECT->quantize_opts_editor == qo)
#define QUANTIZE_OPTIONS_IS_TIMELINE(qo) (PROJECT->quantize_opts_timeline == qo)
#define QUANTIZE_OPTIONS_TIMELINE (PROJECT->quantize_opts_timeline)
#define QUANTIZE_OPTIONS_EDITOR (PROJECT->quantize_opts_editor)

#define MAX_SNAP_POINTS 120096

typedef struct QuantizeOptions
{
  int schema_version;

  /** See SnapGrid. */
  NoteLength note_length;

  /** See SnapGrid. */
  NoteType note_type;

  /** Percentage to apply quantize (0-100). */
  float amount;

  /** Adjust start position or not (only applies to
   * objects with length. */
  int adj_start;

  /** Adjust end position or not (only applies to
   * objects with length. */
  int adj_end;

  /** Swing amount (0-100). */
  float swing;

  /** Number of ticks for randomization. */
  double rand_ticks;

  /**
   * Quantize points.
   *
   * These only take into account note_length,
   * note_type and swing. They don't take into
   * account the amount % or randomization ticks.
   *
   * Not to be serialized.
   */
  Position q_points[MAX_SNAP_POINTS];
  int      num_q_points;
} QuantizeOptions;

static const cyaml_schema_field_t quantize_options_fields_schema[] = {
  YAML_FIELD_INT (QuantizeOptions, schema_version),
  YAML_FIELD_ENUM (QuantizeOptions, note_length, note_length_strings),
  YAML_FIELD_ENUM (QuantizeOptions, note_type, note_type_strings),
  YAML_FIELD_FLOAT (QuantizeOptions, amount),
  YAML_FIELD_INT (QuantizeOptions, adj_start),
  YAML_FIELD_INT (QuantizeOptions, adj_end),
  YAML_FIELD_FLOAT (QuantizeOptions, swing),
  YAML_FIELD_FLOAT (QuantizeOptions, rand_ticks),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t quantize_options_schema = {
  YAML_VALUE_PTR (QuantizeOptions, quantize_options_fields_schema),
};

void
quantize_options_init (QuantizeOptions * self, NoteLength note_length);

/**
 * Updates snap points.
 */
void
quantize_options_update_quantize_points (QuantizeOptions * self);

float
quantize_options_get_swing (QuantizeOptions * self);

float
quantize_options_get_amount (QuantizeOptions * self);

float
quantize_options_get_randomization (QuantizeOptions * self);

void
quantize_options_set_swing (QuantizeOptions * self, float swing);

void
quantize_options_set_amount (QuantizeOptions * self, float amount);

void
quantize_options_set_randomization (QuantizeOptions * self, float randomization);

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
quantize_options_stringize (NoteLength note_length, NoteType note_type);

/**
 * Quantizes the given Position using the given
 * QuantizeOptions.
 *
 * This assumes that the start/end check has been
 * done already and it ignores the adjust_start and
 * adjust_end options.
 *
 * @return The amount of ticks moved (negative for
 *   backwards).
 */
double
quantize_options_quantize_position (QuantizeOptions * self, Position * pos);

/**
 * Clones the QuantizeOptions.
 */
QuantizeOptions *
quantize_options_clone (const QuantizeOptions * src);

QuantizeOptions *
quantize_options_new (void);

/**
 * Free's the QuantizeOptions.
 */
void
quantize_options_free (QuantizeOptions * self);

/**
 * @}
 */

#endif
