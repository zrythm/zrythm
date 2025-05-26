// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor.h"

using namespace zrythm::plugins;

std::unique_ptr<PluginDescriptor>
PluginDescriptor::from_juce_description (
  const juce::PluginDescription &juce_desc)
{
  auto descr = std::make_unique<PluginDescriptor> ();
  descr->name_ = utils::Utf8String::from_juce_string (juce_desc.name);
  descr->author_ =
    utils::Utf8String::from_juce_string (juce_desc.manufacturerName);
  return descr;
}

void
PluginDescriptor::init_after_cloning (
  const PluginDescriptor &other,
  ObjectCloneType         clone_type)
{
  author_ = other.author_;
  name_ = other.name_;
  website_ = other.website_;
  category_ = other.category_;
  category_str_ = other.category_str_;
  protocol_ = other.protocol_;
  num_audio_ins_ = other.num_audio_ins_;
  num_audio_outs_ = other.num_audio_outs_;
  num_midi_ins_ = other.num_midi_ins_;
  num_midi_outs_ = other.num_midi_outs_;
  num_ctrl_ins_ = other.num_ctrl_ins_;
  num_ctrl_outs_ = other.num_ctrl_outs_;
  num_cv_ins_ = other.num_cv_ins_;
  num_cv_outs_ = other.num_cv_outs_;
  arch_ = other.arch_;
  protocol_ = other.protocol_;
  path_ = other.path_;
  uri_ = other.uri_;
  unique_id_ = other.unique_id_;
  min_bridge_mode_ = other.min_bridge_mode_;
  has_custom_ui_ = other.has_custom_ui_;
  sha1_ = other.sha1_;
}

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

  return
    /* if VSTs are instruments their category must be INSTRUMENT, otherwise
       they are not */
    this->protocol_ != Protocol::ProtocolType::VST
    && this->category_ == ZPluginCategory::NONE && this->num_midi_ins_ > 0
    && this->num_audio_outs_ > 0;
}

bool
PluginDescriptor::is_effect () const
{
  constexpr std::array<ZPluginCategory, 37> effect_categories = {
    ZPluginCategory::DELAY,
    ZPluginCategory::REVERB,
    ZPluginCategory::DISTORTION,
    ZPluginCategory::WAVESHAPER,
    ZPluginCategory::DYNAMICS,
    ZPluginCategory::AMPLIFIER,
    ZPluginCategory::COMPRESSOR,
    ZPluginCategory::ENVELOPE,
    ZPluginCategory::EXPANDER,
    ZPluginCategory::GATE,
    ZPluginCategory::LIMITER,
    ZPluginCategory::FILTER,
    ZPluginCategory::ALLPASS_FILTER,
    ZPluginCategory::BANDPASS_FILTER,
    ZPluginCategory::COMB_FILTER,
    ZPluginCategory::EQ,
    ZPluginCategory::MULTI_EQ,
    ZPluginCategory::PARA_EQ,
    ZPluginCategory::HIGHPASS_FILTER,
    ZPluginCategory::LOWPASS_FILTER,
    ZPluginCategory::GENERATOR,
    ZPluginCategory::CONSTANT,
    ZPluginCategory::OSCILLATOR,
    ZPluginCategory::MODULATOR,
    ZPluginCategory::CHORUS,
    ZPluginCategory::FLANGER,
    ZPluginCategory::PHASER,
    ZPluginCategory::SIMULATOR,
    ZPluginCategory::SIMULATOR_REVERB,
    ZPluginCategory::SPATIAL,
    ZPluginCategory::SPECTRAL,
    ZPluginCategory::PITCH,
    ZPluginCategory::UTILITY,
    ZPluginCategory::ANALYZER,
    ZPluginCategory::CONVERTER,
    ZPluginCategory::FUNCTION,
    ZPluginCategory::MIXER
  };
  return (category_ > ZPluginCategory::NONE
          && std::ranges::contains (effect_categories, category_))
         || (category_ == ZPluginCategory::NONE && num_audio_ins_ > 0 && num_audio_outs_ > 0);
}

bool
PluginDescriptor::is_modulator () const
{
  constexpr std::array<ZPluginCategory, 37> modulator_categories = {
    ZPluginCategory::ENVELOPE,  ZPluginCategory::GENERATOR,
    ZPluginCategory::CONSTANT,  ZPluginCategory::OSCILLATOR,
    ZPluginCategory::MODULATOR, ZPluginCategory::UTILITY,
    ZPluginCategory::CONVERTER, ZPluginCategory::FUNCTION,
  };
  return (category_ == ZPluginCategory::NONE
          || (category_ > ZPluginCategory::NONE && (std::ranges::contains (modulator_categories, category_))))
         && num_cv_outs_ > 0;
}

bool
PluginDescriptor::is_midi_modifier () const
{
  return (category_ > ZPluginCategory::NONE && category_ == ZPluginCategory::MIDI)
         || (category_ == ZPluginCategory::NONE && num_midi_ins_ > 0 && this->num_midi_outs_ > 0 && this->protocol_ != Protocol::ProtocolType::VST);
}

static const std::unordered_map<ZPluginCategory, std::string_view> category_map = {
  { ZPluginCategory::DELAY,            "Delay"           },
  { ZPluginCategory::REVERB,           "Reverb"          },
  { ZPluginCategory::DISTORTION,       "Distortion"      },
  { ZPluginCategory::WAVESHAPER,       "Waveshaper"      },
  { ZPluginCategory::DYNAMICS,         "Dynamics"        },
  { ZPluginCategory::AMPLIFIER,        "Amplifier"       },
  { ZPluginCategory::COMPRESSOR,       "Compressor"      },
  { ZPluginCategory::ENVELOPE,         "Envelope"        },
  { ZPluginCategory::EXPANDER,         "Expander"        },
  { ZPluginCategory::GATE,             "Gate"            },
  { ZPluginCategory::LIMITER,          "Limiter"         },
  { ZPluginCategory::FILTER,           "Filter"          },
  { ZPluginCategory::ALLPASS_FILTER,   "Allpass"         },
  { ZPluginCategory::BANDPASS_FILTER,  "Bandpass"        },
  { ZPluginCategory::COMB_FILTER,      "Comb"            },
  { ZPluginCategory::EQ,               "Equaliser"       },
  { ZPluginCategory::MULTI_EQ,         "Multiband"       },
  { ZPluginCategory::PARA_EQ,          "Para"            },
  { ZPluginCategory::HIGHPASS_FILTER,  "Highpass"        },
  { ZPluginCategory::LOWPASS_FILTER,   "Lowpass"         },
  { ZPluginCategory::GENERATOR,        "Generator"       },
  { ZPluginCategory::CONSTANT,         "Constant"        },
  { ZPluginCategory::INSTRUMENT,       "Instrument"      },
  { ZPluginCategory::OSCILLATOR,       "Oscillator"      },
  { ZPluginCategory::MIDI,             "MIDI"            },
  { ZPluginCategory::MODULATOR,        "Modulator"       },
  { ZPluginCategory::CHORUS,           "Chorus"          },
  { ZPluginCategory::FLANGER,          "Flanger"         },
  { ZPluginCategory::PHASER,           "Phaser"          },
  { ZPluginCategory::SIMULATOR,        "Simulator"       },
  { ZPluginCategory::SIMULATOR_REVERB, "SimulatorReverb" },
  { ZPluginCategory::SPATIAL,          "Spatial"         },
  { ZPluginCategory::SPECTRAL,         "Spectral"        },
  { ZPluginCategory::PITCH,            "Pitch"           },
  { ZPluginCategory::UTILITY,          "Utility"         },
  { ZPluginCategory::ANALYZER,         "Analyzer"        },
  { ZPluginCategory::CONVERTER,        "Converter"       },
  { ZPluginCategory::FUNCTION,         "Function"        },
  { ZPluginCategory::MIXER,            "Mixer"           }
};

ZPluginCategory
PluginDescriptor::string_to_category (const utils::Utf8String &str)
{
  // Search through category_map
  for (const auto &[category, name] : category_map)
    {
      if (str.str ().find (std::string{ name }) != std::string::npos)
        {
          return category;
        }
    }

  // Special case for "Equalizer" spelling variant
  if (str.str ().find ("Equalizer") != std::string::npos)
    {
      return ZPluginCategory::EQ;
    }

  return ZPluginCategory::NONE;
}

utils::Utf8String
PluginDescriptor::category_to_string (ZPluginCategory category)
{
  if (auto it = category_map.find (category); it != category_map.end ())
    {
      return utils::Utf8String::from_utf8_encoded_string (std::string{
        it->second });
    }

  return u8"Plugin";
}

bool
PluginDescriptor::has_custom_ui () const
{
  switch (protocol_)
    {
    case Protocol::ProtocolType::LV2:
    case Protocol::ProtocolType::VST:
    case Protocol::ProtocolType::VST3:
    case Protocol::ProtocolType::AudioUnit:
    case Protocol::ProtocolType::CLAP:
    case Protocol::ProtocolType::JSFX:
#if HAVE_CARLA
      return zrythm::gui::old_dsp::plugins::CarlaNativePlugin::has_custom_ui (
        *this);
#else
      return false;
#endif
      break;
    default:
      return false;
      break;
    }

  z_return_val_if_reached (false);
}

CarlaBridgeMode
PluginDescriptor::get_min_bridge_mode () const
{
  zrythm::plugins::CarlaBridgeMode mode = zrythm::plugins::CarlaBridgeMode::None;

  if (arch_ == PluginArchitecture::ARCH_32_BIT)
    {
      mode = zrythm::plugins::CarlaBridgeMode::Full;
    }

  return mode;
}

utils::Utf8String
PluginDescriptor::get_icon_name () const
{
  if (is_instrument ())
    {
      return u8"instrument";
    }
  else if (is_modulator ())
    {
      return u8"modulator";
    }
  else if (is_midi_modifier ())
    {
      return u8"signal-midi";
    }
  else if (is_effect ())
    {
      return u8"bars";
    }
  else
    {
      return u8"plug";
    }
}

#if 0
GMenuModel *
PluginDescriptor::generate_context_menu () const
{
  GMenu * menu = g_menu_new ();

  GMenuItem * menuitem;
  char        tmp[600];

  /* TODO */
#  if 0
  /* add option for native generic LV2 UI */
  if (this->protocol == ProtocolType::LV2
      &&
      this->min_bridge_mode_ == zrythm::plugins::CarlaBridgeMode::None)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (native generic UI)"),
          nullptr,
          "app.plugin-browser-add-to-project");
      g_menu_append_item (menu, menuitem);
    }
#  endif

#  if HAVE_CARLA
  sprintf (tmp, "app.plugin-browser-add-to-project-carla::%p", this);
  menuitem = z_gtk_create_menu_item (QObject::tr ("Add to project"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);

  PluginSetting new_setting (*this);
  if (
    has_custom_ui_
    && this->min_bridge_mode_ == zrythm::plugins::CarlaBridgeMode::None
    && !new_setting.force_generic_ui_)
    {
      sprintf (
        tmp,
        "app.plugin-browser-add-to-project-"
        "bridged-ui::%p",
        this);
      menuitem = z_gtk_create_menu_item (
        QObject::tr ("Add to project (bridged UI)"), nullptr, tmp);
      g_menu_append_item (menu, menuitem);
    }

  sprintf (
    tmp,
    "app.plugin-browser-add-to-project-bridged-"
    "full::%p",
    this);
  menuitem =
    z_gtk_create_menu_item (QObject::tr ("Add to project (bridged full)"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);
#  endif

#  if 0
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
#  endif

  /* add to collection */
  GMenu * add_collections_submenu = g_menu_new ();
  int     num_added = 0;
  for (auto &coll : zrythm::gui::old_dsp::plugins::PluginManager::get_active_instance ()->collections_->collections_)
    {
      if (coll->contains_descriptor (*this))
        {
          continue;
        }

      sprintf (tmp, "app.plugin-browser-add-to-collection::%p,%p", &coll, this);
      menuitem = z_gtk_create_menu_item (coll->name_.c_str (), nullptr, tmp);
      g_menu_append_item (add_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, QObject::tr ("Add to collection"), G_MENU_MODEL (add_collections_submenu));
    }
  else
    {
      g_object_unref (add_collections_submenu);
    }

  /* remove from collection */
  GMenu * remove_collections_submenu = g_menu_new ();
  num_added = 0;
  for (auto &coll : zrythm::gui::old_dsp::plugins::PluginManager::get_active_instance ()->collections_->collections_)
    {
      if (!coll->contains_descriptor (*this))
        {
          continue;
        }

      sprintf (
        tmp, "app.plugin-browser-remove-from-collection::%p,%p", &coll, this);
      menuitem = z_gtk_create_menu_item (coll->name_.c_str (), nullptr, tmp);
      g_menu_append_item (remove_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, QObject::tr ("Remove from collection"),
        G_MENU_MODEL (remove_collections_submenu));
    }
  else
    {
      g_object_unref (remove_collections_submenu);
    }

  return G_MENU_MODEL (menu);
}
#endif

bool
PluginDescriptor::is_same_plugin (
  const zrythm::plugins::PluginDescriptor &other) const
{
  return *this == other;
}
