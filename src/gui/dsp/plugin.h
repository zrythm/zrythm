// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_BASE_PLUGIN_H__
#define __PLUGINS_BASE_PLUGIN_H__

#include "zrythm-config.h"

#include <memory>
#include <string>
#include <vector>

#include "dsp/plugin_identifier.h"
#include "gui/backend/backend/settings/plugin_settings.h"
#include "gui/dsp/plugin_descriptor.h"
#include "gui/dsp/port.h"
#include "gui/dsp/port_span.h"
#include "utils/types.h"

namespace zrythm::gui
{
class Channel;
};

class Project;
class AutomationTrack;
// using ModulatorWidget = struct _ModulatorWidget;
class MidiPort;
class AudioPort;
class CVPort;
class ControlPort;
class AutomatableTrack;
// using WrappedObjectWithChangeSignal = struct _WrappedObjectWithChangeSignal;

namespace zrythm::gui::old_dsp::plugins
{

class Lv2Plugin;

/**
 * @addtogroup plugins
 *
 * @{
 */

constexpr auto PLUGIN_MAGIC = 43198683;

/**
 * This class provides the core functionality for managing a plugin, including
 * creating/initializing the plugin, connecting and disconnecting its ports,
 * activating and deactivating it, and managing its state and UI.
 *
 * The plugin can be of various types, such as LV2, Carla native, etc., and this
 * class provides a common interface for working with them.
 */
class Plugin
    : public utils::serialization::ISerializable<Plugin>,
      public dsp::IProcessable,
      public IPortOwner,
      public utils::UuidIdentifiableObject<Plugin>
{
public:
  using PluginIdentifier = dsp::PluginIdentifier;
  using PortIdentifier = dsp::PortIdentifier;
  using PluginSlot = dsp::PluginSlot;
  using PluginSlotType = dsp::PluginSlotType;
  using PluginSlotNo = PluginSlot::SlotNo;
  using Channel = gui::Channel;

  /**
   * Preset identifier.
   */
  struct PresetIdentifier
      : public zrythm::utils::serialization::ISerializable<PresetIdentifier>
  {
    // Rule of 0

    DECLARE_DEFINE_FIELDS_METHOD ();

    /** Index in bank, or -1 if this is used for a bank. */
    int idx_ = 0;

    /** Bank index in plugin. */
    int bank_idx_ = 0;

    /** Plugin identifier. */
    PluginUuid plugin_id_;
  };

  /**
   * Plugin preset.
   */
  struct Preset : public zrythm::utils::serialization::ISerializable<Preset>
  {
    DECLARE_DEFINE_FIELDS_METHOD ();

    /** Human readable name. */
    std::string name_;

    /** URI if LV2. */
    std::string uri_;

    /** Carla program index. */
    int carla_program_ = 0;

    PresetIdentifier id_;
  };

  /**
   * A plugin bank containing presets.
   *
   * If the plugin has no banks, there must be a default bank that will contain
   * all the presets.
   */
  struct Bank : public zrythm::utils::serialization::ISerializable<Bank>
  {
    // Rule of 0

    void add_preset (Preset &&preset);

    DECLARE_DEFINE_FIELDS_METHOD ();

    /** Presets in this bank. */
    std::vector<Preset> presets_;

    /** URI if LV2. */
    std::string uri_;

    /** Human readable name. */
    std::string name_;

    PresetIdentifier id_;
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

  static std::unique_ptr<Plugin>
  create_unique_from_hosting_type (PluginSetting::HostingType hosting_type);

  /**
   * @brief Factory method to create a plugin based on the setting.
   */
  static Plugin * create_with_setting (
    const PluginSetting &setting,
    TrackUuid            track_id,
    PluginSlot           slot);

  PluginDescriptor      &get_descriptor () { return *setting_->descr_; }
  std::string            get_name () const { return setting_->descr_->name_; }
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

  PortRegistrySpan get_input_port_span () const;
  PortRegistrySpan get_output_port_span () const;

  /**
   * @brief Initializes a plugin after deserialization.
   *
   * This may attempt to instantiate the plugin, which can throw an exception.
   *
   * @param track
   * @param ms
   *
   * @throw ZrythmException If an error occured during initialization.
   */
  void init_loaded (AutomatableTrack * track);

  bool is_in_active_project () const override;

  std::string
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
   * Adds an AutomationTrack to the Plugin.
   */
  void add_automation_track (AutomationTrack &at);

  /**
   * Sets the UI refresh rate on the Plugin.
   */
  void set_ui_refresh_rate ();

  /**
   * Gets the enable/disable port for this plugin.
   */
  ControlPort * get_enabled_port ();

  /**
   * Verifies that the plugin identifiers are valid.
   */
  bool validate () const;

  /**
   * Prints the plugin to string.
   */
  std::string print () const;

  /**
   * Removes the automation tracks associated with this plugin from the
   * automation tracklist in the corresponding track.
   *
   * Used e.g. when moving plugins.
   *
   * @param free_ats Also free the ats.
   */
  void remove_ats_from_automation_tracklist (bool free_ats, bool fire_events);

  std::string
  get_full_port_group_designation (const std::string &port_group) const;

  Port * get_port_in_group (const std::string &port_group, bool left) const;

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

  /**
   * Moves the plugin to the given slot in the given channel.
   *
   * If a plugin already exists, it deletes it and replaces it.
   *
   * @param confirm_overwrite Whether to show a dialog to confirm the
   * overwrite when a plugin already exists.
   */
  void move (
    AutomatableTrack * track,
    PluginSlot         slot,
    bool               confirm_overwrite,
    bool               fire_events);

  /**
   * Sets the channel and slot on the plugin and its ports.
   */
  void set_track_and_slot (TrackUuid track_id, PluginSlot slot);

  auto get_slot () const { return id_.slot_; }
  auto get_slot_type () const
  {
    const auto slot = id_.slot_;
    if (slot.has_slot_index ())
      {
        return slot.get_slot_with_index ().first;
      }

    return slot.get_slot_type_only ();
  }

  /**
   * @brief Moves the Plugin's automation from one Channel to another.
   */
  void move_automation (
    AutomatableTrack &prev_track,
    AutomatableTrack &track,
    PluginSlot        new_slot);

  /**
   * @brief Appends this plugin's ports to the given vector.
   */
  void append_ports (std::vector<Port *> &ports);

  /**
   * Exposes or unexposes plugin ports to the backend.
   *
   * @param expose Expose or not.
   * @param inputs Expose/unexpose inputs.
   * @param outputs Expose/unexpose outputs.
   */
  void
  expose_ports (AudioEngine &engine, bool expose, bool inputs, bool outputs);

  /**
   * Gets a port by its symbol.
   *
   * @note Only works on LV2 plugins.
   */
  std::optional<PortPtrVariant> get_port_by_symbol (const std::string &sym);

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
    const Plugin       &src,
    bool                is_backup,
    const std::string * abs_state_dir);

  /**
   * Returns the state dir as an absolute path.
   *
   * @param create_if_not_exists Whether to create the state dir if it does
   * not exist. If this is false, the function below is called internally.
   * FIXME refactor this into cleaner API.
   */
  std::string get_abs_state_dir (bool is_backup, bool create_if_not_exists);

  /**
   * @brief Simply gets the absolute state directory path, without attempting
   * to create it.
   */
  std::string get_abs_state_dir (bool is_backup) const
  {
    return get_abs_state_dir (state_dir_, is_backup);
    }

  /**
   * @brief Constructs the absolute path to the plugin state dir based on the
   * given relative path given.
   *
   * @param plugin_state_dir
   * @param is_backup
   * @return std::string
   */
  static std::string
  get_abs_state_dir (const std::string &plugin_state_dir, bool is_backup);

  /**
   * Ensures the state dir exists or creates it.
   *
   * @throw ZrythmException If the state dir could not be created or does not
   * exist.
   */
  void ensure_state_dir (bool is_backup);

  Channel * get_channel () const;

  AutomatableTrack * get_track () const;

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
   * Generates automatables for the plugin.
   *
   * The plugin must be instantiated already.
   *
   * @param track The Track this plugin belongs to. This is passed because the
   * track might not be in the project yet so we can't fetch it through indices.
   */
  void generate_automation_tracks (AutomatableTrack &track);

  /**
   * Prepare plugin for processing.
   */
  [[gnu::hot]] void prepare_process ();

  /**
   * Instantiates the plugin (e.g. when adding to a channel).
   *
   * @throw ZrymException if an error occurred.
   */
  void instantiate ();

  /**
   * Sets the track name hash on the plugin.
   */
  void change_track (TrackUuid track_id);

  /**
   * Process plugin.
   */
  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  std::string get_node_name () const override;

  std::string generate_window_title () const;

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

  void set_selected_preset_by_name (std::string_view name);

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
   * Connects the Plugin's output Port's to the input Port's of the given
   * Channel's prefader.
   *
   * Used when doing automatic connections.
   */
  void connect_to_prefader (Channel &ch);

  /**
   * Disconnect the automatic connections from the Plugin to the Channel's
   * prefader (if last Plugin).
   */
  void disconnect_from_prefader (Channel &ch);

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
  void add_in_port (const PortUuid &port_id);

  /**
   * Adds an out port to the plugin's list and returns a reference to it.
   */
  void add_out_port (const PortUuid &port_id);

  /** Adds a bank to the plugin's list and returns a reference to it. */
  Bank *
  add_bank_if_not_exists (const std::string * uri, std::string_view name);

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
  void init (TrackUuid track_id, PluginSlot slot);
  void set_as_port_owner_with_index (PortUuid port_id, size_t index);

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
  save_state (bool is_backup, const std::string * abs_state_dir) = 0;

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
   * @param track_name_hash The expected name hash of track the plugin will be
   * in.
   * @param slot The expected slot the plugin will be in.
   *
   * @throw ZrythmException If the plugin could not be created.
   */
  Plugin (
    PortRegistry        &port_registry,
    const PluginSetting &setting,
    TrackUuid            track_id,
    PluginSlot           slot);

  /**
   * Create a dummy plugin for tests.
   */
  Plugin (
    PortRegistry   &port_registry,
    ZPluginCategory cat,
    TrackUuid       track_id,
    PluginSlot      slot);

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  PluginIdentifier id_;

  /** Setting this plugin was instantiated with. */
  std::unique_ptr<PluginSetting> setting_;

  /** Ports coming in as input. */
  std::vector<PortUuid> in_ports_;

  /* Caches - avoid shared_ptr due to performance cost */
  std::vector<ControlPort *> ctrl_in_ports_;
  std::vector<AudioPort *>   audio_in_ports_;
  std::vector<CVPort *>      cv_in_ports_;
  std::vector<MidiPort *>    midi_in_ports_;

  /** Cache. */
  MidiPort * midi_in_port_ = nullptr;

  /** Outgoing ports. */
  std::vector<PortUuid> out_ports_;

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
  std::string state_dir_;

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

  int magic_ = PLUGIN_MAGIC;

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

  /** Pointer to owner track, if any. */
  AutomatableTrack * track_ = nullptr;

  std::optional<std::reference_wrapper<PortRegistry>> port_registry_;
};

inline bool
operator< (const Plugin &lhs, const Plugin &rhs)
{
  return lhs.id_ < rhs.id_;
}

class CarlaNativePlugin;

using PluginVariant = std::variant<CarlaNativePlugin>;
using PluginPtrVariant = to_pointer_variant<PluginVariant>;
using PluginUniquePtrVariant = to_unique_ptr_variant<PluginVariant>;
using PluginRegistry = utils::OwningObjectRegistry<PluginPtrVariant, Plugin>;
using PluginRegistryRef = std::reference_wrapper<PluginRegistry>;

} // namespace zrythm::gui::old_dsp::plugins

#endif
