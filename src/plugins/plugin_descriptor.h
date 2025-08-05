// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "plugins/plugin_protocol.h"
#include "utils/icloneable.h"

#include <QObject>

#include <boost/describe.hpp>

/**
 * @addtogroup plugins
 *
 * @{
 */

namespace zrythm::plugins
{

/**
 * Plugin category.
 */
enum class PluginCategory : std::uint_fast8_t
{
  /** None specified. */
  None,
  Delay,
  REVERB,
  DISTORTION,
  WAVESHAPER,
  DYNAMICS,
  AMPLIFIER,
  COMPRESSOR,
  ENVELOPE,
  EXPANDER,
  GATE,
  LIMITER,
  FILTER,
  ALLPASS_FILTER,
  BANDPASS_FILTER,
  COMB_FILTER,
  EQ,
  MULTI_EQ,
  PARA_EQ,
  HIGHPASS_FILTER,
  LOWPASS_FILTER,
  GENERATOR,
  CONSTANT,
  Instrument,
  OSCILLATOR,
  MIDI,
  MODULATOR,
  CHORUS,
  FLANGER,
  PHASER,
  SIMULATOR,
  SIMULATOR_REVERB,
  SPATIAL,
  SPECTRAL,
  PITCH,
  UTILITY,
  ANALYZER,
  CONVERTER,
  FUNCTION,
  MIXER,
};

/**
 * 32 or 64 bit.
 */
enum class PluginArchitecture : std::uint_fast8_t
{
  ARCH_32_BIT,
  ARCH_64_BIT
};

/**
 * Plugin bridge mode.
 */
enum class BridgeMode : std::uint_fast8_t
{
  None,
  UI,
  Full,
};

/**
 * The PluginDescriptor class provides a set of static utility functions and
 * member functions to work with plugin descriptors. It contains information
 * about a plugin such as its name, author, category, protocol, and various port
 * counts.
 *
 * The static utility functions are used to convert between different
 * plugin-related enumerations and strings, as well as to determine if a plugin
 * protocol is supported.
 *
 * The member functions provide information about the plugin, such as whether it
 * is an instrument, effect, modulator, or MIDI modifier, whether it is valid
 * for a given slot type and track type, whether it has a custom UI, the minimum
 * required bridge mode, and whether it is whitelisted.
 *
 * The PluginDescriptor also contains various fields that store the plugin's
 * metadata, such as its author, name, website, category, port counts,
 * architecture, protocol, and unique ID (for VST plugins).
 */
class PluginDescriptor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (QString name READ getName CONSTANT FINAL)
  Q_PROPERTY (QString format READ getFormat CONSTANT FINAL)
  QML_UNCREATABLE ("")

public:
  static std::unique_ptr<PluginDescriptor>
  from_juce_description (const juce::PluginDescription &juce_desc);

  std::unique_ptr<juce::PluginDescription> to_juce_description () const;

  static PluginCategory    string_to_category (const utils::Utf8String &str);
  static utils::Utf8String category_to_string (PluginCategory category);

  bool is_instrument () const;
  bool is_effect () const;
  bool is_modulator () const;
  bool is_midi_modifier () const;

  /**
   * Returns whether the two descriptors describe the same plugin, ignoring
   * irrelevant fields.
   */
  bool is_same_plugin (const PluginDescriptor &other) const;

  /**
   * Returns if the Plugin has a supported custom UI.
   */
  bool has_custom_ui () const;

  /**
   * Returns the minimum bridge mode required for this plugin.
   */
  BridgeMode get_min_bridge_mode () const;

  /**
   * Gets an appropriate icon name.
   */
  utils::Utf8String get_icon_name () const;

  // GMenuModel * generate_context_menu () const;

  [[nodiscard]] QString getName () const { return name_.to_qstring (); }
  QString               getFormat () const;

  friend void init_from (
    PluginDescriptor       &obj,
    const PluginDescriptor &other,
    utils::ObjectCloneType  clone_type);

private:
  static constexpr auto kAuthorKey = "author"sv;
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kWebsiteKey = "website"sv;
  static constexpr auto kCategoryKey = "category"sv;
  static constexpr auto kCategoryStringKey = "categoryString"sv;
  static constexpr auto kNumAudioInsKey = "numAudioIns"sv;
  static constexpr auto kNumAudioOutsKey = "numAudioOuts"sv;
  static constexpr auto kNumMidiInsKey = "numMidiIns"sv;
  static constexpr auto kNumMidiOutsKey = "numMidiOuts"sv;
  static constexpr auto kNumCtrlInsKey = "numCtrlIns"sv;
  static constexpr auto kNumCtrlOutsKey = "numCtrlOuts"sv;
  static constexpr auto kNumCvInsKey = "numCvIns"sv;
  static constexpr auto kNumCvOutsKey = "numCvOuts"sv;
  static constexpr auto kUniqueIdKey = "uniqueId"sv;
  static constexpr auto kDeprecatedUniqueIdKey = "deprecatedUniqueId"sv;
  static constexpr auto kArchitectureKey = "architecture"sv;
  static constexpr auto kProtocolKey = "protocol"sv;
  static constexpr auto kPathOrIdKey = "pathOrId"sv;
  static constexpr auto kMinBridgeModeKey = "minBridgeMode"sv;
  static constexpr auto kHasCustomUIKey = "hasCustomUI"sv;
  friend void           to_json (nlohmann::json &j, const PluginDescriptor &p)
  {
    j = nlohmann::json{
      { kAuthorKey,             p.author_                           },
      { kNameKey,               p.name_                             },
      { kWebsiteKey,            p.website_                          },
      { kCategoryKey,           p.category_                         },
      { kCategoryStringKey,     p.category_str_                     },
      { kNumAudioInsKey,        p.num_audio_ins_                    },
      { kNumAudioOutsKey,       p.num_audio_outs_                   },
      { kNumMidiInsKey,         p.num_midi_ins_                     },
      { kNumMidiOutsKey,        p.num_midi_outs_                    },
      { kNumCtrlInsKey,         p.num_ctrl_ins_                     },
      { kNumCtrlOutsKey,        p.num_ctrl_outs_                    },
      { kNumCvInsKey,           p.num_cv_ins_                       },
      { kNumCvOutsKey,          p.num_cv_outs_                      },
      { kUniqueIdKey,           p.unique_id_                        },
      { kDeprecatedUniqueIdKey, p.juce_compat_deprecated_unique_id_ },
      { kArchitectureKey,       p.arch_                             },
      { kProtocolKey,           p.protocol_                         },
      { kPathOrIdKey,           p.path_or_id_                       },
      { kMinBridgeModeKey,      p.min_bridge_mode_                  },
      { kHasCustomUIKey,        p.has_custom_ui_                    },
    };
  }
  friend void from_json (const nlohmann::json &j, PluginDescriptor &p)
  {
    j.at (kAuthorKey).get_to (p.author_);
    j.at (kNameKey).get_to (p.name_);
    j.at (kWebsiteKey).get_to (p.website_);
    j.at (kCategoryKey).get_to (p.category_);
    j.at (kCategoryStringKey).get_to (p.category_str_);
    j.at (kNumAudioInsKey).get_to (p.num_audio_ins_);
    j.at (kNumAudioOutsKey).get_to (p.num_audio_outs_);
    j.at (kNumMidiInsKey).get_to (p.num_midi_ins_);
    j.at (kNumMidiOutsKey).get_to (p.num_midi_outs_);
    j.at (kNumCtrlInsKey).get_to (p.num_ctrl_ins_);
    j.at (kNumCtrlOutsKey).get_to (p.num_ctrl_outs_);
    j.at (kNumCvInsKey).get_to (p.num_cv_ins_);
    j.at (kNumCvOutsKey).get_to (p.num_cv_outs_);
    j.at (kUniqueIdKey).get_to (p.unique_id_);
    j.at (kArchitectureKey).get_to (p.arch_);
    j.at (kProtocolKey).get_to (p.protocol_);
    {
      const auto val = j.at (kPathOrIdKey);
      if (val[zrythm::utils::serialization::kVariantIndexKey] == 0)
        {
          p.path_or_id_ =
            val[zrythm::utils::serialization::kVariantValueKey].get<fs::path> ();
        }
      else
        {
          p.path_or_id_ =
            val[zrythm::utils::serialization::kVariantValueKey]
              .get<utils::Utf8String> ();
        }
    }
    j.at (kMinBridgeModeKey).get_to (p.min_bridge_mode_);
    j.at (kHasCustomUIKey).get_to (p.has_custom_ui_);
  }

  friend bool operator== (const PluginDescriptor &a, const PluginDescriptor &b)
  {
    constexpr auto tie = [] (const auto &p) {
      return std::tie (
        p.arch_, p.protocol_, p.path_or_id_, p.unique_id_,
        p.juce_compat_deprecated_unique_id_, p.min_bridge_mode_,
        p.has_custom_ui_);
    };
    return tie (a) == tie (b);
  }

public:
  utils::Utf8String author_;
  utils::Utf8String name_;
  utils::Utf8String website_;
  PluginCategory    category_ = PluginCategory::None;
  /** Lv2 plugin subcategory. */
  utils::Utf8String category_str_;
  /** Number of audio input ports. */
  int num_audio_ins_ = 0;
  /** Number of MIDI input ports. */
  int num_midi_ins_ = 0;
  /** Number of audio output ports. */
  int num_audio_outs_ = 0;
  /** Number of MIDI output ports. */
  int num_midi_outs_ = 0;
  /** Number of input control (plugin param) ports. */
  int num_ctrl_ins_ = 0;
  /** Number of output control (plugin param) ports. */
  int num_ctrl_outs_ = 0;
  /** Number of input CV ports. */
  int num_cv_ins_ = 0;
  /** Number of output CV ports. */
  int num_cv_outs_ = 0;
  /** Architecture (32/64bit). */
  PluginArchitecture arch_ = PluginArchitecture::ARCH_64_BIT;
  /** Plugin protocol (Lv2/DSSI/LADSPA/VST/etc.). */
  Protocol::ProtocolType protocol_ = Protocol::ProtocolType::Internal;

  /**
   * @brief Identifier, for plugin formats that use unique identifiers, or path
   * otherwise.
   */
  std::variant<fs::path, utils::Utf8String> path_or_id_;

  /** Used by some plugin formats to uniquely identify the plugin. */
  int64_t unique_id_{};

  /** This is additionally needed by JUCE for some plugin formats. */
  int juce_compat_deprecated_unique_id_{};

  /** Minimum required bridge mode. */
  BridgeMode min_bridge_mode_ = BridgeMode::None;

  bool has_custom_ui_{};

  BOOST_DESCRIBE_CLASS (
    PluginDescriptor,
    (),
    (author_,
     name_,
     website_,
     category_,
     category_str_,
     protocol_,
     path_or_id_,
     unique_id_,
     juce_compat_deprecated_unique_id_,
     num_cv_ins_,
     num_cv_outs_,
     arch_,
     min_bridge_mode_,
     has_custom_ui_),
    (),
    ())
};

} // namespace zrythm::plugins

namespace std
{
template <> struct hash<zrythm::plugins::PluginDescriptor>
{
  size_t operator() (const zrythm::plugins::PluginDescriptor &d) const
  {
    size_t h{};
    h = h ^ qHash (d.protocol_);
    h = h ^ qHash (d.arch_);
    if (std::holds_alternative<fs::path> (d.path_or_id_))
      {
        h = h ^ qHash (std::get<fs::path> (d.path_or_id_).string ());
      }
    else
      {
        h = h ^ qHash (std::get<utils::Utf8String> (d.path_or_id_).str ());
      }
    h = h ^ qHash (d.unique_id_);
    h = h ^ qHash (d.juce_compat_deprecated_unique_id_);
    return h;
  }
};
}

/**
 * @}
 */
