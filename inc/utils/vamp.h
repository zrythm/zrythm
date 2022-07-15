/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * Centre for Digital Music, Queen Mary, University of London.

  Copyright 2006 Chris Cannam.

  Permission is hereby granted, free of charge, to any person

  obtaining a copy of this software and associated documentation

  files (the "Software"), to deal in the Software without

  restriction, including without limitation the rights to use, copy,

  modify, merge, publish, distribute, sublicense, and/or sell copies

  of the Software, and to permit persons to whom the Software is

  furnished to do so, subject to the following conditions:


  The above copyright notice and this permission notice shall be

  included in all copies or substantial portions of the Software.


  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,

  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF

  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND

  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR

  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF

  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION

  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  Except as contained in this notice, the names of the Centre for

  Digital Music; Queen Mary, University of London; and Chris Cannam

  shall not be used in advertising or otherwise to promote the sale,

  use or other dealings in this Software without prior written

  authorization.

 */

/**
 * \file
 *
 * Vamp plugin utils.
 *
 * See https://code.soundsoftware.ac.uk/projects/vamp-plugin-sdk/repository/entry/vamp/vamp.h
 * and https://code.soundsoftware.ac.uk/projects/vamp-plugin-sdk/repository/entry/vamp-hostsdk/host-c.h
 * for API.
 */

#ifndef __UTILS_VAMP_H__
#define __UTILS_VAMP_H__

#include <glib.h>

#include <vamp-hostsdk/host-c.h>
#include <vamp/vamp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup utils
 *
 * @{
 */

typedef void ZVampPlugin;

typedef enum ZVampPluginType
{
  Z_VAMP_PLUGIN_BEAT_TRACKER,
  Z_VAMP_PLUGIN_FIXED_TEMPO_ESTIMATOR,
} ZVampPluginType;

typedef struct ZVampFeature
{
  /**
   * True if an output feature has its own
   * timestamp.
   *
   * This is mandatory if the output has
   * VariableSampleRate, optional if
   * the output has FixedSampleRate, and unused if
   * the output has OneSamplePerStep.
   */

  bool has_timestamp;

  /**
   * Timestamp of the output feature.  This is
   * mandatory if the output has VariableSampleRate
   * or if the output has FixedSampleRate and
   * hasTimestamp is true, and unused otherwise.
   *
   * Number of frames from start (per channel).
   */
  long timestamp;

  /**
   * True if an output feature has a specified
   * duration.  This is optional if the output has
   * VariableSampleRate or FixedSampleRate, and
   * unused if the output has OneSamplePerStep.
   */
  bool has_duration;

  /**
   * Duration of the output feature.  This is
   * mandatory if the output has VariableSampleRate
   * or FixedSampleRate and hasDuration is true,
   * and unused otherwise.
   *
   * Number of frames.
   */
  size_t duration;

  /**
   * Results for a single sample of this feature.
   * If the output hasFixedBinCount, there must be
   * the same number of values as the output's
   * binCount count.
   */
  float * values;

  size_t num_values;

  /**
   * Label for the sample of this feature.
   */
  char * label;
} ZVampFeature;

typedef struct ZVampOutputDescriptor
{
  /**
   * The name of the output, in computer-usable
   * form.  Should be reasonably short and without
   * whitespace or punctuation, using
   * the characters [a-zA-Z0-9_-] only.
   * Example: "zero_crossing_count"
   */
  char * identifier;

  /**
   * The human-readable name of the output.
   * Example: "Zero Crossing Counts"
   */
  char * name;

  /**
   * A human-readable short text describing the
   * output.  May be empty if the name has said it
   * all already.
   * Example: "The number of zero crossing points
   * per processing block".
   */
  char * description;

  /**
   * The unit of the output, in human-readable form.
   */
  char * unit;

  /**
   * True if the output has the same number of
   * values per sample for every output sample.
   * Outputs for which this is false are unlikely to
   * be very useful in a general-purpose host.
   */
  bool hasFixedBinCount;

  /**
   * True if the results in each output bin fall within a fixed
   * numeric range (minimum and maximum values).  Undefined if
   * binCount is zero.
   */
  bool hasKnownExtents;

  /**
   * Minimum value of the results in the output.  Undefined if
   * hasKnownExtents is false or binCount is zero.
   */
  float minValue;

  /**
   * Maximum value of the results in the output.  Undefined if
   * hasKnownExtents is false or binCount is zero.
   */
  float maxValue;

  /**
   * Positioning in time of the output results.
   */
  int sampleType;

  /**

   * Sample rate of the output results, as samples per second.
   * Undefined if sampleType is OneSamplePerStep.
   *
   * If sampleType is VariableSampleRate and this value is
   * non-zero, then it may be used to calculate a resolution for
   * the output (i.e. the "duration" of each sample, in time,
   * will be 1/sampleRate seconds).  It's recommended to set
   * this to zero if that behaviour is not desired.
   */
  float sampleRate;

  /**
   * True if the returned results for this output are known to
   * have a duration field.
   */
  bool hasDuration;
} ZVampOutputDescriptor;

typedef struct ZVampOutputList
{
  GPtrArray * outputs;
} ZVampOutputList;

typedef struct ZVampFeatureList
{
  /** Array of allocated ZVampFeature pointers. */
  GPtrArray * list;
} ZVampFeatureList;

typedef struct ZVampFeatureSet
{
  /**
   * Contains pointers to ZVampFeatureList.
   *
   * Index: output index.
   */
  GPtrArray * set;

  /** Array of output indices. */
  GArray * outputs;
} ZVampFeatureSet;

/**
 * Prints detected vamp plugins.
 */
void
vamp_print_all (void);

const VampPluginDescriptor *
vamp_get_simple_fixed_tempo_estimator_descriptor (void);

ZVampPlugin *
vamp_get_plugin (ZVampPluginType type, float samplerate);

void
vamp_plugin_initialize (
  ZVampPlugin * plugin,
  size_t        channels,
  size_t        step_size,
  size_t        block_size);

/**
 */
ZVampFeatureSet *
vamp_plugin_process (
  ZVampPlugin *         plugin,
  const float * const * input_buffers,
  long                  timestamp,
  unsigned int          samplerate);

ZVampFeatureSet *
vamp_plugin_get_remaining_features (
  ZVampPlugin * plugin,
  unsigned int  samplerate);

size_t
vamp_plugin_get_preferred_step_size (ZVampPlugin * plugin);

size_t
vamp_plugin_get_preferred_block_size (ZVampPlugin * plugin);

ZVampOutputList *
vamp_plugin_get_output_descriptors (ZVampPlugin * plugin);

ZVampOutputDescriptor *
vamp_output_descriptor_new (
  const char * identifier,
  const char * name,
  const char * description,
  const char * unit,
  bool         hasFixedBinCount,
  bool         hasKnownExtents,
  float        minValue,
  float        maxValue,
  int          sampleType,
  float        sampleRate,
  bool         hasDuration);

ZVampFeature *
vamp_feature_new (
  bool         has_timestamp,
  long         timestamp,
  bool         hasDuration,
  size_t       duration,
  float *      values,
  size_t       num_values,
  const char * label);

const ZVampFeatureList *
vamp_feature_set_get_list_for_output (
  ZVampFeatureSet * self,
  int               output_idx);

void
vamp_feature_list_print (const ZVampFeatureList * self);

void
vamp_feature_set_print (const ZVampFeatureSet * self);

void
vamp_feature_print (ZVampFeature * self);

void
vamp_plugin_output_print (ZVampOutputDescriptor * self);

void
vamp_plugin_output_list_print (ZVampOutputList * self);

void
vamp_feature_free (void * self);

void
vamp_feature_list_free (void * list);

void
vamp_output_descriptor_free (void * descr);

void
vamp_feature_set_free (ZVampFeatureSet * self);

void
vamp_plugin_output_list_free (ZVampOutputList * self);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // __UTILS_VAMP_H__
