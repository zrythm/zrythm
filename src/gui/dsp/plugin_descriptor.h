// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/plugin_slot.h"
#include "gui/dsp/plugin_protocol.h"
#include "utils/icloneable.h"
#include "utils/serialization.h"

#include <QObject>

/**
 * @addtogroup plugins
 *
 * @{
 */

namespace zrythm::gui::old_dsp::plugins
{

using PluginSlotType = zrythm::dsp::PluginSlotType;

/**
 * Plugin category.
 */
enum class ZPluginCategory
{
  /** None specified. */
  NONE,
  DELAY,
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
  INSTRUMENT,
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
enum class PluginArchitecture
{
  ARCH_32_BIT,
  ARCH_64_BIT
};

/**
 * Carla bridge mode.
 */
enum class CarlaBridgeMode
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
class PluginDescriptor final : public QObject, public ICloneable<PluginDescriptor>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (QString name READ getName CONSTANT FINAL)

public:
  static ZPluginCategory   string_to_category (const utils::Utf8String &str);
  static utils::Utf8String category_to_string (ZPluginCategory category);

  bool is_instrument () const;
  bool is_effect () const;
  bool is_modulator () const;
  bool is_midi_modifier () const;

  /**
   * Returns if this can be dropped in a slot of the given type.
   */
  bool is_valid_for_slot_type (PluginSlotType slot_type, int track_type) const;

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
  CarlaBridgeMode get_min_bridge_mode () const;

  /**
   * Returns whether the plugin is known to work, so it should be whitelisted.
   *
   * Non-whitelisted plugins will run in full bridge mode. This is to prevent
   * crashes when Zrythm is not at fault.
   *
   * These must all be free-software plugins so that they can be debugged if
   * issues arise.
   */
  bool is_whitelisted () const;

  /**
   * Gets an appropriate icon name.
   */
  utils::Utf8String get_icon_name () const;

  // GMenuModel * generate_context_menu () const;

  [[nodiscard]] QString getName () const { return name_.to_qstring (); }

  void
  init_after_cloning (const PluginDescriptor &other, ObjectCloneType clone_type)
    override;

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
  static constexpr auto kArchitectureKey = "architecture"sv;
  static constexpr auto kProtocolKey = "protocol"sv;
  static constexpr auto kPathKey = "path"sv;
  static constexpr auto kUriKey = "uri"sv;
  static constexpr auto kMinBridgeModeKey = "minBridgeMode"sv;
  static constexpr auto kHasCustomUIKey = "hasCustomUI"sv;
  static constexpr auto kGHashKey = "ghash"sv;
  static constexpr auto kSha1Key = "sha1"sv;
  friend void           to_json (nlohmann::json &j, const PluginDescriptor &p)
  {
    j = nlohmann::json{
      { kAuthorKey,         p.author_          },
      { kNameKey,           p.name_            },
      { kWebsiteKey,        p.website_         },
      { kCategoryKey,       p.category_        },
      { kCategoryStringKey, p.category_str_    },
      { kNumAudioInsKey,    p.num_audio_ins_   },
      { kNumAudioOutsKey,   p.num_audio_outs_  },
      { kNumMidiInsKey,     p.num_midi_ins_    },
      { kNumMidiOutsKey,    p.num_midi_outs_   },
      { kNumCtrlInsKey,     p.num_ctrl_ins_    },
      { kNumCtrlOutsKey,    p.num_ctrl_outs_   },
      { kNumCvInsKey,       p.num_cv_ins_      },
      { kNumCvOutsKey,      p.num_cv_outs_     },
      { kUniqueIdKey,       p.unique_id_       },
      { kArchitectureKey,   p.arch_            },
      { kProtocolKey,       p.protocol_        },
      { kPathKey,           p.path_            },
      { kUriKey,            p.uri_             },
      { kMinBridgeModeKey,  p.min_bridge_mode_ },
      { kHasCustomUIKey,    p.has_custom_ui_   },
      { kGHashKey,          p.ghash_           },
      { kSha1Key,           p.sha1_            },
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
    j.at (kPathKey).get_to (p.path_);
    j.at (kUriKey).get_to (p.uri_);
    j.at (kMinBridgeModeKey).get_to (p.min_bridge_mode_);
    j.at (kHasCustomUIKey).get_to (p.has_custom_ui_);
    j.at (kGHashKey).get_to (p.ghash_);
    j.at (kSha1Key).get_to (p.sha1_);
  }

public:
  utils::Utf8String author_;
  utils::Utf8String name_;
  utils::Utf8String website_;
  ZPluginCategory   category_ = ZPluginCategory::NONE;
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
  PluginArchitecture arch_ = PluginArchitecture::ARCH_32_BIT;
  /** Plugin protocol (Lv2/DSSI/LADSPA/VST/etc.). */
  Protocol::ProtocolType protocol_ = Protocol::ProtocolType::Internal;
  /** Path, if not an Lv2Plugin which uses URIs. */
  fs::path path_;
  /** Lv2Plugin URI. */
  utils::Utf8String uri_;

  /** Used for VST. */
  int64_t unique_id_ = 0;

  /** Minimum required bridge mode. */
  CarlaBridgeMode min_bridge_mode_ = CarlaBridgeMode::None;

  bool has_custom_ui_ = false;

  /**
   * Hash of the plugin's bundle (.so/.ddl for VST) used when caching
   * PluginDescriptor's, obtained using g_file_hash().
   *
   * @deprecated Kept so that older projects still work.
   */
  unsigned int ghash_ = 0;

  /** SHA1 of the file (replaces ghash). */
  std::string sha1_;
};

inline bool
operator== (const PluginDescriptor &a, const PluginDescriptor &b)
{
  return a.arch_ == b.arch_ && a.protocol_ == b.protocol_
         && a.unique_id_ == b.unique_id_ && a.ghash_ == b.ghash_
         && a.sha1_ == b.sha1_ && a.uri_ == b.uri_;
}

} // namespace zrythm::gui::old_dsp::plugins

/**
 * @}
 */
