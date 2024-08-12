// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "dsp/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/collections.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_manager.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

constexpr const char * plugin_protocol_strings[] = {
  "Dummy", "LV2", "DSSI", "LADSPA", "VST",  "VST3",
  "AU",    "SFZ", "SF2",  "CLAP",   "JSFX",
};

std::string
PluginDescriptor::get_icon_name_for_protocol (PluginProtocol prot)
{
  std::string icon;
  switch (prot)
    {
    case PluginProtocol::LV2:
      icon = "logo-lv2";
      break;
    case PluginProtocol::LADSPA:
      icon = "logo-ladspa";
      break;
    case PluginProtocol::AU:
      icon = "logo-au";
      break;
    case PluginProtocol::VST:
    case PluginProtocol::VST3:
      icon = "logo-vst";
      break;
    case PluginProtocol::SFZ:
    case PluginProtocol::SF2:
      icon = "file-music-line";
      break;
    default:
      icon = "plug";
      break;
    }
  return icon;
}

std::string
PluginDescriptor::plugin_protocol_to_str (PluginProtocol prot)
{
  return plugin_protocol_strings[ENUM_VALUE_TO_INT (prot)];
}

PluginProtocol
PluginDescriptor::plugin_protocol_from_str (const std::string &str)
{
  for (size_t i = 0; i < G_N_ELEMENTS (plugin_protocol_strings); i++)
    {
      if (plugin_protocol_strings[i] == str)
        {
          return (PluginProtocol) i;
        }
    }
  g_return_val_if_reached (PluginProtocol::LV2);
}

PluginProtocol
PluginDescriptor::get_protocol_from_carla_plugin_type (PluginType ptype)
{
  switch (ptype)
    {
    case CarlaBackend::PLUGIN_LV2:
      return PluginProtocol::LV2;
    case CarlaBackend::PLUGIN_AU:
      return PluginProtocol::AU;
    case CarlaBackend::PLUGIN_VST2:
      return PluginProtocol::VST;
    case CarlaBackend::PLUGIN_VST3:
      return PluginProtocol::VST3;
    case CarlaBackend::PLUGIN_SFZ:
      return PluginProtocol::SFZ;
    case CarlaBackend::PLUGIN_SF2:
      return PluginProtocol::SF2;
    case CarlaBackend::PLUGIN_DSSI:
      return PluginProtocol::DSSI;
    case CarlaBackend::PLUGIN_LADSPA:
      return PluginProtocol::LADSPA;
#ifdef CARLA_HAVE_CLAP_SUPPORT
    case CarlaBackend::PLUGIN_CLAP:
#else
    case (PluginType) 14:
#endif
      return PluginProtocol::CLAP;
    case CarlaBackend::PLUGIN_JSFX:
      return PluginProtocol::JSFX;
    default:
      g_return_val_if_reached (ENUM_INT_TO_VALUE (PluginProtocol, 0));
    }

  g_return_val_if_reached (ENUM_INT_TO_VALUE (PluginProtocol, 0));
}

PluginType
PluginDescriptor::get_carla_plugin_type_from_protocol (PluginProtocol protocol)
{
  switch (protocol)
    {
    case PluginProtocol::LV2:
      return CarlaBackend::PLUGIN_LV2;
    case PluginProtocol::AU:
      return CarlaBackend::PLUGIN_AU;
    case PluginProtocol::VST:
      return CarlaBackend::PLUGIN_VST2;
    case PluginProtocol::VST3:
      return CarlaBackend::PLUGIN_VST3;
    case PluginProtocol::SFZ:
      return CarlaBackend::PLUGIN_SFZ;
    case PluginProtocol::SF2:
      return CarlaBackend::PLUGIN_SF2;
    case PluginProtocol::DSSI:
      return CarlaBackend::PLUGIN_DSSI;
    case PluginProtocol::LADSPA:
      return CarlaBackend::PLUGIN_LADSPA;
    case PluginProtocol::CLAP:
#ifdef CARLA_HAVE_CLAP_SUPPORT
      return CarlaBackend::PLUGIN_CLAP;
#else
      return (PluginType) 14;
#endif
    case PluginProtocol::JSFX:
      return CarlaBackend::PLUGIN_JSFX;
    default:
      g_return_val_if_reached ((PluginType) 0);
    }

  g_return_val_if_reached ((PluginType) 0);
}

ZPluginCategory
PluginDescriptor::get_category_from_carla_category_str (
  const std::string &category)
{
#define EQUALS(x) (category == x)

  if (EQUALS ("synth"))
    return ZPluginCategory::INSTRUMENT;
  else if (EQUALS ("delay"))
    return ZPluginCategory::DELAY;
  else if (EQUALS ("eq"))
    return ZPluginCategory::EQ;
  else if (EQUALS ("filter"))
    return ZPluginCategory::FILTER;
  else if (EQUALS ("distortion"))
    return ZPluginCategory::DISTORTION;
  else if (EQUALS ("dynamics"))
    return ZPluginCategory::DYNAMICS;
  else if (EQUALS ("modulator"))
    return ZPluginCategory::MODULATOR;
  else if (EQUALS ("utility"))
    return ZPluginCategory::UTILITY;
  else
    return ZPluginCategory::NONE;

#undef EQUALS
}

ZPluginCategory
PluginDescriptor::get_category_from_carla_category (
  CarlaBackend::PluginCategory carla_cat)
{
  switch (carla_cat)
    {
    case CarlaBackend::PLUGIN_CATEGORY_SYNTH:
      return ZPluginCategory::INSTRUMENT;
    case CarlaBackend::PLUGIN_CATEGORY_DELAY:
      return ZPluginCategory::DELAY;
    case CarlaBackend::PLUGIN_CATEGORY_EQ:
      return ZPluginCategory::EQ;
    case CarlaBackend::PLUGIN_CATEGORY_FILTER:
      return ZPluginCategory::FILTER;
    case CarlaBackend::PLUGIN_CATEGORY_DISTORTION:
      return ZPluginCategory::DISTORTION;
    case CarlaBackend::PLUGIN_CATEGORY_DYNAMICS:
      return ZPluginCategory::DYNAMICS;
    case CarlaBackend::PLUGIN_CATEGORY_MODULATOR:
      return ZPluginCategory::MODULATOR;
    case CarlaBackend::PLUGIN_CATEGORY_UTILITY:
      break;
    case CarlaBackend::PLUGIN_CATEGORY_OTHER:
    case CarlaBackend::PLUGIN_CATEGORY_NONE:
    default:
      break;
    }
  return ZPluginCategory::NONE;
}

bool
PluginDescriptor::protocol_is_supported (PluginProtocol protocol)
{
#ifndef __APPLE__
  if (protocol == PluginProtocol::AU)
    return false;
#endif
#if defined(_WIN32) || defined(__APPLE__)
  if (protocol == PluginProtocol::LADSPA || protocol == PluginProtocol::DSSI)
    return false;
#endif
#ifndef CARLA_HAVE_CLAP_SUPPORT
  if (protocol == PluginProtocol::CLAP)
    return false;
#endif
  return true;
}

#define IS_CAT(x) (category_ == ZPluginCategory::x)

bool
PluginDescriptor::is_instrument () const
{
  if (this->num_midi_ins_ == 0 || this->num_audio_outs_ == 0)
    {
      return false;
    }

  if (this->category_ == ZPluginCategory::INSTRUMENT)
    {
      return true;
    }
  else
    {
      return
        /* if VSTs are instruments their category must be INSTRUMENT, otherwise
           they are not */
        this->protocol_ != PluginProtocol::VST
        && this->category_ == ZPluginCategory::NONE && this->num_midi_ins_ > 0
        && this->num_audio_outs_ > 0;
    }
}

bool
PluginDescriptor::is_effect () const
{
  return (this->category_ > ZPluginCategory::NONE
          && (IS_CAT (DELAY) || IS_CAT (REVERB) || IS_CAT (DISTORTION) || IS_CAT (WAVESHAPER) || IS_CAT (DYNAMICS) || IS_CAT (AMPLIFIER) || IS_CAT (COMPRESSOR) || IS_CAT (ENVELOPE) || IS_CAT (EXPANDER) || IS_CAT (GATE) || IS_CAT (LIMITER) || IS_CAT (FILTER) || IS_CAT (ALLPASS_FILTER) || IS_CAT (BANDPASS_FILTER) || IS_CAT (COMB_FILTER) || IS_CAT (EQ) || IS_CAT (MULTI_EQ) || IS_CAT (PARA_EQ) || IS_CAT (HIGHPASS_FILTER) || IS_CAT (LOWPASS_FILTER) || IS_CAT (GENERATOR) || IS_CAT (CONSTANT) || IS_CAT (OSCILLATOR) || IS_CAT (MODULATOR) || IS_CAT (CHORUS) || IS_CAT (FLANGER) || IS_CAT (PHASER) || IS_CAT (SIMULATOR) || IS_CAT (SIMULATOR_REVERB) || IS_CAT (SPATIAL) || IS_CAT (SPECTRAL) || IS_CAT (PITCH) || IS_CAT (UTILITY) || IS_CAT (ANALYZER) || IS_CAT (CONVERTER) || IS_CAT (FUNCTION) || IS_CAT (MIXER)))
         || (this->category_ == ZPluginCategory::NONE && this->num_audio_ins_ > 0 && this->num_audio_outs_ > 0);
}

bool
PluginDescriptor::is_modulator () const
{
  return (this->category_ == ZPluginCategory::NONE
          || (this->category_ > ZPluginCategory::NONE && (IS_CAT (ENVELOPE) || IS_CAT (GENERATOR) || IS_CAT (CONSTANT) || IS_CAT (OSCILLATOR) || IS_CAT (MODULATOR) || IS_CAT (UTILITY) || IS_CAT (CONVERTER) || IS_CAT (FUNCTION))))
         && this->num_cv_outs_ > 0;
}

bool
PluginDescriptor::is_midi_modifier () const
{
  return (this->category_ > ZPluginCategory::NONE
          && this->category_ == ZPluginCategory::MIDI)
         || (this->category_ == ZPluginCategory::NONE && this->num_midi_ins_ > 0 && this->num_midi_outs_ > 0 && this->protocol_ != PluginProtocol::VST);
}

#undef IS_CAT

ZPluginCategory
PluginDescriptor::string_to_category (const std::string &str)
{
  ZPluginCategory category = ZPluginCategory::NONE;

#define CHECK_CAT(term, cat) \
  if (g_strrstr (str.c_str (), term)) \
  category = ZPluginCategory::cat

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

std::string
PluginDescriptor::category_to_string (ZPluginCategory category)
{

#define RET_STRING(term, cat) \
  if (category == ZPluginCategory::cat) \
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

bool
PluginDescriptor::is_valid_for_slot_type (
  PluginSlotType slot_type,
  Track::Type    track_type) const
{
  switch (slot_type)
    {
    case PluginSlotType::Insert:
      if (track_type == Track::Type::Midi)
        {
          return num_midi_outs_ > 0;
        }
      else
        {
          return num_audio_outs_ > 0;
        }
    case PluginSlotType::MidiFx:
      return num_midi_outs_ > 0;
      break;
    case PluginSlotType::Instrument:
      return track_type == Track::Type::Instrument && is_instrument ();
    default:
      break;
    }

  g_return_val_if_reached (false);
}

bool
PluginDescriptor::has_custom_ui () const
{
  switch (protocol_)
    {
    case PluginProtocol::LV2:
    case PluginProtocol::VST:
    case PluginProtocol::VST3:
    case PluginProtocol::AU:
    case PluginProtocol::CLAP:
    case PluginProtocol::JSFX:
#ifdef HAVE_CARLA
      return CarlaNativePlugin::has_custom_ui (*this);
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

CarlaBridgeMode
PluginDescriptor::get_min_bridge_mode () const
{
  CarlaBridgeMode mode = CarlaBridgeMode::None;

  if (arch_ == PluginArchitecture::ARCH_32_BIT)
    {
      mode = CarlaBridgeMode::Full;
    }

  return mode;
}

bool
PluginDescriptor::is_whitelisted () const
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

  if (author_.empty ())
    {
      return false;
    }

  for (size_t i = 0; i < G_N_ELEMENTS (authors); i++)
    {
      if (author_ == authors[i])
        {
#if 0
          g_debug (
            "author '%s' is whitelisted", this->author);
#endif
          return true;
        }
    }

  return false;
}

std::string
PluginDescriptor::get_icon_name () const
{
  if (is_instrument ())
    {
      return "instrument";
    }
  else if (is_modulator ())
    {
      return "modulator";
    }
  else if (is_midi_modifier ())
    {
      return "signal-midi";
    }
  else if (is_effect ())
    {
      return "bars";
    }
  else
    {
      return "plug";
    }
}

GMenuModel *
PluginDescriptor::generate_context_menu () const
{
  GMenu * menu = g_menu_new ();

  GMenuItem * menuitem;
  char        tmp[600];

  /* TODO */
#if 0
  /* add option for native generic LV2 UI */
  if (this->protocol == PluginProtocol::LV2
      &&
      this->min_bridge_mode_ == CarlaBridgeMode::None)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (native generic UI)"),
          nullptr,
          "app.plugin-browser-add-to-project");
      g_menu_append_item (menu, menuitem);
    }
#endif

#ifdef HAVE_CARLA
  sprintf (tmp, "app.plugin-browser-add-to-project-carla::%p", this);
  menuitem = z_gtk_create_menu_item (_ ("Add to project"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);

  PluginSetting new_setting (*this);
  if (
    has_custom_ui_ && this->min_bridge_mode_ == CarlaBridgeMode::None
    && !new_setting.force_generic_ui_)
    {
      sprintf (
        tmp,
        "app.plugin-browser-add-to-project-"
        "bridged-ui::%p",
        this);
      menuitem = z_gtk_create_menu_item (
        _ ("Add to project (bridged UI)"), nullptr, tmp);
      g_menu_append_item (menu, menuitem);
    }

  sprintf (
    tmp,
    "app.plugin-browser-add-to-project-bridged-"
    "full::%p",
    this);
  menuitem =
    z_gtk_create_menu_item (_ ("Add to project (bridged full)"), nullptr, tmp);
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
    G_CALLBACK (on_use_generic_ui_toggled), this);
#endif

  /* add to collection */
  GMenu * add_collections_submenu = g_menu_new ();
  int     num_added = 0;
  for (auto &coll : PLUGIN_MANAGER->collections_->collections_)
    {
      if (coll.contains_descriptor (*this))
        {
          continue;
        }

      sprintf (tmp, "app.plugin-browser-add-to-collection::%p,%p", &coll, this);
      menuitem = z_gtk_create_menu_item (coll.name_.c_str (), nullptr, tmp);
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
  for (auto &coll : PLUGIN_MANAGER->collections_->collections_)
    {
      if (!coll.contains_descriptor (*this))
        {
          continue;
        }

      sprintf (
        tmp, "app.plugin-browser-remove-from-collection::%p,%p", &coll, this);
      menuitem = z_gtk_create_menu_item (coll.name_.c_str (), nullptr, tmp);
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

bool PluginDescriptor::is_same_plugin (const PluginDescriptor & other) const
{
  return *this == other;
}

void
PluginDescriptor::free_closure (void * data, GClosure * closure)
{
  PluginDescriptor * self = (PluginDescriptor *) data;
  delete self;
}
