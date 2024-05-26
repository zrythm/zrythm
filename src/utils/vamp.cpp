/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <cstddef>

#include "dsp/vamp-ports/BeatTrack.h"
#include "dsp/vamp-ports/FixedTempoEstimator.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/vamp.h"

#include <gtk/gtk.h>

extern "C" {

/**
 * Prints detected vamp plugins.
 */
void
vamp_print_all (void)
{
  int num_vamp_libs = vhGetLibraryCount ();
  g_message ("loading %d vamp libraries...", num_vamp_libs);
  for (int i = 0; i < num_vamp_libs; i++)
    {
      const char * lib_name = vhGetLibraryName (i);
      vhLibrary    lib = vhLoadLibrary (i);
      int          num_plugins = vhGetPluginCount (lib);
      g_message ("[%d-%s: %d plugins]", i, lib_name, num_plugins);
      for (int j = 0; j < num_plugins; j++)
        {
          const VampPluginDescriptor * descr = vhGetPluginDescriptor (lib, j);
          g_message (
            "\n[plugin %d: %s]\n"
            "name: %s\n"
            "description: %s\n"
            "maker: %s\n"
            "plugin version: %d\n"
            "copyright: %s\n"
            "param count: %u\n"
            "program count: %u\n"
            "vamp API version: %d",
            j, descr->identifier, descr->name, descr->description, descr->maker,
            descr->pluginVersion, descr->copyright, descr->parameterCount,
            descr->programCount, descr->vampApiVersion);
        }
      vhUnloadLibrary (lib);
    }
}

const VampPluginDescriptor *
vamp_get_simple_fixed_tempo_estimator_descriptor (void)
{
  int num_vamp_libs = vhGetLibraryCount ();
  for (int i = 0; i < num_vamp_libs; i++)
    {
      const char * lib_name = vhGetLibraryName (i);
      if (!string_is_equal (lib_name, "vamp-example-plugins"))
        continue;

      vhLibrary lib = vhLoadLibrary (i);
      int       num_plugins = vhGetPluginCount (lib);
      for (int j = 0; j < num_plugins; j++)
        {
          const VampPluginDescriptor * descr = vhGetPluginDescriptor (lib, j);
          if (string_is_equal (descr->identifier, "fixedtempo"))
            {
              return descr;
            }
        }
    }

  g_return_val_if_reached (NULL);
}

ZVampPlugin *
vamp_get_plugin (ZVampPluginType type, float samplerate)
{
  switch (type)
    {
    case Z_VAMP_PLUGIN_BEAT_TRACKER:
      return new BeatTracker (samplerate);
    case Z_VAMP_PLUGIN_FIXED_TEMPO_ESTIMATOR:
      return new FixedTempoEstimator (samplerate);
    default:
      g_return_val_if_reached (NULL);
    }
}

void
vamp_plugin_initialize (
  ZVampPlugin * plugin,
  size_t        channels,
  size_t        step_size,
  size_t        block_size)
{
  Vamp::Plugin * pl = (Vamp::Plugin *) plugin;
  pl->initialise (channels, step_size, block_size);
}

static ZVampFeatureSet *
gen_feature_set_from_vamp_feature_set (
  Vamp::Plugin::FeatureSet &fset,
  unsigned int              samplerate)
{
  std::map<int, Vamp::Plugin::FeatureList>::iterator it;
  ZVampFeatureSet * zfset = (ZVampFeatureSet *) object_new (ZVampFeatureSet);
  zfset->set = g_ptr_array_new_with_free_func (vamp_feature_list_free);
  zfset->outputs = g_array_new (false, true, sizeof (int));
  for (it = fset.begin (); it != fset.end (); it++)
    {
      int                       key = it->first;
      Vamp::Plugin::FeatureList list = it->second;
      ZVampFeatureList *        zlist =
        (ZVampFeatureList *) object_new (ZVampFeatureList);
      zlist->list = g_ptr_array_new_full (list.size (), vamp_feature_free);
      for (Vamp::Plugin::Feature f : list)
        {
          ZVampFeature * feature = vamp_feature_new (
            f.hasTimestamp,
            Vamp::RealTime::realTime2Frame (f.timestamp, samplerate),
            f.hasDuration,
            Vamp::RealTime::realTime2Frame (f.duration, samplerate),
            &f.values[0], f.values.size (), f.label.c_str ());
          g_ptr_array_add (zlist->list, feature);
        }
      g_ptr_array_add (zfset->set, zlist);
      g_array_append_val (zfset->outputs, key);
    }

  return zfset;
}

ZVampFeatureSet *
vamp_plugin_process (
  ZVampPlugin *         plugin,
  const float * const * input_buffers,
  long                  timestamp,
  unsigned int          samplerate)
{
  Vamp::Plugin * pl = (Vamp::Plugin *) plugin;
  Vamp::RealTime realtime =
    Vamp::RealTime::frame2RealTime (timestamp, samplerate);
#if 0
  g_message (
    "processing at %s", realtime.toString().c_str());
#endif
  Vamp::Plugin::FeatureSet fset = pl->process (input_buffers, realtime);
  return gen_feature_set_from_vamp_feature_set (fset, samplerate);
}

ZVampFeatureSet *
vamp_plugin_get_remaining_features (ZVampPlugin * plugin, unsigned int samplerate)
{
  Vamp::Plugin *           pl = (Vamp::Plugin *) plugin;
  Vamp::Plugin::FeatureSet fset = pl->getRemainingFeatures ();
  return gen_feature_set_from_vamp_feature_set (fset, samplerate);
}

size_t
vamp_plugin_get_preferred_step_size (ZVampPlugin * plugin)
{
  Vamp::Plugin * pl = (Vamp::Plugin *) plugin;
  return pl->getPreferredStepSize ();
}

size_t
vamp_plugin_get_preferred_block_size (ZVampPlugin * plugin)
{
  Vamp::Plugin * pl = (Vamp::Plugin *) plugin;
  return pl->getPreferredBlockSize ();
}
ZVampOutputList *
vamp_plugin_get_output_descriptors (ZVampPlugin * plugin)
{
  Vamp::Plugin *           pl = (Vamp::Plugin *) plugin;
  Vamp::Plugin::OutputList list = pl->getOutputDescriptors ();
  ZVampOutputList * self = (ZVampOutputList *) object_new (ZVampOutputList);
  self->outputs = g_ptr_array_new_with_free_func (vamp_output_descriptor_free);
  for (Vamp::Plugin::OutputDescriptor d : list)
    {
      ZVampOutputDescriptor * output = vamp_output_descriptor_new (
        d.identifier.c_str (), d.name.c_str (), d.description.c_str (),
        d.unit.c_str (), d.hasFixedBinCount, d.hasKnownExtents, d.minValue,
        d.maxValue, d.sampleType, d.sampleRate, d.hasDuration);
      g_ptr_array_add (self->outputs, output);
    }

  return self;
}

ZVampFeature *
vamp_feature_new (
  bool         has_timestamp,
  long         timestamp,
  bool         has_duration,
  size_t       duration,
  float *      values,
  size_t       num_values,
  const char * label)
{
  ZVampFeature * self = (ZVampFeature *) object_new (ZVampFeature);

  self->has_timestamp = has_timestamp;
  self->timestamp = timestamp;
  self->has_duration = has_duration;
  self->duration = duration;
  self->values = (float *) object_new_n (num_values, float);
  memcpy (self->values, values, num_values * sizeof (float));
  self->num_values = num_values;
  self->label = g_strdup (label);

  return self;
}

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
  bool         hasDuration)
{
  ZVampOutputDescriptor * self =
    (ZVampOutputDescriptor *) object_new (ZVampOutputDescriptor);

  self->identifier = g_strdup (identifier);
  self->name = g_strdup (name);
  self->description = g_strdup (description);
  self->unit = g_strdup (unit);
  self->hasFixedBinCount = hasFixedBinCount;
  self->hasKnownExtents = hasKnownExtents;
  self->minValue = minValue;
  self->maxValue = maxValue;
  self->sampleType = sampleType;
  self->sampleRate = sampleRate;
  self->hasDuration = hasDuration;

  return self;
}

void
vamp_plugin_output_list_print (ZVampOutputList * self)
{
  g_message ("%d outputs", self->outputs->len);
  for (size_t i = 0; i < self->outputs->len; i++)
    {
      g_message ("output %zu", i);
      ZVampOutputDescriptor * o =
        (ZVampOutputDescriptor *) g_ptr_array_index (self->outputs, i);
      vamp_plugin_output_print (o);
    }
}

const ZVampFeatureList *
vamp_feature_set_get_list_for_output (ZVampFeatureSet * self, int output_idx)
{
  for (size_t i = 0; i < self->set->len; i++)
    {
      ZVampFeatureList * l =
        (ZVampFeatureList *) g_ptr_array_index (self->set, i);
      int * output = &g_array_index (self->outputs, int, i);
      if (*output == output_idx)
        return l;
    }
  return NULL;
}

void
vamp_feature_list_print (const ZVampFeatureList * self)
{
  for (size_t j = 0; j < self->list->len; j++)
    {
      g_message ("feature %zu", j);
      ZVampFeature * f = (ZVampFeature *) g_ptr_array_index (self->list, j);
      vamp_feature_print (f);
    }
}

void
vamp_feature_set_print (const ZVampFeatureSet * self)
{
  if (self->set->len == 0)
    return;

  g_message ("%d features", self->set->len);
  for (size_t i = 0; i < self->set->len; i++)
    {
      ZVampFeatureList * l =
        (ZVampFeatureList *) g_ptr_array_index (self->set, i);
      int * output = &g_array_index (self->outputs, int, i);
      g_message ("output %d", *output);
      vamp_feature_list_print (l);
    }
}

void
vamp_plugin_output_print (ZVampOutputDescriptor * self)
{
  g_message (
    "identifier: %s\n"
    "name: %s\n"
    "description: %s",
    self->identifier, self->name, self->description);
}

void
vamp_feature_print (ZVampFeature * self)
{
  GString * gstr = g_string_new (NULL);
  g_string_append_printf (
    gstr,
    "Has timestamp: %d\n"
    "timestamp: %ld\n"
    "has duration: %d\n"
    "duration: %zu\n"
    "values: (",
    self->has_timestamp, self->timestamp, self->has_duration, self->duration);
  for (size_t i = 0; i < self->num_values; i++)
    {
      g_string_append_printf (gstr, "%f, ", (double) self->values[i]);
    }
  if (self->num_values > 0)
    {
      g_string_erase (gstr, gstr->len - 2, -1);
    }
  g_string_append_printf (gstr, ")\nlabel: %s", self->label);

  char * str = g_string_free (gstr, false);
  g_message ("%s", str);
  g_free (str);
}

void
vamp_feature_free (void * f)
{
  ZVampFeature * self = (ZVampFeature *) f;
  if (self->values)
    free (self->values);
  if (self->label)
    g_free (self->label);

  free (self);
}

void
vamp_feature_list_free (void * list)
{
  ZVampFeatureList * self = (ZVampFeatureList *) list;
  if (self->list)
    g_ptr_array_unref (self->list);
  free (self);
}

void
vamp_feature_set_free (ZVampFeatureSet * self)
{
  if (self->set)
    g_ptr_array_unref (self->set);
  if (self->outputs)
    g_array_unref (self->outputs);
  free (self);
}

void
vamp_output_descriptor_free (void * descr)
{
  ZVampOutputDescriptor * self = (ZVampOutputDescriptor *) descr;

  if (self->identifier)
    g_free (self->identifier);
  if (self->name)
    g_free (self->name);
  if (self->description)
    g_free (self->description);
  if (self->unit)
    g_free (self->unit);

  free (self);
}

void
vamp_plugin_output_list_free (ZVampOutputList * self)
{
  g_ptr_array_unref (self->outputs);
  free (self);
}

} /* extern "C" */
