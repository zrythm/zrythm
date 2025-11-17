// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor.h"
#include "utils/bidirectional_map.h"

namespace zrythm::plugins
{

std::unique_ptr<PluginDescriptor>
PluginDescriptor::from_juce_description (
  const juce::PluginDescription &juce_desc)
{
  auto descr = std::make_unique<PluginDescriptor> ();
  descr->name_ = utils::Utf8String::from_juce_string (juce_desc.name);
  descr->author_ =
    utils::Utf8String::from_juce_string (juce_desc.manufacturerName);
  descr->unique_id_ = juce_desc.uniqueId;
  descr->juce_compat_deprecated_unique_id_ = juce_desc.deprecatedUid;
  descr->protocol_ =
    Protocol::from_juce_format_name (juce_desc.pluginFormatName);

  {
    const auto tmp =
      utils::Utf8String::from_juce_string (juce_desc.fileOrIdentifier);
    switch (descr->protocol_)
      {
      case Protocol::ProtocolType::LV2:
      case Protocol::ProtocolType::AudioUnit:
        descr->path_or_id_ = tmp;
        break;
      case Protocol::ProtocolType::LADSPA:
      case Protocol::ProtocolType::VST:
      case Protocol::ProtocolType::VST3:
      case Protocol::ProtocolType::CLAP:
        descr->path_or_id_ = tmp.to_path ();
        break;
      default:
        break;
      }
  }
  if (juce_desc.isInstrument)
    {
      descr->category_ = PluginCategory::Instrument;
      descr->num_midi_ins_ = 1;
    }

  descr->num_audio_ins_ = juce_desc.numInputChannels;
  descr->num_audio_outs_ = juce_desc.numOutputChannels;

  return descr;
}

std::unique_ptr<juce::PluginDescription>
PluginDescriptor::to_juce_description () const
{
  auto juce_desc = std::make_unique<juce::PluginDescription> ();
  std::visit (
    [&] (auto &&val) {
      using T = std::decay_t<decltype (val)>;
      if constexpr (std::is_same_v<T, utils::Utf8String>)
        {
          juce_desc->fileOrIdentifier = val.to_juce_string ();
        }
      else if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
          juce_desc->fileOrIdentifier =
            utils::Utf8String::from_path (val).to_juce_string ();
        }
    },
    path_or_id_);
  juce_desc->pluginFormatName = Protocol::to_juce_format_name (protocol_);
  juce_desc->name = name_.to_juce_string ();
  juce_desc->manufacturerName = author_.to_juce_string ();
  juce_desc->category = category_str_.to_juce_string ();
  juce_desc->uniqueId = static_cast<int> (unique_id_);
  juce_desc->deprecatedUid = juce_compat_deprecated_unique_id_;
  return juce_desc;
}

QString
PluginDescriptor::serializeToString () const
{
  return utils::Utf8String::
    from_juce_string (to_juce_description ()->createXml ()->toString ())
      .to_qstring ();
}

QString
PluginDescriptor::getFormat () const
{
  return utils::Utf8String::
    from_juce_string (to_juce_description ()->pluginFormatName)
      .to_qstring ();
}

void
init_from (
  PluginDescriptor       &obj,
  const PluginDescriptor &other,
  utils::ObjectCloneType  clone_type)
{
  obj.author_ = other.author_;
  obj.name_ = other.name_;
  obj.website_ = other.website_;
  obj.category_ = other.category_;
  obj.category_str_ = other.category_str_;
  obj.protocol_ = other.protocol_;
  obj.num_audio_ins_ = other.num_audio_ins_;
  obj.num_audio_outs_ = other.num_audio_outs_;
  obj.num_midi_ins_ = other.num_midi_ins_;
  obj.num_midi_outs_ = other.num_midi_outs_;
  obj.num_ctrl_ins_ = other.num_ctrl_ins_;
  obj.num_ctrl_outs_ = other.num_ctrl_outs_;
  obj.num_cv_ins_ = other.num_cv_ins_;
  obj.num_cv_outs_ = other.num_cv_outs_;
  obj.arch_ = other.arch_;
  obj.protocol_ = other.protocol_;
  obj.path_or_id_ = other.path_or_id_;
  obj.unique_id_ = other.unique_id_;
  obj.min_bridge_mode_ = other.min_bridge_mode_;
  obj.has_custom_ui_ = other.has_custom_ui_;
}

bool
PluginDescriptor::is_instrument () const
{
  if (this->num_midi_ins_ == 0 || this->num_audio_outs_ == 0)
    {
      return false;
    }

  if (this->category_ == PluginCategory::Instrument)
    {
      return true;
    }

  return
    /* if VSTs are instruments their category must be INSTRUMENT, otherwise
       they are not */
    this->protocol_ != Protocol::ProtocolType::VST
    && this->category_ == PluginCategory::None && this->num_midi_ins_ > 0
    && this->num_audio_outs_ > 0;
}

bool
PluginDescriptor::is_effect () const
{
  constexpr std::array<PluginCategory, 37> effect_categories = {
    PluginCategory::Delay,
    PluginCategory::REVERB,
    PluginCategory::DISTORTION,
    PluginCategory::WAVESHAPER,
    PluginCategory::DYNAMICS,
    PluginCategory::AMPLIFIER,
    PluginCategory::COMPRESSOR,
    PluginCategory::ENVELOPE,
    PluginCategory::EXPANDER,
    PluginCategory::GATE,
    PluginCategory::LIMITER,
    PluginCategory::FILTER,
    PluginCategory::ALLPASS_FILTER,
    PluginCategory::BANDPASS_FILTER,
    PluginCategory::COMB_FILTER,
    PluginCategory::EQ,
    PluginCategory::MULTI_EQ,
    PluginCategory::PARA_EQ,
    PluginCategory::HIGHPASS_FILTER,
    PluginCategory::LOWPASS_FILTER,
    PluginCategory::GENERATOR,
    PluginCategory::CONSTANT,
    PluginCategory::OSCILLATOR,
    PluginCategory::MODULATOR,
    PluginCategory::CHORUS,
    PluginCategory::FLANGER,
    PluginCategory::PHASER,
    PluginCategory::SIMULATOR,
    PluginCategory::SIMULATOR_REVERB,
    PluginCategory::SPATIAL,
    PluginCategory::SPECTRAL,
    PluginCategory::PITCH,
    PluginCategory::UTILITY,
    PluginCategory::ANALYZER,
    PluginCategory::CONVERTER,
    PluginCategory::FUNCTION,
    PluginCategory::MIXER
  };
  return (category_ > PluginCategory::None
          && std::ranges::contains (effect_categories, category_))
         || (category_ == PluginCategory::None && num_audio_ins_ > 0 && num_audio_outs_ > 0);
}

bool
PluginDescriptor::is_modulator () const
{
  constexpr std::array<PluginCategory, 37> modulator_categories = {
    PluginCategory::ENVELOPE,  PluginCategory::GENERATOR,
    PluginCategory::CONSTANT,  PluginCategory::OSCILLATOR,
    PluginCategory::MODULATOR, PluginCategory::UTILITY,
    PluginCategory::CONVERTER, PluginCategory::FUNCTION,
  };
  return (category_ == PluginCategory::None
          || (category_ > PluginCategory::None && (std::ranges::contains (modulator_categories, category_))))
         && num_cv_outs_ > 0;
}

bool
PluginDescriptor::is_midi_modifier () const
{
  return (category_ > PluginCategory::None && category_ == PluginCategory::MIDI)
         || (category_ == PluginCategory::None && num_midi_ins_ > 0 && this->num_midi_outs_ > 0 && this->protocol_ != Protocol::ProtocolType::VST);
}

namespace
{
const utils::ConstBidirectionalMap<PluginCategory, std::string_view> category_map = {
  { PluginCategory::Delay,            "Delay"           },
  { PluginCategory::REVERB,           "Reverb"          },
  { PluginCategory::DISTORTION,       "Distortion"      },
  { PluginCategory::WAVESHAPER,       "Waveshaper"      },
  { PluginCategory::DYNAMICS,         "Dynamics"        },
  { PluginCategory::AMPLIFIER,        "Amplifier"       },
  { PluginCategory::COMPRESSOR,       "Compressor"      },
  { PluginCategory::ENVELOPE,         "Envelope"        },
  { PluginCategory::EXPANDER,         "Expander"        },
  { PluginCategory::GATE,             "Gate"            },
  { PluginCategory::LIMITER,          "Limiter"         },
  { PluginCategory::FILTER,           "Filter"          },
  { PluginCategory::ALLPASS_FILTER,   "Allpass"         },
  { PluginCategory::BANDPASS_FILTER,  "Bandpass"        },
  { PluginCategory::COMB_FILTER,      "Comb"            },
  { PluginCategory::EQ,               "Equaliser"       },
  { PluginCategory::MULTI_EQ,         "Multiband"       },
  { PluginCategory::PARA_EQ,          "Para"            },
  { PluginCategory::HIGHPASS_FILTER,  "Highpass"        },
  { PluginCategory::LOWPASS_FILTER,   "Lowpass"         },
  { PluginCategory::GENERATOR,        "Generator"       },
  { PluginCategory::CONSTANT,         "Constant"        },
  { PluginCategory::Instrument,       "Instrument"      },
  { PluginCategory::OSCILLATOR,       "Oscillator"      },
  { PluginCategory::MIDI,             "MIDI"            },
  { PluginCategory::MODULATOR,        "Modulator"       },
  { PluginCategory::CHORUS,           "Chorus"          },
  { PluginCategory::FLANGER,          "Flanger"         },
  { PluginCategory::PHASER,           "Phaser"          },
  { PluginCategory::SIMULATOR,        "Simulator"       },
  { PluginCategory::SIMULATOR_REVERB, "SimulatorReverb" },
  { PluginCategory::SPATIAL,          "Spatial"         },
  { PluginCategory::SPECTRAL,         "Spectral"        },
  { PluginCategory::PITCH,            "Pitch"           },
  { PluginCategory::UTILITY,          "Utility"         },
  { PluginCategory::ANALYZER,         "Analyzer"        },
  { PluginCategory::CONVERTER,        "Converter"       },
  { PluginCategory::FUNCTION,         "Function"        },
  { PluginCategory::MIXER,            "Mixer"           }
};
}

PluginCategory
PluginDescriptor::string_to_category (const utils::Utf8String &str)
{
  // Search through category_map
  const auto res = category_map.find_by_value (str.str ());
  if (res)
    {
      return *res;
    }

  // Special case for "Equalizer" spelling variant
  if (str.str ().find ("Equalizer") != std::string::npos)
    {
      return PluginCategory::EQ;
    }

  return PluginCategory::None;
}

utils::Utf8String
PluginDescriptor::category_to_string (PluginCategory category)
{
  const auto res = category_map.find_by_key (category);
  if (res)
    {
      return utils::Utf8String::from_utf8_encoded_string (std::string{ *res });
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

BridgeMode
PluginDescriptor::get_min_bridge_mode () const
{
  zrythm::plugins::BridgeMode mode = zrythm::plugins::BridgeMode::None;

  if (arch_ == PluginArchitecture::ARCH_32_BIT)
    {
      mode = zrythm::plugins::BridgeMode::Full;
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
      this->min_bridge_mode_ == zrythm::plugins::BridgeMode::None)
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

  PluginConfiguration new_setting (*this);
  if (
    has_custom_ui_
    && this->min_bridge_mode_ == zrythm::plugins::BridgeMode::None
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
} // namespace zrythm::plugins
