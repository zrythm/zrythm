// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "dsp/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/collection.h"
#include "plugins/collections.h"
#include "plugins/plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_manager.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <lv2/instance-access/instance-access.h>

PluginDescriptor *
plugin_descriptor_new (void)
{
  PluginDescriptor * self = object_new (PluginDescriptor);
  self->schema_version = PLUGIN_DESCRIPTOR_SCHEMA_VERSION;

  return self;
}

const char *
plugin_protocol_to_str (ZPluginProtocol prot)
{
  for (size_t i = 0; i < G_N_ELEMENTS (plugin_protocol_strings); i++)
    {
      if (plugin_protocol_strings[i].val == (int64_t) prot)
        {
          return plugin_protocol_strings[i].str;
        }
    }
  g_return_val_if_reached (NULL);
}

ZPluginProtocol
plugin_protocol_from_str (const char * str)
{
  for (size_t i = 0; i < G_N_ELEMENTS (plugin_protocol_strings); i++)
    {
      if (string_is_equal (plugin_protocol_strings[i].str, str))
        {
          return (ZPluginProtocol) plugin_protocol_strings[i].val;
        }
    }
  g_return_val_if_reached (Z_PLUGIN_PROTOCOL_LV2);
}

ZPluginProtocol
plugin_descriptor_get_protocol_from_carla_plugin_type (PluginType ptype)
{
  switch (ptype)
    {
    case PLUGIN_LV2:
      return Z_PLUGIN_PROTOCOL_LV2;
    case PLUGIN_AU:
      return Z_PLUGIN_PROTOCOL_AU;
    case PLUGIN_VST2:
      return Z_PLUGIN_PROTOCOL_VST;
    case PLUGIN_VST3:
      return Z_PLUGIN_PROTOCOL_VST3;
    case PLUGIN_SFZ:
      return Z_PLUGIN_PROTOCOL_SFZ;
    case PLUGIN_SF2:
      return Z_PLUGIN_PROTOCOL_SF2;
    case PLUGIN_DSSI:
      return Z_PLUGIN_PROTOCOL_DSSI;
    case PLUGIN_LADSPA:
      return Z_PLUGIN_PROTOCOL_LADSPA;
#ifdef CARLA_HAVE_CLAP_SUPPORT
    case PLUGIN_CLAP:
#else
    case 14:
#endif
      return Z_PLUGIN_PROTOCOL_CLAP;
    case PLUGIN_JSFX:
      return Z_PLUGIN_PROTOCOL_JSFX;
    default:
      g_return_val_if_reached (0);
    }

  g_return_val_if_reached (0);
}

PluginType
plugin_descriptor_get_carla_plugin_type_from_protocol (ZPluginProtocol protocol)
{
  switch (protocol)
    {
    case Z_PLUGIN_PROTOCOL_LV2:
      return PLUGIN_LV2;
    case Z_PLUGIN_PROTOCOL_AU:
      return PLUGIN_AU;
    case Z_PLUGIN_PROTOCOL_VST:
      return PLUGIN_VST2;
    case Z_PLUGIN_PROTOCOL_VST3:
      return PLUGIN_VST3;
    case Z_PLUGIN_PROTOCOL_SFZ:
      return PLUGIN_SFZ;
    case Z_PLUGIN_PROTOCOL_SF2:
      return PLUGIN_SF2;
    case Z_PLUGIN_PROTOCOL_DSSI:
      return PLUGIN_DSSI;
    case Z_PLUGIN_PROTOCOL_LADSPA:
      return PLUGIN_LADSPA;
    case Z_PLUGIN_PROTOCOL_CLAP:
#ifdef CARLA_HAVE_CLAP_SUPPORT
      return PLUGIN_CLAP;
#else
      return 14;
#endif
    case Z_PLUGIN_PROTOCOL_JSFX:
      return PLUGIN_JSFX;
    default:
      g_return_val_if_reached (0);
    }

  g_return_val_if_reached (0);
}

ZPluginCategory
plugin_descriptor_get_category_from_carla_category_str (const char * category)
{
#define EQUALS(x) string_is_equal (category, x)

  if (EQUALS ("synth"))
    return PC_INSTRUMENT;
  else if (EQUALS ("delay"))
    return PC_DELAY;
  else if (EQUALS ("eq"))
    return PC_EQ;
  else if (EQUALS ("filter"))
    return PC_FILTER;
  else if (EQUALS ("distortion"))
    return PC_DISTORTION;
  else if (EQUALS ("dynamics"))
    return PC_DYNAMICS;
  else if (EQUALS ("modulator"))
    return PC_MODULATOR;
  else if (EQUALS ("utility"))
    return PC_UTILITY;
  else
    return ZPLUGIN_CATEGORY_NONE;

#undef EQUALS
}

ZPluginCategory
plugin_descriptor_get_category_from_carla_category (PluginCategory carla_cat)
{
  switch (carla_cat)
    {
    case PLUGIN_CATEGORY_SYNTH:
      return PC_INSTRUMENT;
    case PLUGIN_CATEGORY_DELAY:
      return PC_DELAY;
    case PLUGIN_CATEGORY_EQ:
      return PC_EQ;
    case PLUGIN_CATEGORY_FILTER:
      return PC_FILTER;
    case PLUGIN_CATEGORY_DISTORTION:
      return PC_DISTORTION;
    case PLUGIN_CATEGORY_DYNAMICS:
      return PC_DYNAMICS;
    case PLUGIN_CATEGORY_MODULATOR:
      return PC_MODULATOR;
    case PLUGIN_CATEGORY_UTILITY:
      break;
    case PLUGIN_CATEGORY_OTHER:
    case PLUGIN_CATEGORY_NONE:
    default:
      break;
    }
  return ZPLUGIN_CATEGORY_NONE;
}

bool
plugin_protocol_is_supported (ZPluginProtocol protocol)
{
#ifndef __APPLE__
  if (protocol == Z_PLUGIN_PROTOCOL_AU)
    return false;
#endif
#if defined(_WOE32) || defined(__APPLE__)
  if (protocol == Z_PLUGIN_PROTOCOL_LADSPA || protocol == Z_PLUGIN_PROTOCOL_DSSI)
    return false;
#endif
#ifndef CARLA_HAVE_CLAP_SUPPORT
  if (protocol == Z_PLUGIN_PROTOCOL_CLAP)
    return false;
#endif
  return true;
}

/**
 * Clones the plugin descriptor.
 */
void
plugin_descriptor_copy (PluginDescriptor * dest, const PluginDescriptor * src)
{
  /*g_return_if_fail (src->schema_version > 0);*/
  dest->schema_version = src->schema_version;
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
  dest->sha1 = g_strdup (src->sha1);
  dest->uri = g_strdup (src->uri);
  dest->min_bridge_mode = src->min_bridge_mode;
  dest->has_custom_ui = src->has_custom_ui;
  dest->ghash = src->ghash;
}

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_descriptor_clone (const PluginDescriptor * src)
{
  PluginDescriptor * self = object_new (PluginDescriptor);

  plugin_descriptor_copy (self, src);

  return self;
}

#define IS_CAT(x) (descr->category == PC_##x)

/**
 * Returns if the Plugin is an instrument or not.
 */
bool
plugin_descriptor_is_instrument (const PluginDescriptor * const descr)
{
  if (descr->num_midi_ins == 0 || descr->num_audio_outs == 0)
    {
      return false;
    }

  if (descr->category == PC_INSTRUMENT)
    {
      return true;
    }
  else
    {
      return
        /* if VSTs are instruments their category
         * must be INSTRUMENT, otherwise they are
         * not */
        descr->protocol != Z_PLUGIN_PROTOCOL_VST
        && descr->category == ZPLUGIN_CATEGORY_NONE && descr->num_midi_ins > 0
        && descr->num_audio_outs > 0;
    }
}

/**
 * Returns if the Plugin is an effect or not.
 */
bool
plugin_descriptor_is_effect (const PluginDescriptor * const descr)
{

  return
    (descr->category > ZPLUGIN_CATEGORY_NONE &&
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
    (descr->category == ZPLUGIN_CATEGORY_NONE &&
     descr->num_audio_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is a modulator or not.
 */
int
plugin_descriptor_is_modulator (const PluginDescriptor * const descr)
{
  return (descr->category == ZPLUGIN_CATEGORY_NONE
          || (descr->category > ZPLUGIN_CATEGORY_NONE && (IS_CAT (ENVELOPE) || IS_CAT (GENERATOR) || IS_CAT (CONSTANT) || IS_CAT (OSCILLATOR) || IS_CAT (MODULATOR) || IS_CAT (UTILITY) || IS_CAT (CONVERTER) || IS_CAT (FUNCTION))))
         && descr->num_cv_outs > 0;
}

/**
 * Returns if the Plugin is a midi modifier or not.
 */
int
plugin_descriptor_is_midi_modifier (const PluginDescriptor * const descr)
{
  return
    (descr->category > ZPLUGIN_CATEGORY_NONE &&
     descr->category == PC_MIDI) ||
    (descr->category == ZPLUGIN_CATEGORY_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_midi_outs > 0 &&
     descr->protocol != Z_PLUGIN_PROTOCOL_VST);
}

#undef IS_CAT

/**
 * Returns the ZPluginCategory matching the given
 * string.
 */
ZPluginCategory
plugin_descriptor_string_to_category (const char * str)
{
  ZPluginCategory category = ZPLUGIN_CATEGORY_NONE;

#define CHECK_CAT(term, cat) \
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

const char *
plugin_category_to_string (ZPluginCategory category)
{

#define RET_STRING(term, cat) \
  if (category == PC_##cat) \
  return term

  /* add category */
  RET_STRING ("Delay", DELAY);
  RET_STRING ("Reverb", REVERB);
  RET_STRING ("Distortion", DISTORTION);
  RET_STRING ("Waveshaper", WAVESHAPER);
  RET_STRING ("Dynamics", DYNAMICS);
  RET_STRING ("Amplifier", AMPLIFIER);
  RET_STRING ("Compressor", COMPRESSOR);
  RET_STRING ("Envelope", ENVELOPE);
  RET_STRING ("Expander", EXPANDER);
  RET_STRING ("Gate", GATE);
  RET_STRING ("Limiter", LIMITER);
  RET_STRING ("Filter", FILTER);
  RET_STRING ("Allpass", ALLPASS_FILTER);
  RET_STRING ("Bandpass", BANDPASS_FILTER);
  RET_STRING ("Comb", COMB_FILTER);
  RET_STRING ("Equaliser", EQ);
  RET_STRING ("Equalizer", EQ);
  RET_STRING ("Multiband", MULTI_EQ);
  RET_STRING ("Para", PARA_EQ);
  RET_STRING ("Highpass", HIGHPASS_FILTER);
  RET_STRING ("Lowpass", LOWPASS_FILTER);
  RET_STRING ("Generator", GENERATOR);
  RET_STRING ("Constant", CONSTANT);
  RET_STRING ("Instrument", INSTRUMENT);
  RET_STRING ("Oscillator", OSCILLATOR);
  RET_STRING ("MIDI", MIDI);
  RET_STRING ("Modulator", MODULATOR);
  RET_STRING ("Chorus", CHORUS);
  RET_STRING ("Flanger", FLANGER);
  RET_STRING ("Phaser", PHASER);
  RET_STRING ("Simulator", SIMULATOR);
  RET_STRING ("SimulatorReverb", SIMULATOR_REVERB);
  RET_STRING ("Spatial", SPATIAL);
  RET_STRING ("Spectral", SPECTRAL);
  RET_STRING ("Pitch", PITCH);
  RET_STRING ("Utility", UTILITY);
  RET_STRING ("Analyser", ANALYZER);
  RET_STRING ("Analyzer", ANALYZER);
  RET_STRING ("Converter", CONVERTER);
  RET_STRING ("Function", FUNCTION);
  RET_STRING ("Mixer", MIXER);

#undef RET_STRING

  return "Plugin";
}

char *
plugin_descriptor_category_to_string (ZPluginCategory category)
{
  return g_strdup (plugin_category_to_string (category));
}

/**
 * Returns if the given plugin identifier can be
 * dropped in a slot of the given type.
 */
bool
plugin_descriptor_is_valid_for_slot_type (
  const PluginDescriptor * self,
  int                      slot_type,
  int                      track_type)
{
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      if (track_type == TRACK_TYPE_MIDI)
        {
          return self->num_midi_outs > 0;
        }
      else
        {
          return self->num_audio_outs > 0;
        }
    case PLUGIN_SLOT_MIDI_FX:
      return self->num_midi_outs > 0;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      return track_type == TRACK_TYPE_INSTRUMENT
             && plugin_descriptor_is_instrument (self);
    default:
      break;
    }

  g_return_val_if_reached (false);
}

/**
 * Returns whether the two descriptors describe
 * the same plugin, ignoring irrelevant fields.
 */
bool
plugin_descriptor_is_same_plugin (
  const PluginDescriptor * a,
  const PluginDescriptor * b)
{
  return a->arch == b->arch && a->protocol == b->protocol
         && a->unique_id == b->unique_id && a->ghash == b->ghash
         && string_is_equal (a->sha1, b->sha1) && string_is_equal (a->uri, b->uri);
}

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
bool
plugin_descriptor_has_custom_ui (const PluginDescriptor * self)
{
  switch (self->protocol)
    {
    case Z_PLUGIN_PROTOCOL_LV2:
    case Z_PLUGIN_PROTOCOL_VST:
    case Z_PLUGIN_PROTOCOL_VST3:
    case Z_PLUGIN_PROTOCOL_AU:
    case Z_PLUGIN_PROTOCOL_CLAP:
    case Z_PLUGIN_PROTOCOL_JSFX:
#ifdef HAVE_CARLA
      return carla_native_plugin_has_custom_ui (self);
#else
      return false;
#endif
      break;
    default:
      return false;
      break;
    }

  g_return_val_if_reached (false);
}

/**
 * Returns the minimum bridge mode required for this
 * plugin.
 */
CarlaBridgeMode
plugin_descriptor_get_min_bridge_mode (const PluginDescriptor * self)
{
  CarlaBridgeMode mode = CARLA_BRIDGE_NONE;

  if (self->arch == ARCH_32)
    {
      mode = CARLA_BRIDGE_FULL;
    }

  return mode;
}

/**
 * Returns whether the plugin is known to work, so it should
 * be whitelisted.
 *
 * Non-whitelisted plugins will run in full bridge mode. This
 * is to prevent crashes when Zrythm is not at fault.
 *
 * These must all be free-software plugins so that they can
 * be debugged if issues arise.
 */
bool
plugin_descriptor_is_whitelisted (const PluginDescriptor * self)
{
  /* on wayland nothing is whitelisted */
  if (z_gtk_is_wayland ())
    {
      return false;
    }

  static const char * authors[] = {
    "Alexandros Theodotou",
    "Andrew Deryabin",
    "AnnieShin",
    "Artican",
    "Aurelien Leblond",
    "Automatl",
    "Breakfast Quay",
    "brummer",
    "Clearly Broken Software",
    "Creative Intent",
    "Damien Zammit",
    "Datsounds",
    "David Robillard",
    "Digital Suburban",
    "DISTRHO",
    "dRowAudio",
    "DrumGizmo Team",
    "falkTX",
    "Filipe Coelho",
    "Guitarix team",
    "Hanspeter Portner",
    "Hermann Meyer",
    "IEM",
    "Iurie Nistor",
    "Jean Pierre Cimalando",
    "Klangfreund",
    "kRAkEn/gORe",
    "Lkjb",
    "LSP LADSPA",
    "LSP LV2",
    "LSP VST",
    "Luciano Dato",
    "Martin Eastwood, falkTX",
    "Matt Tytel",
    "Michael Willis",
    "Michael Willis and Rob vd Berg",
    "ndc Plugs",
    "OpenAV",
    "Patrick Desaulniers",
    "Paul Ferrand",
    "Plainweave Software",
    "Punk Labs LLC",
    "Resonant DSP",
    "Robin Gareus",
    "RockHardbuns",
    "SFZTools",
    "Spencer Jackson",
    "Stefan Westerfeld",
    "Surge Synth Team",
    "Sven Jaehnichen",
    "TAL-Togu Audio Line",
    "TheWaveWarden",
    "Tom Szilagyi",
    "tumbetoene",
    "Zrythm DAW",
  };

  if (!self->author)
    {
      return false;
    }

  for (size_t i = 0; i < G_N_ELEMENTS (authors); i++)
    {
      if (string_is_equal (self->author, authors[i]))
        {
#if 0
          g_debug (
            "author '%s' is whitelisted", self->author);
#endif
          return true;
        }
    }

  return false;
}

/**
 * Gets an appropriate icon name for the given
 * descriptor.
 */
const char *
plugin_descriptor_get_icon_name (const PluginDescriptor * const self)
{
  if (plugin_descriptor_is_instrument (self))
    {
      return "instrument";
    }
  else if (plugin_descriptor_is_modulator (self))
    {
      return "modulator";
    }
  else if (plugin_descriptor_is_midi_modifier (self))
    {
      return "signal-midi";
    }
  else if (plugin_descriptor_is_effect (self))
    {
      return "bars";
    }
  else
    {
      return "plug";
    }
}

GMenuModel *
plugin_descriptor_generate_context_menu (const PluginDescriptor * self)
{
  GMenu * menu = g_menu_new ();

  GMenuItem * menuitem;
  char        tmp[600];

  /* TODO */
#if 0
  /* add option for native generic LV2 UI */
  if (self->protocol == Z_PLUGIN_PROTOCOL_LV2
      &&
      self->min_bridge_mode == CARLA_BRIDGE_NONE)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (native generic UI)"),
          NULL,
          "app.plugin-browser-add-to-project");
      g_menu_append_item (menu, menuitem);
    }
#endif

#ifdef HAVE_CARLA
  sprintf (tmp, "app.plugin-browser-add-to-project-carla::%p", self);
  menuitem = z_gtk_create_menu_item (_ ("Add to project"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  PluginSetting * new_setting = plugin_setting_new_default (self);
  if (
    self->has_custom_ui && self->min_bridge_mode == CARLA_BRIDGE_NONE
    && !new_setting->force_generic_ui)
    {
      sprintf (
        tmp,
        "app.plugin-browser-add-to-project-"
        "bridged-ui::%p",
        self);
      menuitem =
        z_gtk_create_menu_item (_ ("Add to project (bridged UI)"), NULL, tmp);
      g_menu_append_item (menu, menuitem);
    }
  plugin_setting_free (new_setting);

  sprintf (
    tmp,
    "app.plugin-browser-add-to-project-bridged-"
    "full::%p",
    self);
  menuitem =
    z_gtk_create_menu_item (_ ("Add to project (bridged full)"), NULL, tmp);
  g_menu_append_item (menu, menuitem);
#endif

#if 0
  menuitem =
    GTK_MENU_ITEM (
      gtk_check_menu_item_new_with_mnemonic (
        _("Use _Generic UI")));
  APPEND;
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    new_setting->force_generic_ui);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_use_generic_ui_toggled), self);
#endif

  /* add to collection */
  GMenu * add_collections_submenu = g_menu_new ();
  int     num_added = 0;
  for (size_t i = 0; i < PLUGIN_MANAGER->collections->collections->len; i++)
    {
      PluginCollection * coll =
        g_ptr_array_index (PLUGIN_MANAGER->collections->collections, i);
      if (plugin_collection_contains_descriptor (coll, self, false))
        {
          continue;
        }

      sprintf (tmp, "app.plugin-browser-add-to-collection::%p,%p", coll, self);
      menuitem = z_gtk_create_menu_item (coll->name, NULL, tmp);
      g_menu_append_item (add_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _ ("Add to collection"), G_MENU_MODEL (add_collections_submenu));
    }
  else
    {
      g_object_unref (add_collections_submenu);
    }

  /* remove from collection */
  GMenu * remove_collections_submenu = g_menu_new ();
  num_added = 0;
  for (size_t i = 0; i < PLUGIN_MANAGER->collections->collections->len; i++)
    {
      PluginCollection * coll =
        g_ptr_array_index (PLUGIN_MANAGER->collections->collections, i);
      if (!plugin_collection_contains_descriptor (coll, self, false))
        {
          continue;
        }

      sprintf (
        tmp, "app.plugin-browser-remove-from-collection::%p,%p", coll, self);
      menuitem = z_gtk_create_menu_item (coll->name, NULL, tmp);
      g_menu_append_item (remove_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _ ("Remove from collection"),
        G_MENU_MODEL (remove_collections_submenu));
    }
  else
    {
      g_object_unref (remove_collections_submenu);
    }

  return G_MENU_MODEL (menu);
}

void
plugin_descriptor_free (PluginDescriptor * self)
{
  g_free_and_null (self->author);
  g_free_and_null (self->name);
  g_free_and_null (self->website);
  g_free_and_null (self->category_str);
  g_free_and_null (self->path);
  g_free_and_null (self->sha1);
  g_free_and_null (self->uri);

  object_zero_and_free (self);
}

void
plugin_descriptor_free_closure (void * data, GClosure * closure)
{
  PluginDescriptor * self = (PluginDescriptor *) data;
  plugin_descriptor_free (self);
}
