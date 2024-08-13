// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Code related to Carla plugins.
 */

#include "zrythm-config.h"

#include <utility>
#include <vector>

#include "plugins/plugin.h"
#include "utils/icloneable.h"
#include "utils/types.h"

#ifndef __PLUGINS_CARLA_NATIVE_PLUGIN_H__
#  define __PLUGINS_CARLA_NATIVE_PLUGIN_H__

#  ifdef HAVE_CARLA
#    include "carla_wrapper.h"
#  endif

class PluginDescriptor;
class Port;
using CarlaPluginHandle = void *;

/**
 * @addtogroup plugins
 *
 * @{
 */

constexpr const char * CARLA_STATE_FILENAME = "state.carla";

/**
 * The type of the Carla plugin.
 */
enum class CarlaPluginType
{
  CARLA_PLUGIN_NONE,
  CARLA_PLUGIN_RACK,
  CARLA_PLUGIN_PATCHBAY,
  CARLA_PLUGIN_PATCHBAY16,
  CARLA_PLUGIN_PATCHBAY32,
  CARLA_PLUGIN_PATCHBAY64,
};

struct CarlaPatchbayPortInfo
{
  CarlaPatchbayPortInfo () = default;
  CarlaPatchbayPortInfo (
    unsigned int _plugin_id,
    unsigned int _port_hints,
    unsigned int _port_id,
    std::string  _port_name)
      : plugin_id (_plugin_id), port_hints (_port_hints), port_id (_port_id),
        port_name (std::move (_port_name))
  {
  }
  unsigned int plugin_id;
  unsigned int port_hints;
  unsigned int port_id;
  std::string  port_name;
};

class CarlaNativePlugin final
    : public Plugin,
      public ICloneable<CarlaNativePlugin>,
      public ISerializable<CarlaNativePlugin>
{
public:
  CarlaNativePlugin () = default;
  /**
   * Creates/initializes a plugin using the given setting.
   *
   * @param track_name_hash The expected name hash of track the plugin will be
   * in.
   * @param slot The expected slot the plugin will be in.
   *
   * @throw ZrythmException If the plugin could not be created.
   */
  CarlaNativePlugin (
    const PluginSetting &setting,
    unsigned int         track_name_hash,
    PluginSlotType       slot_type,
    int                  slot);

  /**
   * Deactivates, cleanups and frees the instance.
   */
  ~CarlaNativePlugin ();

  /**
   * Returns a filled in descriptor from @p info.
   */
  static std::unique_ptr<PluginDescriptor>
  get_descriptor_from_cached (const CarlaCachedPluginInfo &info, PluginType type);

  void save_state (bool is_backup, const std::string * abs_state_dir) override;

  /**
   * Loads the state from the given file or from
   * its state file.
   *
   * @throw ZrythmException if failed to load state.
   */
  void load_state (const std::string * abs_path);

  void update_buffer_size_and_sample_rate ();

  /**
   * Shows or hides the UI.
   */
  void open_custom_ui (bool show) override;

  /**
   * Returns the plugin Port corresponding to the given parameter.
   */
  ControlPort * get_port_from_param_id (uint32_t id);

  /**
   * Returns the MIDI out port.
   */
  MidiPort * get_midi_out_port ();

  float get_param_value (uint32_t id);

  /**
   * Called from port_set_control_value() to send the value to carla.
   *
   * @param val Real value (ie, not normalized).
   */

  void set_param_value (const uint32_t id, float val);

  void close () override;

  static bool has_custom_ui (const PluginDescriptor &descr);

  /**
   * Returns the latency in samples.
   */

  nframes_t get_latency () const override;

  /**
   * @brief Adds the internal plugin from the given descriptor.
   *
   * @param descr
   * @return Whether added.
   */
  bool add_internal_plugin_from_descr (const PluginDescriptor &descr);

  void init_after_cloning (const CarlaNativePlugin &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void populate_banks () override;

  void set_selected_preset_from_index_impl (int idx) override;

  void activate_impl (bool activate) override;

  /**
   * Instantiates the plugin.
   *
   * @param loading Whether loading an existing plugin
   *   or not.
   * @param use_state_file Whether to use the plugin's
   *   state file to instantiate the plugin.
   *
   * @throw ZrythmException if failed to instantiate.
   */
  void instantiate_impl (bool loading, bool use_state_file) override;

  /**
   * Processes the plugin for this cycle.
   */
  HOT OPTIMIZE_O3 void
  process_impl (const EngineProcessTimeInfo time_nfo) override;

  void cleanup_impl () override;

  /**
   * @brief Idle callback for the plugin UI.
   */
  bool idle_cb ();

  void create_ports (bool loading);

public:
#  ifdef HAVE_CARLA
  NativePluginHandle             native_plugin_handle_ = nullptr;
  NativeHostDescriptor           native_host_descriptor_ = {};
  const NativePluginDescriptor * native_plugin_descriptor_ = nullptr;

  CarlaHostHandle host_handle_ = nullptr;

  NativeTimeInfo time_info_ = {};
#  endif

  /** Plugin ID inside carla engine. */
  unsigned int carla_plugin_id_ = 0;

  /** Whether ports are already created or not. */
  bool ports_created_ = false;

  /** Flag. */
  bool loading_state_ = false;

  /** Port ID of first audio input (for connecting inside patchbay). */
  unsigned int audio_input_port_id_ = 0;
  /** Port ID of first audio output (for connecting inside patchbay). */
  unsigned int audio_output_port_id_ = 0;

  /** Port ID of first cv input (for connecting inside patchbay). */
  unsigned int cv_input_port_id_ = 0;
  /** Port ID of first cv output (for connecting inside patchbay). */
  unsigned int cv_output_port_id_ = 0;

  /** Port ID of first midi input (for connecting inside patchbay). */
  unsigned int midi_input_port_id_ = 0;
  /** Port ID of first midi output (for connecting inside patchbay). */
  unsigned int midi_output_port_id_ = 0;

  /** Used when connecting Carla's internal plugin to patchbay ports. */
  std::vector<CarlaPatchbayPortInfo> patchbay_port_info_;

  /**
   * @brief Idle callback for the plugin UI.
   *
   * do not use tick callback:
   * falktx: I am doing some checks on ildaeil/carla, and see there is
   * a nice way without conflicts to avoid the GL context issues. it
   * came from cardinal, where I cannot draw plugin UIs in the same
   * function as the main stuff, because it is in between other opengl
   * calls (before and after). the solution I found was to have a
   * dedicated idle timer, and handle the plugin UI stuff there,
   * outside of the main application draw function
   */
  sigc::scoped_connection idle_connection_;

  /**
   * Used during processing.
   *
   * Must be resized on buffer size change.
   */
  std::vector<std::vector<float>> zero_inbufs_;
  std::vector<std::vector<float>> zero_outbufs_;
  std::vector<const float *>      inbufs_;
  std::vector<float *>            outbufs_;

  unsigned int max_variant_audio_ins_ = 0;
  unsigned int max_variant_audio_outs_ = 0;
  unsigned int max_variant_cv_ins_ = 0;
  unsigned int max_variant_cv_outs_ = 0;
  unsigned int max_variant_midi_ins_ = 0;
  unsigned int max_variant_midi_outs_ = 0;
};

/**
 * @}
 */

#endif
