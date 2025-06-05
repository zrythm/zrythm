// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <memory>
#include <string>
#include <vector>

#include "gui/dsp/port.h"
#include "gui/dsp/port_span.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_slot.h"
#include "structure/tracks/track_fwd.h"
#include "utils/types.h"

namespace zrythm::engine::device_io
{
class AudioEngine;
}

namespace zrythm::gui::old_dsp::plugins
{

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * This class provides the core functionality for managing a plugin, including
 * creating/initializing the plugin, connecting and disconnecting its ports,
 * activating and deactivating it, and managing its state and UI.
 *
 * The plugin can be of various types, such as LV2, Carla native, etc., and this
 * class provides a common interface for working with them.
 */
class Plugin
    : public dsp::graph::IProcessable,
      public IPortOwner,
      public utils::UuidIdentifiableObject<Plugin>
{
  Z_DISABLE_COPY_MOVE (Plugin)
public:
  using PortIdentifier = dsp::PortIdentifier;
  using PluginSlot = zrythm::plugins::PluginSlot;
  using PluginSlotType = zrythm::plugins::PluginSlotType;
  using PluginSlotNo = PluginSlot::SlotNo;
  using PluginDescriptor = zrythm::plugins::PluginDescriptor;
  using Protocol = zrythm::plugins::Protocol;
  using PluginCategory = zrythm::plugins::PluginCategory;
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;
  using TrackPtrVariant = zrythm::structure::tracks::TrackPtrVariant;
  using TrackResolver = zrythm::structure::tracks::TrackResolver;

  /**
   * Preset identifier.
   */
  struct PresetIdentifier
  {
    // Rule of 0

    /** Index in bank, or -1 if this is used for a bank. */
    int idx_ = 0;

    /** Bank index in plugin. */
    int bank_idx_ = 0;

    /** Plugin identifier. */
    PluginUuid plugin_id_;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE (PresetIdentifier, idx_, bank_idx_, plugin_id_)
  };

  /**
   * Plugin preset.
   */
  struct Preset
  {
    /** Human readable name. */
    utils::Utf8String name_;

    /** URI if LV2. */
    utils::Utf8String uri_;

    /** Carla program index. */
    int carla_program_ = 0;

    PresetIdentifier id_;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE (Preset, name_, uri_, carla_program_, id_)
  };

  /**
   * A plugin bank containing presets.
   *
   * If the plugin has no banks, there must be a default bank that will contain
   * all the presets.
   */
  struct Bank
  {
    // Rule of 0

    void add_preset (Preset &&preset);

    /** Presets in this bank. */
    std::vector<Preset> presets_;

    /** URI if LV2. */
    utils::Utf8String uri_;

    /** Human readable name. */
    utils::Utf8String name_;

    PresetIdentifier id_;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE (Bank, name_, presets_, uri_, id_)
  };

  constexpr static auto DEFAULT_BANK_URI = "https://lv2.zrythm.org#default-bank";
  constexpr static auto INIT_PRESET_URI = "https://lv2.zrythm.org#init-preset";

  /**
   * UI refresh rate limits.
   */
  constexpr static float MIN_REFRESH_RATE = 30.f;
  constexpr static float MAX_REFRESH_RATE = 121.f;

  /**
   * UI scale factor limits.
   */
  constexpr static float MIN_SCALE_FACTOR = 0.5f;
  constexpr static float MAX_SCALE_FACTOR = 4.f;

public:
  ~Plugin () override;

  PluginDescriptor      &get_descriptor () { return *setting_->descr_; }
  utils::Utf8String      get_name () const { return setting_->descr_->name_; }
  Protocol::ProtocolType get_protocol () const
  {
    return setting_->descr_->protocol_;
  }

  static Plugin * from_variant (const auto &variant)
  {
    return std::visit ([&] (auto &&pl) -> Plugin * { return pl; }, variant);
  }

  static auto name_projection (const auto &var)
  {
    return std::visit ([&] (auto &&val) { return val->get_name (); }, var);
  }

  auto get_input_port_span () const { return PortSpan{ in_ports_ }; }
  auto get_output_port_span () const { return PortSpan{ out_ports_ }; }

  /**
   * @brief Initializes a plugin after deserialization.
   *
   * This may attempt to instantiate the plugin, which can throw an exception.
   *
   * @throw ZrythmException If an error occured during initialization.
   */
  void init_loaded ();

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  void on_control_change_event (
    const dsp::PortIdentifier::PortUuid &port_uuid,
    const dsp::PortIdentifier           &id,
    float                                val) override;

  bool should_bounce_to_master (utils::audio::BounceStep step) const override;

  /** Whether the plugin is used for MIDI auditioning in SampleProcessor. */
  bool is_auditioner () const;

  /**
   * Sets the UI refresh rate on the Plugin.
   */
  void set_ui_refresh_rate ();

  /**
   * Gets the enable/disable port for this plugin.
   */
  ControlPort * get_enabled_port ();

  /**
   * Removes the automation tracks associated with this plugin from the
   * automation tracklist in the corresponding track.
   *
   * Used e.g. when moving plugins.
   *
   * @param free_ats Also free the ats.
   */
  void remove_ats_from_automation_tracklist (bool free_ats, bool fire_events);

  utils::Utf8String
  get_full_port_group_designation (const utils::Utf8String &port_group) const;

  Port *
  get_port_in_group (const utils::Utf8String &port_group, bool left) const;

  /**
   * @brief Finds the corresponding port in the same port group (eg, if this
   * is left, find right and vice versa).
   */
  Port * get_port_in_same_group (const Port &port);

  /**
   * Activates or deactivates the plugin.
   *
   * @param activate True to activate, false to deactivate.
   * @throw ZrythmException If an error occured during activation.
   */
  void activate (bool activate = true);

  auto get_slot_type () const
  {
    const auto slot = get_slot ();
    assert (slot.has_value ());
    if (slot->has_slot_index ())
      {
        return slot->get_slot_with_index ().first;
      }

    return slot->get_slot_type_only ();
  }

  /**
   * @brief Appends this plugin's ports to the given vector.
   */
  void append_ports (std::vector<Port *> &ports);

  /**
   * Gets a port by its symbol.
   *
   * @note Only works on LV2 plugins.
   */
  std::optional<PortPtrVariant>
  get_port_by_symbol (const utils::Utf8String &sym);

  /**
   * @brief Copies the state directory from the given source plugin to this
   * plugin's state directory.
   *
   * @param is_backup Whether this is a backup project. Used for calculating
   * the absolute path to the state dir.
   * @param abs_state_dir If passed, the state will be saved inside this
   * directory instead of the plugin's state directory. Used when saving
   * presets.
   *
   * @throw ZrythmException On error.
   */
  void copy_state_dir (
    const Plugin           &src,
    bool                    is_backup,
    std::optional<fs::path> abs_state_dir);

  /**
   * Returns the state dir as an absolute path.
   *
   * @param create_if_not_exists Whether to create the state dir if it does
   * not exist. If this is false, the function below is called internally.
   * FIXME refactor this into cleaner API.
   */
  fs::path get_abs_state_dir (bool is_backup, bool create_if_not_exists);

  /**
   * @brief Simply gets the absolute state directory path, without attempting
   * to create it.
   */
  fs::path get_abs_state_dir (bool is_backup) const
  {
    return get_abs_state_dir (state_dir_, is_backup);
  }

  /**
   * @brief Constructs the absolute path to the plugin state dir based on the
   * given relative path given.
   */
  static fs::path
  get_abs_state_dir (const fs::path &plugin_state_dir, bool is_backup);

  /**
   * Ensures the state dir exists or creates it.
   *
   * @throw ZrythmException If the state dir could not be created or does not
   * exist.
   */
  void ensure_state_dir (bool is_backup);

  std::optional<TrackPtrVariant> get_track () const;

  void set_track (const TrackUuid &track_id)
  {
    track_id_ = track_id;
    update_identifier ();
  }

  TrackUuid get_track_id () const
  {
    assert (has_track ());
    return *track_id_;
  }

  bool has_track () const { return track_id_.has_value (); }

  /**
   * To be called when changes to the plugin identifier were made, so we can
   * update all children recursively.
   */
  void update_identifier ();

  /**
   * Updates the plugin's latency.
   *
   * Calls the plugin format's get_latency()
   * function and stores the result in the plugin.
   */
  // void update_latency ();

  /**
   * Prepare plugin for processing.
   */
  [[gnu::hot]] void prepare_process (std::size_t block_length);

  /**
   * Instantiates the plugin (e.g. when adding to a channel).
   *
   * @throw ZrymException if an error occurred.
   */
  void instantiate ();

  /**
   * @brief Returns the slot this number is inserted at in the owner.
   *
   * @return PluginSlot
   */
  auto get_slot () const -> std::optional<PluginSlot>;

  /**
   * Process plugin.
   */
  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  utils::Utf8String get_node_name () const override;

  utils::Utf8String generate_window_title () const;

  /**
   * Process show ui
   */
  void open_ui ();

  bool is_selected () const { return selected_; }

  void set_selected (bool selected) { selected_ = selected; }

#if 0
  /**
   * Selects the plugin in the MixerSelections.
   *
   * @param select Select or deselect.
   * @param exclusive Whether to make this the only selected plugin or add it
   * to the selections.
   */
  void select (bool select, bool exclusive);
#endif

  /**
   * Returns whether the plugin is enabled.
   *
   * @param check_track Whether to check if the track
   *   is enabled as well.
   */
  bool is_enabled (bool check_track) const;

  void set_enabled (bool enabled, bool fire_events);

  /**
   * Processes the plugin by passing through the input to its output.
   *
   * This is called when the plugin is bypassed.
   */
  [[gnu::hot]] void process_passthrough (EngineProcessTimeInfo time_nfo);

  /**
   * Process hide ui
   */
  void close_ui ();

  /**
   * (re)Generates automatables for the plugin.
   */
  void update_automatables ();

  void set_selected_bank_from_index (int idx);

  void set_selected_preset_from_index (int idx);

  void set_selected_preset_by_name (const utils::Utf8String &name);

  /**
   * Sets caches for processing.
   */
  void set_caches ();

  /**
   * Connect the output Ports of the given source Plugin to the input Ports of
   * the given destination Plugin.
   *
   * Used when automatically connecting a Plugin in the Channel strip to the
   * next Plugin.
   */
  void connect_to_plugin (Plugin &dest);

  /**
   * Disconnect the automatic connections from the given source Plugin to the
   * given destination Plugin.
   */
  void disconnect_from_plugin (Plugin &dest);

  /**
   * To be called immediately when a channel or plugin
   * is deleted.
   *
   * A call to plugin_free can be made at any point
   * later just to free the resources.
   */
  void disconnect ();

  /**
   * Deletes any state files associated with this plugin.
   *
   * This should be called when a plugin instance is removed from the project
   * (including undo stacks) to remove any files not needed anymore.
   */
  void delete_state_files ();

  /**
   * Cleans up resources of an instantiated but not activated plugin.
   *
   * @note This cleans up resources of the underlying plugin format only. Ports
   * and other host-related resources are free'd in the destructor.
   */
  void cleanup ();

protected:
  /**
   * Adds an in port to the plugin's list and returns a reference to it.
   */
  void add_in_port (const PortUuidReference &port_id);

  /**
   * Adds an out port to the plugin's list and returns a reference to it.
   */
  void add_out_port (const PortUuidReference &port_id);

  /** Adds a bank to the plugin's list and returns a reference to it. */
  Bank * add_bank_if_not_exists (
    std::optional<utils::Utf8String> uri,
    const utils::Utf8String         &name);

  /**
   * @brief
   *
   * @param other
   * @note @p other is not const because we will attempt to save its state.
   * @throw ZrymException if an error occurred.
   */
  void copy_members_from (Plugin &other);

protected:
  Plugin () = default;

private:
  void set_stereo_outs_and_midi_in ();
  void set_enabled_and_gain ();
  void init ();
  void set_port_index (PortUuidReference port_id);

  virtual void populate_banks () = 0;

  /**
   * @brief Called internally by @ref set_selected_preset_from_index.
   */
  virtual void set_selected_preset_from_index_impl (int idx) = 0;

  virtual void cleanup_impl () = 0;

  /**
   * @brief Called by @ref instantiate().
   *
   * @param loading Whether loading an existing plugin or not.
   * @param use_state_file Whether to use the plugin's state file to
   * instantiate the plugin.
   */
  virtual void instantiate_impl (bool loading, bool use_state_file) = 0;

  /**
   * Saves the state inside the standard state directory.
   *
   * @param is_backup Whether this is a backup project. Used for calculating
   * the absolute path to the state dir.
   * @param abs_state_dir If passed, the state will be savedinside this
   * directory instead of the plugin's state directory. Used when saving
   * presets.
   *
   * @throw ZrythmException If the state could not be saved.
   */
  virtual void
  save_state (bool is_backup, std::optional<fs::path> abs_state_dir) = 0;

  /**
   * @brief Opens or closes a custom non-generic UI.
   *
   */
  virtual void open_custom_ui (bool show) = 0;

  virtual void activate_impl (bool activate = true) = 0;

  virtual void process_impl (EngineProcessTimeInfo time_info) = 0;

  /**
   * @brief Deactivates, cleans up and frees underlying plugin resources.
   *
   * FIXME what's the difference between this and @ref cleanup_impl()?
   */
  virtual void close () = 0;

protected:
  /**
   * Creates/initializes a plugin and its internal plugin (LV2, etc.) using
   * the given setting.
   *
   * @throw ZrythmException If the plugin could not be created.
   */
  Plugin (PortRegistry &port_registry, const PluginConfiguration &setting);

private:
  static constexpr auto kTrackIdKey = "trackId"sv;
  static constexpr auto kSettingKey = "setting"sv;
  static constexpr auto kInPortsKey = "inPorts"sv;
  static constexpr auto kOutPortsKey = "outPorts"sv;
  static constexpr auto kBanksKey = "banks"sv;
  static constexpr auto kSelectedBankKey = "selectedBank"sv;
  static constexpr auto kSelectedPresetKey = "selectedPreset"sv;
  static constexpr auto kVisibleKey = "visible"sv;
  static constexpr auto kStateDirectoryKey = "stateDir"sv;
  friend void           to_json (nlohmann::json &j, const Plugin &p)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (p));
    j[kTrackIdKey] = p.track_id_;
    j[kSettingKey] = p.setting_;
    j[kInPortsKey] = p.in_ports_;
    j[kOutPortsKey] = p.out_ports_;
    j[kBanksKey] = p.banks_;
    j[kSelectedBankKey] = p.selected_bank_;
    j[kSelectedPresetKey] = p.selected_preset_;
    j[kVisibleKey] = p.visible_;
    j[kStateDirectoryKey] = p.state_dir_;
  }
  friend void from_json (const nlohmann::json &j, Plugin &p)
  {
    from_json (j, static_cast<UuidIdentifiableObject &> (p));
    j.at (kTrackIdKey).get_to (p.track_id_);
    j.at (kSettingKey).get_to (p.setting_);
    j.at (kInPortsKey).get_to (p.in_ports_);
    j.at (kOutPortsKey).get_to (p.out_ports_);
    j.at (kBanksKey).get_to (p.banks_);
    j.at (kSelectedBankKey).get_to (p.selected_bank_);
    j.at (kSelectedPresetKey).get_to (p.selected_preset_);
    j.at (kVisibleKey).get_to (p.visible_);
    j.at (kStateDirectoryKey).get_to (p.state_dir_);
  }

public:
  OptionalRef<PortRegistry>    port_registry_;
  std::optional<TrackResolver> track_resolver_;

  std::optional<TrackUuid> track_id_;

  /** Setting this plugin was instantiated with. */
  std::unique_ptr<PluginConfiguration> setting_;

  /** Ports coming in as input. */
  std::vector<PortUuidReference> in_ports_;

  /* Caches - avoid shared_ptr due to performance cost */
  std::vector<ControlPort *> ctrl_in_ports_;
  std::vector<AudioPort *>   audio_in_ports_;
  std::vector<CVPort *>      cv_in_ports_;
  std::vector<MidiPort *>    midi_in_ports_;

  /** Cache. */
  MidiPort * midi_in_port_ = nullptr;

  /** Outgoing ports. */
  std::vector<PortUuidReference> out_ports_;

  /**
   * Control for plugin enabled, for convenience.
   *
   * This port is already in @ref Plugin.in_ports.
   */
  ControlPort * enabled_ = nullptr;

  /**
   * Whether the plugin has a custom "enabled" port (LV2).
   *
   * If true, bypass logic will be delegated to the plugin.
   */
  ControlPort * own_enabled_port_ = nullptr;

  /**
   * Control for plugin gain, for convenience.
   *
   * This port is already in @ref Plugin.in_ports.
   */
  ControlPort * gain_ = nullptr;

  /**
   * Instrument left stereo output, for convenience.
   *
   * This port is already in @ref Plugin.out_ports if instrument.
   */
  AudioPort * l_out_ = nullptr;
  AudioPort * r_out_ = nullptr;

  std::vector<Bank> banks_;

  PresetIdentifier selected_bank_;
  PresetIdentifier selected_preset_;

  /** Whether plugin UI is opened or not. */
  bool visible_ = false;

  /** Latency reported by the Lv2Plugin, if any, in samples. */
  // nframes_t latency_ = 0;

  /** Whether the plugin is currently instantiated or not. */
  bool instantiated_{};

  /** Set to true if instantiation failed and the plugin will be treated as
   * disabled. */
  bool instantiation_failed_ = false;

  /** Whether the plugin is currently activated or not. */
  bool activated_ = false;

  /** Update frequency of the UI, in Hz (times per second). */
  float ui_update_hz_ = 0.f;

  /** Scale factor for drawing UIs in scale of the monitor. */
  float ui_scale_factor_ = 0.f;

  /**
   * @brief State directory (only basename).
   *
   * Used for saving/loading the state.
   *
   * @note This is only the directory basename and should go in
   * project/plugins/states.
   */
  fs::path state_dir_;

  /** Whether the plugin is currently being deleted. */
  bool deleting_ = false;

  /**
   * ID of the destroy signal for @ref Plugin.window so that we can deactivate
   * before freeing the plugin.
   */
  ulong destroy_window_id_ = 0;

  /**
   * ID of the close-request signal for @ref Plugin.window so that we can
   * deactivate before freeing the plugin.
   */
  ulong close_request_id_ = 0;

  /**
   * Whether selected in the slot owner (mixer for example).
   */
  bool selected_{};

  /**
   * ID of GSource (if > 0).
   *
   * @see update_plugin_ui().
   */
  uint update_ui_source_id_ = 0;

  /** Temporary variable to check if plugin is currently undergoing
   * deactivation. */
  bool deactivating_ = false;

  /**
   * Set to true to avoid sending multiple ET_PLUGIN_STATE_CHANGED for the
   * same plugin.
   */
  std::atomic<bool> state_changed_event_sent_ = false;

  /** Whether the plugin is used for functions. */
  bool is_function_ = false;
};

inline bool
operator< (const Plugin &lhs, const Plugin &rhs)
{
  return lhs.get_slot () < rhs.get_slot ();
}

class CarlaNativePlugin;

using PluginVariant = std::variant<CarlaNativePlugin>;
using PluginPtrVariant = to_pointer_variant<PluginVariant>;
using PluginUniquePtrVariant = to_unique_ptr_variant<PluginVariant>;
using PluginRegistry = utils::OwningObjectRegistry<PluginPtrVariant, Plugin>;
using PluginRegistryRef = std::reference_wrapper<PluginRegistry>;
using PluginUuidReference = utils::UuidReference<PluginRegistry>;

} // namespace zrythm::gui::old_dsp::plugins

void
from_json (
  const nlohmann::json                  &j,
  gui::old_dsp::plugins::PluginRegistry &registry);
