// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "plugins/plugin_descriptor.h"
#include "utils/bidirectional_map.h"
#include "utils/logger.h"
#include "utils/serialization.h"

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
      case Protocol::ProtocolType::Internal:
      case Protocol::ProtocolType::AudioUnit:
        descr->path_or_id_ = tmp;
        break;
      case Protocol::ProtocolType::LV2:
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
  descr->category_str_ =
    utils::Utf8String::from_juce_string (juce_desc.category);
  descr->descriptive_name_ =
    utils::Utf8String::from_juce_string (juce_desc.descriptiveName);
  descr->version_ = utils::Utf8String::from_juce_string (juce_desc.version);
  descr->last_file_mod_time_ms_ = juce_desc.lastFileModTime.toMilliseconds ();
  descr->last_info_update_time_ms_ =
    juce_desc.lastInfoUpdateTime.toMilliseconds ();

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
  juce_desc->descriptiveName = descriptive_name_.to_juce_string ();
  juce_desc->version = version_.to_juce_string ();
  juce_desc->lastFileModTime = juce::Time (last_file_mod_time_ms_);
  juce_desc->lastInfoUpdateTime = juce::Time (last_info_update_time_ms_);
  juce_desc->uniqueId = static_cast<int> (unique_id_);
  juce_desc->deprecatedUid = juce_compat_deprecated_unique_id_;
  juce_desc->isInstrument = category_ == PluginCategory::Instrument;
  juce_desc->numInputChannels = num_audio_ins_;
  juce_desc->numOutputChannels = num_audio_outs_;
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
PluginDescriptor::format () const
{
  // The Internal protocol is Zrythm's own plugin family; its user-facing
  // format label reads "Built-in"
  if (protocol_ == Protocol::ProtocolType::Internal)
    return QStringLiteral ("Built-in");
  return utils::Utf8String::
    from_juce_string (to_juce_description ()->pluginFormatName)
      .to_qstring ();
}

QString
PluginDescriptor::vendor () const
{
  return author_.to_qstring ();
}

QString
PluginDescriptor::pathOrId () const
{
  if (auto * path = std::get_if<std::filesystem::path> (&path_or_id_))
    {
      return utils::Utf8String::from_path (*path).to_qstring ();
    }
  return {};
}

utils::Utf8String
PluginDescriptor::source_identifier () const
{
  return std::visit (
    [] (const auto &value) {
      using T = std::decay_t<decltype (value)>;
      if constexpr (std::is_same_v<T, utils::Utf8String>)
        {
          return value;
        }
      else
        {
          return utils::Utf8String::from_path (value);
        }
    },
    path_or_id_);
}

QString
PluginDescriptor::category () const
{
  return category_str_.to_qstring ();
}

void
init_from (
  PluginDescriptor       &obj,
  const PluginDescriptor &other,
  utils::ObjectCloneType  clone_type)
{
  obj.author_ = other.author_;
  obj.name_ = other.name_;
  obj.descriptive_name_ = other.descriptive_name_;
  obj.website_ = other.website_;
  obj.category_ = other.category_;
  obj.category_str_ = other.category_str_;
  obj.version_ = other.version_;
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
  obj.juce_compat_deprecated_unique_id_ =
    other.juce_compat_deprecated_unique_id_;
  obj.last_file_mod_time_ms_ = other.last_file_mod_time_ms_;
  obj.last_info_update_time_ms_ = other.last_info_update_time_ms_;
  obj.has_custom_ui_ = other.has_custom_ui_;
}

bool
PluginDescriptor::isInstrument () const
{
  if (this->category_ == PluginCategory::Instrument)
    {
      return true;
    }

  if (this->num_midi_ins_ == 0 || this->num_audio_outs_ == 0)
    {
      return false;
    }

  // Uncategorized plugins that turn MIDI into audio without consuming any
  // audio input are instruments; effects with an extra MIDI input are not
  return this->protocol_ != Protocol::ProtocolType::VST
         && this->category_ == PluginCategory::None && this->num_midi_ins_ > 0
         && this->num_audio_ins_ == 0 && this->num_audio_outs_ > 0;
}

bool
PluginDescriptor::isEffect () const
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
PluginDescriptor::isModulator () const
{
  return num_cv_outs_ > 0;
}

bool
PluginDescriptor::isMidiModifier () const
{
  return num_midi_outs_ > 0;
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
      return false;
      break;
    default:
      return false;
      break;
    }

  z_return_val_if_reached (false);
}

utils::Utf8String
PluginDescriptor::get_icon_name () const
{
  if (isInstrument ())
    {
      return u8"instrument";
    }
  else if (isModulator ())
    {
      return u8"modulator";
    }
  else if (isMidiModifier ())
    {
      return u8"signal-midi";
    }
  else if (isEffect ())
    {
      return u8"bars";
    }
  else
    {
      return u8"plug";
    }
}

bool
PluginDescriptor::is_same_plugin (
  const zrythm::plugins::PluginDescriptor &other) const
{
  return *this == other;
}

void
to_json (nlohmann::json &j, const PluginDescriptor &p)
{
  j = nlohmann::json{
    { PluginDescriptor::kAuthorKey,             p.author_                   },
    { PluginDescriptor::kNameKey,               p.name_                     },
    { PluginDescriptor::kDescriptiveNameKey,    p.descriptive_name_         },
    { PluginDescriptor::kWebsiteKey,            p.website_                  },
    { PluginDescriptor::kCategoryKey,           p.category_                 },
    { PluginDescriptor::kCategoryStringKey,     p.category_str_             },
    { PluginDescriptor::kVersionKey,            p.version_                  },
    { PluginDescriptor::kNumAudioInsKey,        p.num_audio_ins_            },
    { PluginDescriptor::kNumAudioOutsKey,       p.num_audio_outs_           },
    { PluginDescriptor::kNumMidiInsKey,         p.num_midi_ins_             },
    { PluginDescriptor::kNumMidiOutsKey,        p.num_midi_outs_            },
    { PluginDescriptor::kNumCtrlInsKey,         p.num_ctrl_ins_             },
    { PluginDescriptor::kNumCtrlOutsKey,        p.num_ctrl_outs_            },
    { PluginDescriptor::kNumCvInsKey,           p.num_cv_ins_               },
    { PluginDescriptor::kNumCvOutsKey,          p.num_cv_outs_              },
    { PluginDescriptor::kUniqueIdKey,           p.unique_id_                },
    { PluginDescriptor::kDeprecatedUniqueIdKey,
     p.juce_compat_deprecated_unique_id_                                    },
    { PluginDescriptor::kArchitectureKey,       p.arch_                     },
    { PluginDescriptor::kProtocolKey,           p.protocol_                 },
    { PluginDescriptor::kPathOrIdKey,           p.path_or_id_               },
    { PluginDescriptor::kHasCustomUIKey,        p.has_custom_ui_            },
    { PluginDescriptor::kLastFileModTimeKey,    p.last_file_mod_time_ms_    },
    { PluginDescriptor::kLastInfoUpdateTimeKey, p.last_info_update_time_ms_ },
  };
}

void
from_json (const nlohmann::json &j, PluginDescriptor &p)
{
  j.at (PluginDescriptor::kNameKey).get_to (p.name_);
  p.author_ = j.value (PluginDescriptor::kAuthorKey, utils::Utf8String{});
  p.descriptive_name_ =
    j.value (PluginDescriptor::kDescriptiveNameKey, utils::Utf8String{});
  p.website_ = j.value (PluginDescriptor::kWebsiteKey, utils::Utf8String{});
  p.category_ = j.value (PluginDescriptor::kCategoryKey, PluginCategory::None);
  p.category_str_ =
    j.value (PluginDescriptor::kCategoryStringKey, utils::Utf8String{});
  p.version_ = j.value (PluginDescriptor::kVersionKey, utils::Utf8String{});
  p.num_audio_ins_ = j.value (PluginDescriptor::kNumAudioInsKey, 0);
  p.num_audio_outs_ = j.value (PluginDescriptor::kNumAudioOutsKey, 0);
  p.num_midi_ins_ = j.value (PluginDescriptor::kNumMidiInsKey, 0);
  p.num_midi_outs_ = j.value (PluginDescriptor::kNumMidiOutsKey, 0);
  p.num_ctrl_ins_ = j.value (PluginDescriptor::kNumCtrlInsKey, 0);
  p.num_ctrl_outs_ = j.value (PluginDescriptor::kNumCtrlOutsKey, 0);
  p.num_cv_ins_ = j.value (PluginDescriptor::kNumCvInsKey, 0);
  p.num_cv_outs_ = j.value (PluginDescriptor::kNumCvOutsKey, 0);
  j.at (PluginDescriptor::kUniqueIdKey).get_to (p.unique_id_);
  p.juce_compat_deprecated_unique_id_ =
    j.value (PluginDescriptor::kDeprecatedUniqueIdKey, 0);
  p.arch_ = j.value (
    PluginDescriptor::kArchitectureKey, PluginArchitecture::ARCH_64_BIT);
  j.at (PluginDescriptor::kProtocolKey).get_to (p.protocol_);
  {
    const auto &val = j.at (PluginDescriptor::kPathOrIdKey);
    if (val.at (zrythm::utils::serialization::kVariantTypeKey) == 0)
      {
        p.path_or_id_ =
          val.at (utils::serialization::kVariantNonObjectValueKey)
            .get<std::filesystem::path> ();
      }
    else
      {
        p.path_or_id_ =
          val.at (utils::serialization::kVariantNonObjectValueKey)
            .get<utils::Utf8String> ();
      }
  }
  p.has_custom_ui_ = j.value (PluginDescriptor::kHasCustomUIKey, false);
  p.last_file_mod_time_ms_ =
    j.value (PluginDescriptor::kLastFileModTimeKey, std::int64_t{ 0 });
  p.last_info_update_time_ms_ =
    j.value (PluginDescriptor::kLastInfoUpdateTimeKey, int64_t{ 0 });
}
} // namespace zrythm::plugins
