// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_PLUGIN_DESCRIPTOR_H__
#define __PLUGINS_PLUGIN_DESCRIPTOR_H__

#include "zrythm-config.h"

#include "dsp/plugin_identifier.h"
#include "gui/dsp/plugin_protocol.h"
#include "utils/icloneable.h"
#include "utils/iserializable.h"

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
class PluginDescriptor final
    : public QObject,
      public zrythm::utils::serialization::ISerializable<PluginDescriptor>,
      public ICloneable<PluginDescriptor>
{
  Q_OBJECT

  Q_PROPERTY (QString name READ getName CONSTANT FINAL)

public:
  static ZPluginCategory string_to_category (const std::string &str);
  static std::string     category_to_string (ZPluginCategory category);

  // static void free_closure (void * data, GClosure * closure);

  DECLARE_DEFINE_FIELDS_METHOD ();

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
  std::string get_icon_name () const;

  // GMenuModel * generate_context_menu () const;

  [[nodiscard]] QString getName () const
  {
    return QString::fromStdString (name_);
  }

  void
  init_after_cloning (const PluginDescriptor &other, ObjectCloneType clone_type)
    override;

public:
  std::string     author_;
  std::string     name_;
  std::string     website_;
  ZPluginCategory category_ = ZPluginCategory::NONE;
  /** Lv2 plugin subcategory. */
  std::string category_str_;
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
  std::string uri_;

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

#endif
