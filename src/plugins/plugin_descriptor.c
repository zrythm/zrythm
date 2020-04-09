/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "plugins/plugin_descriptor.h"

#include <gtk/gtk.h>

const char *
plugin_protocol_to_str (
  PluginProtocol prot)
{
  for (size_t i = 0;
       i < G_N_ELEMENTS (plugin_protocol_strings);
       i++)
    {
      if (plugin_protocol_strings[i].val ==
            (int64_t) prot)
        {
          return
            plugin_protocol_strings[i].str;
        }
    }
  g_return_val_if_reached (NULL);
}

/**
 * Clones the plugin descriptor.
 */
void
plugin_descriptor_copy (
  const PluginDescriptor * src,
  PluginDescriptor * dest)
{
  dest->author = g_strdup (src->author);
  dest->name = g_strdup (src->name);
  dest->website = g_strdup (src->website);
  dest->category_str = g_strdup (src->category_str);
  dest->category = src->category;
  dest->num_audio_ins = src->num_audio_ins;
  dest->num_midi_ins = src->num_midi_ins;
  dest->num_audio_outs = src->num_audio_outs;
  dest->num_midi_outs = src->num_midi_outs;
  dest->num_ctrl_ins = src->num_ctrl_ins;
  dest->num_ctrl_outs = src->num_ctrl_outs;
  dest->num_cv_ins = src->num_cv_ins;
  dest->num_cv_outs = src->num_cv_outs;
  dest->arch = src->arch;
  dest->protocol = src->protocol;
  dest->path = g_strdup (src->path);
  dest->uri = g_strdup (src->uri);
#ifdef HAVE_CARLA
  dest->open_with_carla = src->open_with_carla;
#endif
  dest->ghash = src->ghash;
}

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_descriptor_clone (
  const PluginDescriptor * src)
{
  PluginDescriptor * self =
    calloc (1, sizeof (PluginDescriptor));

  plugin_descriptor_copy (src, self);

  return self;
}

#define IS_CAT(x) \
  (descr->category == PC_##x)

/**
 * Returns if the Plugin is an instrument or not.
 */
int
plugin_descriptor_is_instrument (
  const PluginDescriptor * descr)
{
  return
    (descr->category == PC_INSTRUMENT) ||
    /* if VSTs are instruments their category must
     * be INSTRUMENT, otherwise they are not */
    (descr->protocol != PROT_VST &&
     descr->category == PLUGIN_CATEGORY_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is an effect or not.
 */
int
plugin_descriptor_is_effect (
  PluginDescriptor * descr)
{

  return
    (descr->category > PLUGIN_CATEGORY_NONE &&
     (IS_CAT (DELAY) ||
      IS_CAT (REVERB) ||
      IS_CAT (DISTORTION) ||
      IS_CAT (WAVESHAPER) ||
      IS_CAT (DYNAMICS) ||
      IS_CAT (AMPLIFIER) ||
      IS_CAT (COMPRESSOR) ||
      IS_CAT (ENVELOPE) ||
      IS_CAT (EXPANDER) ||
      IS_CAT (GATE) ||
      IS_CAT (LIMITER) ||
      IS_CAT (FILTER) ||
      IS_CAT (ALLPASS_FILTER) ||
      IS_CAT (BANDPASS_FILTER) ||
      IS_CAT (COMB_FILTER) ||
      IS_CAT (EQ) ||
      IS_CAT (MULTI_EQ) ||
      IS_CAT (PARA_EQ) ||
      IS_CAT (HIGHPASS_FILTER) ||
      IS_CAT (LOWPASS_FILTER) ||
      IS_CAT (GENERATOR) ||
      IS_CAT (CONSTANT) ||
      IS_CAT (OSCILLATOR) ||
      IS_CAT (MODULATOR) ||
      IS_CAT (CHORUS) ||
      IS_CAT (FLANGER) ||
      IS_CAT (PHASER) ||
      IS_CAT (SIMULATOR) ||
      IS_CAT (SIMULATOR_REVERB) ||
      IS_CAT (SPATIAL) ||
      IS_CAT (SPECTRAL) ||
      IS_CAT (PITCH) ||
      IS_CAT (UTILITY) ||
      IS_CAT (ANALYZER) ||
      IS_CAT (CONVERTER) ||
      IS_CAT (FUNCTION) ||
      IS_CAT (MIXER))) ||
    (descr->category == PLUGIN_CATEGORY_NONE &&
     descr->num_audio_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is a modulator or not.
 */
int
plugin_descriptor_is_modulator (
  PluginDescriptor * descr)
{
  return
    (descr->category == PLUGIN_CATEGORY_NONE ||
     (descr->category > PLUGIN_CATEGORY_NONE &&
      (IS_CAT (ENVELOPE) ||
       IS_CAT (GENERATOR) ||
       IS_CAT (CONSTANT) ||
       IS_CAT (OSCILLATOR) ||
       IS_CAT (MODULATOR) ||
       IS_CAT (UTILITY) ||
       IS_CAT (CONVERTER) ||
       IS_CAT (FUNCTION)))) &&
    descr->num_cv_outs > 0;
}

/**
 * Returns if the Plugin is a midi modifier or not.
 */
int
plugin_descriptor_is_midi_modifier (
  PluginDescriptor * descr)
{
  return
    (descr->category > PLUGIN_CATEGORY_NONE &&
     descr->category == PC_MIDI) ||
    (descr->category == PLUGIN_CATEGORY_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_midi_outs > 0 &&
     descr->protocol != PROT_VST);
}

#undef IS_CAT

/**
 * Returns the PluginCategory matching the given
 * string.
 */
PluginCategory
plugin_descriptor_string_to_category (
  const char * str)
{
  PluginCategory category = PLUGIN_CATEGORY_NONE;

#define CHECK_CAT(term,cat) \
  if (g_strrstr (str, term)) \
    category = PC_##cat

  /* add category */
  CHECK_CAT ("Delay", DELAY);
  CHECK_CAT ("Reverb", REVERB);
  CHECK_CAT ("Distortion", DISTORTION);
  CHECK_CAT ("Waveshaper", WAVESHAPER);
  CHECK_CAT ("Dynamics", DYNAMICS);
  CHECK_CAT ("Amplifier", AMPLIFIER);
  CHECK_CAT ("Compressor", COMPRESSOR);
  CHECK_CAT ("Envelope", ENVELOPE);
  CHECK_CAT ("Expander", EXPANDER);
  CHECK_CAT ("Gate", GATE);
  CHECK_CAT ("Limiter", LIMITER);
  CHECK_CAT ("Filter", FILTER);
  CHECK_CAT ("Allpass", ALLPASS_FILTER);
  CHECK_CAT ("Bandpass", BANDPASS_FILTER);
  CHECK_CAT ("Comb", COMB_FILTER);
  CHECK_CAT ("Equaliser", EQ);
  CHECK_CAT ("Equalizer", EQ);
  CHECK_CAT ("Multiband", MULTI_EQ);
  CHECK_CAT ("Para", PARA_EQ);
  CHECK_CAT ("Highpass", HIGHPASS_FILTER);
  CHECK_CAT ("Lowpass", LOWPASS_FILTER);
  CHECK_CAT ("Generator", GENERATOR);
  CHECK_CAT ("Constant", CONSTANT);
  CHECK_CAT ("Instrument", INSTRUMENT);
  CHECK_CAT ("Oscillator", OSCILLATOR);
  CHECK_CAT ("MIDI", MIDI);
  CHECK_CAT ("Modulator", MODULATOR);
  CHECK_CAT ("Chorus", CHORUS);
  CHECK_CAT ("Flanger", FLANGER);
  CHECK_CAT ("Phaser", PHASER);
  CHECK_CAT ("Simulator", SIMULATOR);
  CHECK_CAT ("SimulatorReverb", SIMULATOR_REVERB);
  CHECK_CAT ("Spatial", SPATIAL);
  CHECK_CAT ("Spectral", SPECTRAL);
  CHECK_CAT ("Pitch", PITCH);
  CHECK_CAT ("Utility", UTILITY);
  CHECK_CAT ("Analyser", ANALYZER);
  CHECK_CAT ("Analyzer", ANALYZER);
  CHECK_CAT ("Converter", CONVERTER);
  CHECK_CAT ("Function", FUNCTION);
  CHECK_CAT ("Mixer", MIXER);

#undef CHECK_CAT

  return category;
}

void
plugin_descriptor_free (
  PluginDescriptor * self)
{
  if (self->author)
    g_free (self->author);
  if (self->name)
    g_free (self->name);
  if (self->website)
    g_free (self->website);
  if (self->category_str)
    g_free (self->category_str);
  if (self->path)
    g_free (self->path);
  if (self->uri)
    g_free (self->uri);

  free (self);
}
