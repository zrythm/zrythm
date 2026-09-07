// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/std.h>

#include "dsp/audio_bus_configuration.h"
#include "dsp/midi_event.h"
#include "plugins/lv2_discovery.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2_urid_map.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_transport_context.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"
#include "utils/serialization.h"
#include "utils/views.h"

#include <fmt/format.h>
#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/state/state.h>
#include <lv2/time/time.h>
#include <lv2/units/units.h>
#include <lv2/urid/urid.h>

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::plugins
{

namespace
{

/** Capacity of the atom buffers handed to the plugin, in bytes. */
constexpr uint32_t kAtomBufferSize = 16384;

/**
 * Upper bound for atom input buffer capacities derived from
 * rsz:minimumSize: a hostile declaration must not size the buffer
 * arbitrarily.
 */
constexpr uint32_t kMaxAtomBufferSize = 1u << 20;

/**
 * Sample rate and maximum block length of the instance created at
 * configuration time, before the engine's values are known. Processing
 * preparation re-instantiates the plugin when they differ from the
 * engine's.
 */
constexpr auto kProvisionalSampleRate = units::sample_rate (48000);
constexpr auto kProvisionalMaxBlockLength = units::samples (512u);

/** A whole note is four quarter notes. */
constexpr double kQuartersPerWholeNote = 4.0;

/** Returns the last path segment of a URI (after '#' or '/'). */
std::string_view
uri_local_name (std::string_view uri)
{
  const auto hash_pos = uri.find_last_of ('#');
  if (hash_pos != std::string_view::npos)
    return uri.substr (hash_pos + 1);
  const auto slash_pos = uri.find_last_of ('/');
  return slash_pos != std::string_view::npos ? uri.substr (slash_pos + 1) : uri;
}

utils::Utf8String
uri_local_name_string (std::string_view uri)
{
  return utils::Utf8String::from_utf8_encoded_string (
    std::string (uri_local_name (uri)));
}

uint32_t
urid_map_callback (void * handle, const char * uri)
{
  return static_cast<Lv2UridMap *> (handle)->map (uri);
}

const char *
urid_unmap_callback (void * handle, uint32_t urid)
{
  return static_cast<Lv2UridMap *> (handle)->unmap (urid);
}

dsp::ParameterRange::Unit
unit_from_lv2_uri (std::string_view uri)
{
  const auto name = uri_local_name (uri);
  if (name == "hz")
    return dsp::ParameterRange::Unit::Hz;
  if (name == "mhz")
    return dsp::ParameterRange::Unit::MHz;
  if (name == "db")
    return dsp::ParameterRange::Unit::Db;
  if (name == "degree")
    return dsp::ParameterRange::Unit::Degrees;
  if (name == "s")
    return dsp::ParameterRange::Unit::Seconds;
  if (name == "ms")
    return dsp::ParameterRange::Unit::Ms;
  return dsp::ParameterRange::Unit::None;
}

} // namespace

class Lv2Plugin::Lv2PluginImpl
{
public:
  Lv2PluginImpl (Lv2Plugin &owner, PluginHostWindowFactory host_window_factory)
      : owner_ (owner), host_window_factory_ (std::move (host_window_factory))
  {
  }

  struct PortInfo
  {
    enum class Type : std::uint8_t
    {
      Audio,
      Control,
      CV,
      Atom,
      /** Directed port with an unrecognized type, e.g. the legacy
       * lv2:event type; treated like an unroutable atom port. */
      Unknown,
    };

    uint32_t          index{};
    utils::Utf8String symbol;
    utils::Utf8String name;
    dsp::PortFlow     flow{};
    Type              type{};

    /* control port metadata */
    float def{};
    float min{};
    float max{};
    bool  is_toggled{};
    bool  is_integer{};
    bool  is_logarithmic{};
    bool  is_enumeration{};
    /** Scale point labels, sorted by their values. */
    std::vector<utils::Utf8String> scale_point_labels;
    std::vector<float>             scale_point_values;
    dsp::ParameterRange::Unit      unit{ dsp::ParameterRange::Unit::None };
    /** Control input with lv2:designation lv2:latency. */
    bool is_latency{};

    /** Group URI ("" when the port belongs to no group). */
    utils::Utf8String group_uri;

    /** atom port capabilities */
    bool supports_midi{};
    bool supports_time_position{};
    bool has_sequence_buffer_type{};
    /** rsz:minimumSize declared for the port (0 when undeclared). */
    uint32_t rsz_minimum_size{};

    /**
     * Position of the port's buffer within its flow's control buffer list
     * (control ports only).
     */
    size_t control_buffer_index{};

    /** Audio ports: the bus the port's group forms within its flow, the
     * port's channel within it (group ports in index order map to channels
     * in order), and the bus's external id (the group's first port index).
     */
    size_t audio_bus_index{};
    size_t audio_bus_external_id{};
    size_t audio_channel{};
    /** CV ports: position among the flow's CV ports. */
    size_t cv_index_in_flow{};

    /* Per-port slots in the scratch buffers assigned to ports this host
     * cannot route to an engine port. */
    size_t audio_scratch_slot{};
    size_t cv_scratch_slot{};
    /** Whether the port owns a buffer in the atom scratch buffers
     * (routed atom ports do not). */
    bool has_atom_scratch = false;
    /** Byte offset of the port's buffer within the atom scratch
     * buffers (ports that own one). */
    size_t atom_scratch_byte_offset{};
  };

  /** Resolves the lilv plugin for @p bundle matching @p uri_hash. */
  bool resolve_plugin (const std::filesystem::path &bundle, int64_t uri_hash);

  /** Reads the port metadata of plugin_ into ports_. Returns false when
   * the plugin data is malformed (e.g. a port without a direction). */
  bool read_port_metadata ();

  /**
   * Groups the audio ports of one flow into buses, returned as ports_
   * indices: one bus per port group (first-occurrence order, the group's
   * ports in ports_ order) and one bus per ungrouped port. Both the
   * per-port bus assignment in read_port_metadata() and the
   * AudioBusConfig list derive from this grouping, so the two can never
   * diverge.
   */
  [[nodiscard]] std::vector<std::vector<size_t>>
  audio_bus_groups (dsp::PortFlow flow) const;

  /** Builds the bus configuration of @p flow from the port metadata. */
  std::vector<dsp::AudioBusConfig>
  build_audio_bus_configs (dsp::PortFlow flow) const;

  /** Instantiates the plugin at @p sample_rate with a maximum block length
   * of @p max_block_length (both are bound at instantiation). */
  bool instantiate (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length);

  /** Tears the instance down (deactivate + cleanup). */
  void free_instance ();

  /** Connects the control port buffers to the instance. */
  void connect_control_ports ();

  /** Pairs the audio/CV port metadata with the engine ports. */
  void pair_ports_with_engine_ports ();

  /** Connects a port with plugin code exempted from RTSan:
   * connect_port is plugin code, which non-hardRTCapable plugins may
   * block in. */
  void
  connect_port_rt (uint32_t index, void * data) noexcept [[clang::nonblocking]]
  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    __rtsan::ScopedDisabler disabler;
#endif
    lilv_instance_connect_port (instance_, index, data);
  }

  /** Connects the audio and CV port pointers for this chunk; ports with
   * no routed engine port are connected to their scratch buffer slot. */
  void connect_sample_ports (units::sample_u32_t local_offset) noexcept
    [[clang::nonblocking]];

  /** Forges the atom input buffers for this chunk. */
  void forge_atom_inputs (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept [[clang::nonblocking]];

  /** Parses the atom output buffer into the MIDI output port; events
   * outside the chunk are dropped. */
  void parse_atom_outputs (
    units::sample_u32_t local_offset,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  /**
   * Writes an atom:Chunk header with the buffer capacity into every
   * output atom buffer, as required by the LV2 atom port contract before
   * each run.
   */
  void reset_atom_output () noexcept [[clang::nonblocking]];

  /** Writes changed parameter values into the control input buffers. */
  void apply_changed_param_values () noexcept [[clang::nonblocking]];

  /** Reads the control output ports (latency). */
  void read_control_outputs () noexcept [[clang::nonblocking]];

  /**
   * Zeroes the routed output buffers of the chunk window [local_offset,
   * local_offset + nframes) of a block that cannot run on the instance
   * and records the drop: the next processing preparation logs it.
   */
  void degrade_to_silence (
    units::sample_u32_t local_offset,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  /** Serializes the current instance state to a TTL string, or nullopt
   * when no instance exists or serialization fails. */
  [[nodiscard]] std::optional<std::string>
  save_state_to_string () [[clang::blocking]];

  /** Restores the instance from a TTL state string. */
  bool
  restore_state_from_string (const std::string &ttl_state) [[clang::blocking]];

  /** LV2 state restore callback: applies a control port value. */
  static void state_set_port_value (
    const char * port_symbol,
    void *       user_data,
    const void * value,
    uint32_t     size,
    uint32_t     type);

  /**
   * LV2 state save callback: returns the current value of a control input
   * port, read from the connected control buffer.
   */
  static const void * state_get_port_value (
    const char * port_symbol,
    void *       user_data,
    uint32_t *   size,
    uint32_t *   type);

  /** The ports_ index of the first port matching @p pred, or -1. */
  template <typename Pred> int32_t find_port (Pred pred) const
  {
    const auto it = std::ranges::find_if (ports_, pred);
    return it == ports_.end () ? -1 : static_cast<int32_t> (it - ports_.begin ());
  }

  /**
   * Capacity of the atom sequence buffer of @p port: its
   * rsz:minimumSize when larger than the default capacity (already
   * clamped when read).
   */
  static uint32_t atom_port_capacity (const PortInfo &port)
  {
    return std::max (kAtomBufferSize, port.rsz_minimum_size);
  }

  /** Capacity of the atom sequence buffer of the port at @p ports_idx
   * (the default capacity when there is no such port). */
  uint32_t atom_port_capacity (int32_t ports_idx) const
  {
    if (ports_idx < 0)
      return kAtomBufferSize;
    return atom_port_capacity (ports_[static_cast<size_t> (ports_idx)]);
  }

  struct CtrlInParam;

  /**
   * Returns the control buffer value of @p ctrl's port for the parameter
   * value @p value_0_to_1. Enumeration parameters are index-valued while
   * their ports expect the selected scale point's value, so the two
   * spaces are mapped through the scale points.
   */
  float
  control_value_for_param (const CtrlInParam &ctrl, float value_0_to_1) const
  {
    const auto range_value = ctrl.param->range ().convertFrom0To1 (value_0_to_1);
    if (
      ctrl.port == nullptr || !ctrl.port->is_enumeration
      || ctrl.port->scale_point_values.empty ())
      return range_value;
    const auto index = static_cast<size_t> (std::lround (
      std::clamp (
        range_value, 0.f, ctrl.port->scale_point_values.size () - 1.f)));
    return ctrl.port->scale_point_values[index];
  }

  /**
   * Returns the normalized parameter value of @p ctrl's parameter for
   * the control buffer value @p control_value (the inverse of
   * control_value_for_param(): enumeration ports map to the nearest
   * scale point's index).
   */
  float
  param_value_0_to_1_for_control (const CtrlInParam &ctrl, float control_value)
    const
  {
    const auto &range = ctrl.param->range ();
    if (
      ctrl.port == nullptr || !ctrl.port->is_enumeration
      || ctrl.port->scale_point_values.empty ())
      return range.convertTo0To1 (control_value);
    const auto it = std::ranges::min_element (
      ctrl.port->scale_point_values, {},
      [control_value] (float v) { return std::abs (v - control_value); });
    return range.convertTo0To1 (
      static_cast<float> (it - ctrl.port->scale_point_values.begin ()));
  }

  /** The metadata of the control input with @p symbol, or nullptr when no
   * such port exists. */
  const PortInfo * find_control_input (std::string_view symbol) const
  {
    const auto it = std::ranges::find_if (ports_, [&symbol] (const PortInfo &p) {
      return p.symbol.view () == symbol;
    });
    return it == ports_.end () ? nullptr : &*it;
  }

  Lv2Plugin &owner_;

  PluginHostWindowFactory host_window_factory_;

  /** Resolved lilv plugin, borrowed from the shared world. */
  const LilvPlugin * plugin_ = nullptr;

  LilvInstance * instance_ = nullptr;

  std::vector<PortInfo> ports_;

  /* Feature storage; plugins keep these pointers after instantiation, so
   * they must stay stable and valid until the instance is freed. */
  LV2_URID_Map   urid_map_feature_{};
  LV2_URID_Unmap urid_unmap_feature_{};
  /** Copy of the world's host URIDs, read on the audio thread without
   * touching the world. */
  Lv2HostUrids                      host_urids_{};
  std::array<LV2_Options_Option, 6> options_{};
  std::vector<LV2_Feature>          features_;
  std::vector<const LV2_Feature *>  feature_ptrs_;
  float                             sample_rate_opt_{};
  int32_t                           min_block_opt_{};
  int32_t                           max_block_opt_{};
  int32_t                           nominal_block_opt_{};
  int32_t                           sequence_size_opt_{};

  /** Control port buffers, indexed by PortInfo::control_buffer_index. */
  std::vector<float> control_in_bufs_;
  std::vector<float> control_out_bufs_;

  /** Parameter routing, rebuilt on the main thread at load. */
  struct CtrlInParam
  {
    dsp::ProcessorParameter * param{};
    /** The port the parameter drives (null for unrouted buffers). */
    const PortInfo * port{};
  };
  /** Parallel to control_in_bufs_. */
  std::vector<CtrlInParam> ctrl_in_params_;
  /** Parameter list index -> control input buffer index, or -1. */
  std::vector<int32_t> param_to_ctrl_in_;
  /** control_out_bufs_ index of the latency port, or -1. */
  int32_t latency_buf_index_ = -1;

  /** Atom buffers. */
  std::vector<uint8_t> atom_in_buf_;
  std::vector<uint8_t> time_in_buf_;
  std::vector<uint8_t> atom_out_buf_;
  /** Per-port atom buffers for atom ports this host cannot route (and
   * unknown-type ports), concatenated in ports_ order: each port's
   * buffer starts at its PortInfo::atom_scratch_byte_offset and spans
   * its atom_port_capacity(). */
  std::vector<uint8_t> atom_scratch_buf_;
  LV2_Atom_Forge       forge_{};

  int32_t midi_in_port_idx_ = -1;
  int32_t midi_out_port_idx_ = -1;
  int32_t time_in_port_idx_ = -1;

  /** Engine ports paired with the audio buses, indexed by bus (nullptr
   * while unpaired); CV ports pair positionally in ports_ order. */
  std::vector<dsp::AudioPort *> audio_in_ports_by_bus_;
  std::vector<dsp::AudioPort *> audio_out_ports_by_bus_;
  std::vector<dsp::CVPort *>    cv_in_ports_by_port_;
  std::vector<dsp::CVPort *>    cv_out_ports_by_port_;

  /* Zeroed per-port fallback buffers for audio/CV ports with no routed
   * engine port. Sized (port count * scratch_block_stride_) by
   * resize_scratch_buffers(), which also records the stride so buffer
   * size and the per-block pointer arithmetic can never diverge. */
  std::vector<float>  audio_scratch_buf_;
  std::vector<float>  cv_scratch_buf_;
  units::sample_u32_t scratch_block_stride_{ units::samples (0u) };

  /** (Re-)creates the zeroed audio/CV scratch buffers for @p
   * max_block_length and records the stride they are sized with. */
  void resize_scratch_buffers (units::sample_u32_t max_block_length);

  /** Group URIs designated pg:mainInput/pg:mainOutput by the plugin (""
   * when undeclared). */
  utils::Utf8String main_in_group_uri_;
  utils::Utf8String main_out_group_uri_;

  /** Whether the unroutable-port warning was already logged. */
  bool unroutable_ports_warned_ = false;

  /** Whether a block was dropped by degrade_to_silence() since the last
   * processing preparation (which logs and clears it). */
  std::atomic<bool> degradation_reported_{ false };

  /** Number of input events dropped by forge_atom_inputs() since the
   * last processing preparation (which logs and clears it). */
  std::atomic<uint64_t> atom_events_dropped_{ 0 };

  /** Number of plugin output MIDI events dropped because the engine
   * MIDI buffer was full since the last processing preparation (which
   * logs and clears it). */
  std::atomic<uint64_t> midi_output_events_dropped_{ 0 };

  units::sample_rate_t last_sample_rate_{ units::sample_rate (0) };
  units::sample_u32_t  last_max_block_length_{ units::samples (0u) };

  /** Reported playback latency; written by the audio thread and read on
   * the main thread. The value is self-contained, so relaxed access is
   * sufficient: atomicity is all that is required, and the latency-changed
   * notification provides the cross-thread ordering. */
  std::atomic<units::sample_u32_t> latency_{ units::samples (0u) };
};

// ============================================================================
// Lifecycle
// ============================================================================

Lv2Plugin::Lv2Plugin (
  utils::IObjectRegistry                &registry,
  std::shared_ptr<Lv2World>              world,
  std::function<units::sample_rate_t ()> sample_rate_provider,
  std::function<units::sample_u32_t ()>  buffer_size_provider,
  PluginHostWindowFactory                host_window_factory,
  QObject *                              parent)
    : Plugin (registry, parent),
      pimpl_ (
        std::make_unique<Lv2PluginImpl> (*this, std::move (host_window_factory))),
      world_ (std::move (world)),
      sample_rate_provider_ (std::move (sample_rate_provider)),
      buffer_size_provider_ (std::move (buffer_size_provider))
{
  if (world_ == nullptr)
    {
      throw std::logic_error ("Lv2Plugin requires a Lv2World");
    }

  connect (
    this, &Plugin::configurationChanged, this,
    &Lv2Plugin::on_configuration_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  set_bypass_id (bypass_ref.id ());
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

Lv2Plugin::~Lv2Plugin ()
{
  // Unload before member destruction: pimpl_ is destroyed after the
  // world it borrows the plugin data from, and freeing the instance
  // touches that data
  if (pimpl_ != nullptr && pimpl_->plugin_ != nullptr)
    unload_current_plugin ();
}

void
Lv2Plugin::on_configuration_changed (
  PluginConfiguration *,
  bool generateNewPluginPortsAndParams)
{
  // The bundle path is expected as the path alternative of the descriptor
  // identity; anything else is untrusted input and fails the load cleanly
  const auto * path = std::get_if<std::filesystem::path> (
    &configuration ()->descriptor ()->path_or_id_);
  if (path == nullptr)
    {
      z_warning ("LV2: descriptor of '{}' carries no bundle path", get_name ());
      Q_EMIT instantiationFinished (false, tr ("LV2 descriptor has no path"));
      return;
    }

  const auto load = [this, path, generateNewPluginPortsAndParams] () {
    const auto failure = [this, path] (const QString &error) {
      Q_EMIT instantiationFinished (
        false,
        error.isEmpty ()
          ? tr ("Failed to load LV2 plugin from %1")
              .arg (utils::Utf8String::from_path (*path).to_qstring ())
          : error);
    };

    if (
      !load_plugin (
        *path, configuration ()->descriptor ()->unique_id_,
        generateNewPluginPortsAndParams))
      {
        failure (QString{});
        return;
      }

    // The engine's current values give the best first guess for the
    // instance; processing preparation re-creates the instance when they
    // differ from the engine's values at that point
    const auto provisional_sample_rate =
      sample_rate_provider_ && sample_rate_provider_ () > units::sample_rate (0)
        ? sample_rate_provider_ ()
        : kProvisionalSampleRate;
    const auto provisional_block_length =
      buffer_size_provider_ && buffer_size_provider_ () > units::samples (0u)
        ? buffer_size_provider_ ()
        : kProvisionalMaxBlockLength;
    if (!pimpl_->instantiate (provisional_sample_rate, provisional_block_length))
      {
        pimpl_->last_sample_rate_ = provisional_sample_rate;
        pimpl_->last_max_block_length_ = provisional_block_length;
        failure (tr ("Instantiation failed"));
        return;
      }
    pimpl_->last_sample_rate_ = provisional_sample_rate;
    pimpl_->last_max_block_length_ = provisional_block_length;

    // The instance exists before any processing: the pending state can be
    // applied without pausing anything
    apply_pending_state ();

    Q_EMIT instantiationFinished (true, QString{});
  };

  // Re-configuring frees the live instance, which must not overlap audio
  // processing of the same instance
  if (pimpl_->instance_ != nullptr)
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ (load);
        }
      else
        {
          z_warning (
            "LV2: cannot re-configure '{}' while processing; the host "
            "cannot pause processing",
            get_name ());
          Q_EMIT instantiationFinished (
            false, tr ("Cannot re-configure LV2 plugin during processing"));
        }
      return;
    }

  load ();
}

bool
Lv2Plugin::Lv2PluginImpl::resolve_plugin (
  const std::filesystem::path &bundle,
  int64_t                      uri_hash)
{
  for (const auto &info : get_plugins_in_bundle (*owner_.world_, bundle))
    {
      if (get_hash_for_range (std::string (info.uri_.view ())) != uri_hash)
        continue;

      const auto * plugin =
        owner_.world_->find_plugin (bundle, info.uri_.view ());
      if (plugin == nullptr)
        {
          z_warning (
            "LV2: plugin '{}' disappeared from bundle '{}' during "
            "resolution",
            info.uri_, bundle);
          return false;
        }
      plugin_ = plugin;
      return true;
    }
  z_warning (
    "LV2: no plugin with URI hash {} found in bundle {}", uri_hash, bundle);
  return false;
}

bool
Lv2Plugin::Lv2PluginImpl::read_port_metadata ()
{
  ports_.clear ();

  auto *             world = owner_.world_->raw ();
  const LilvNodeUPtr input_port{ lilv_new_uri (world, LV2_CORE__InputPort) };
  const LilvNodeUPtr output_port{ lilv_new_uri (world, LV2_CORE__OutputPort) };
  const LilvNodeUPtr audio_port{ lilv_new_uri (world, LV2_CORE__AudioPort) };
  const LilvNodeUPtr control_port{ lilv_new_uri (world, LV2_CORE__ControlPort) };
  const LilvNodeUPtr cv_port{ lilv_new_uri (world, LV2_CORE__CVPort) };
  const LilvNodeUPtr atom_port{ lilv_new_uri (world, LV2_ATOM__AtomPort) };
  const LilvNodeUPtr midi_event{ lilv_new_uri (world, LV2_MIDI__MidiEvent) };
  const LilvNodeUPtr time_position{ lilv_new_uri (world, LV2_TIME__Position) };
  const LilvNodeUPtr buffer_type{ lilv_new_uri (world, LV2_ATOM__bufferType) };
  const LilvNodeUPtr atom_sequence{ lilv_new_uri (world, LV2_ATOM__Sequence) };
  const LilvNodeUPtr designation{ lilv_new_uri (world, LV2_CORE__designation) };
  const LilvNodeUPtr latency_uri{ lilv_new_uri (world, LV2_CORE__latency) };
  const LilvNodeUPtr group{ lilv_new_uri (world, LV2_PORT_GROUPS__group) };
  const LilvNodeUPtr main_input{
    lilv_new_uri (world, LV2_PORT_GROUPS__mainInput)
  };
  const LilvNodeUPtr main_output{
    lilv_new_uri (world, LV2_PORT_GROUPS__mainOutput)
  };
  const LilvNodeUPtr toggled{ lilv_new_uri (world, LV2_CORE__toggled) };
  const LilvNodeUPtr integer{ lilv_new_uri (world, LV2_CORE__integer) };
  const LilvNodeUPtr enumeration{ lilv_new_uri (world, LV2_CORE__enumeration) };
  const LilvNodeUPtr logarithmic{
    lilv_new_uri (world, LV2_PORT_PROPS__logarithmic)
  };
  const LilvNodeUPtr default_p{ lilv_new_uri (world, LV2_CORE__default) };
  const LilvNodeUPtr minimum_p{ lilv_new_uri (world, LV2_CORE__minimum) };
  const LilvNodeUPtr maximum_p{ lilv_new_uri (world, LV2_CORE__maximum) };
  const LilvNodeUPtr unit{ lilv_new_uri (world, LV2_UNITS__unit) };
  const LilvNodeUPtr minimum_size{
    lilv_new_uri (world, LV2_RESIZE_PORT__minimumSize)
  };

  const auto plugin_group_uri = [this] (const LilvNode * predicate) {
    const LilvNodesUPtr nodes{ lilv_plugin_get_value (plugin_, predicate) };
    if (nodes == nullptr || lilv_nodes_size (nodes.get ()) == 0)
      return utils::Utf8String{};
    return node_to_utf8 (lilv_nodes_get_first (nodes.get ()));
  };
  main_in_group_uri_ = plugin_group_uri (main_input.get ());
  main_out_group_uri_ = plugin_group_uri (main_output.get ());

  const auto num_ports = lilv_plugin_get_num_ports (plugin_);

  for (const auto port_index : std::views::iota (0u, num_ports))
    {
      const auto * port = lilv_plugin_get_port_by_index (plugin_, port_index);

      auto &info = ports_.emplace_back ();
      info.index = port_index;
      info.symbol = node_to_utf8 (lilv_port_get_symbol (plugin_, port));
      if (
        const LilvNodeUPtr name{ lilv_port_get_name (plugin_, port) };
        name != nullptr)
        {
          info.name = node_to_utf8 (name.get ());
        }

      const bool is_input = lilv_port_is_a (plugin_, port, input_port.get ());
      const bool is_output = lilv_port_is_a (plugin_, port, output_port.get ());
      if (!is_input && !is_output)
        {
          // LV2 requires every port to have a direction
          z_warning (
            "LV2: port {} of '{}' has no direction; refusing to load",
            info.symbol, owner_.get_name ());
          return false;
        }
      info.flow = is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output;

      if (lilv_port_is_a (plugin_, port, audio_port.get ()))
        info.type = PortInfo::Type::Audio;
      else if (lilv_port_is_a (plugin_, port, control_port.get ()))
        info.type = PortInfo::Type::Control;
      else if (lilv_port_is_a (plugin_, port, cv_port.get ()))
        info.type = PortInfo::Type::CV;
      else if (lilv_port_is_a (plugin_, port, atom_port.get ()))
        info.type = PortInfo::Type::Atom;
      else
        {
          // Directed ports with an unrecognized type, e.g. the legacy
          // lv2:event type, are connected to an atom scratch buffer
          z_warning (
            "LV2: port {} of '{}' is no audio, control, CV or atom port; "
            "it is connected to a scratch buffer",
            info.symbol, owner_.get_name ());
          info.type = PortInfo::Type::Unknown;
        }

      if (info.type == PortInfo::Type::Control)
        {
          // Ranges, port properties and scale points only feed the
          // parameters built from input control ports
          if (info.flow == dsp::PortFlow::Input)
            {
              const auto get_port_float =
                [&] (const LilvNodeUPtr &predicate) -> std::optional<float> {
                const LilvNodesUPtr nodes{
                  lilv_port_get_value (plugin_, port, predicate.get ())
                };
                if (nodes == nullptr || lilv_nodes_size (nodes.get ()) == 0)
                  return std::nullopt;
                const auto * node = lilv_nodes_get_first (nodes.get ());
                // Non-numeric literals convert to NaN, which cannot be
                // detected reliably in release builds: accept values by
                // node type instead
                if (
                  !lilv_node_is_float (node) && !lilv_node_is_int (node)
                  && !lilv_node_is_bool (node))
                  return std::nullopt;
                return lilv_node_as_float (node);
              };
              const auto defs = get_port_float (default_p);
              const auto mins = get_port_float (minimum_p);
              const auto maxs = get_port_float (maximum_p);
              const auto range_usable = [&] () {
                if (
                  !defs.has_value () || !mins.has_value () || !maxs.has_value ())
                  return false;
                // min >= max breaks the linear range conversion, and
                // non-finite values would propagate NaN into the audio
                // path; both fall back to a usable dummy
                return std::isfinite (*mins) && std::isfinite (*maxs)
                       && std::isfinite (*defs) && *mins < *maxs;
              }();
              if (!range_usable)
                {
                  // LV2 requires a finite default/min/max with min < max on
                  // input control ports; anything else falls back to a
                  // usable dummy so the port can still be connected
                  z_warning (
                    "LV2: control input port {} of '{}' has no usable "
                    "default/min/max; assuming 0..1",
                    info.symbol, owner_.get_name ());
                  info.def = 0.f;
                  info.min = 0.f;
                  info.max = 1.f;
                }
              else
                {
                  info.def = std::clamp (*defs, *mins, *maxs);
                  info.min = *mins;
                  info.max = *maxs;
                }

              info.is_toggled =
                lilv_port_has_property (plugin_, port, toggled.get ());
              info.is_integer =
                lilv_port_has_property (plugin_, port, integer.get ());
              info.is_enumeration =
                lilv_port_has_property (plugin_, port, enumeration.get ());
              info.is_logarithmic =
                lilv_port_has_property (plugin_, port, logarithmic.get ());
              if (info.is_logarithmic && info.min <= 0.f)
                {
                  // A logarithmic parameter needs a positive minimum to
                  // span its normalized range
                  z_warning (
                    "LV2: control port {} of '{}' is logarithmic with a "
                    "non-positive minimum; treating it as linear",
                    info.symbol, owner_.get_name ());
                  info.is_logarithmic = false;
                }

              if (
                const LilvNodesUPtr unit_nodes{
                  lilv_port_get_value (plugin_, port, unit.get ()) };
                unit_nodes != nullptr && lilv_nodes_size (unit_nodes.get ()) > 0)
                {
                  info.unit = unit_from_lv2_uri (
                    node_to_utf8 (lilv_nodes_get_first (unit_nodes.get ()))
                      .view ());
                }

              // Scale points: lilv returns them in undefined order, so sort
              // by value for deterministic enumeration indices; non-numeric
              // point values are skipped like non-numeric ranges above
              std::vector<std::pair<utils::Utf8String, float>> points;
              {
                const LilvScalePointsUPtr scale_points{
                  lilv_port_get_scale_points (plugin_, port)
                };
                if (scale_points != nullptr)
                  {
                    LILV_FOREACH (scale_points, sp_iter, scale_points.get ())
                      {
                        const auto * sp =
                          lilv_scale_points_get (scale_points.get (), sp_iter);
                        const auto * value_node =
                          lilv_scale_point_get_value (sp);
                        if (
                          !lilv_node_is_float (value_node)
                          && !lilv_node_is_int (value_node)
                          && !lilv_node_is_bool (value_node))
                          continue;
                        points.emplace_back (
                          node_to_utf8 (lilv_scale_point_get_label (sp)),
                          lilv_node_as_float (value_node));
                      }
                  }
              }
              std::ranges::sort (
                points, {}, &std::pair<utils::Utf8String, float>::second);
              for (auto &[label, value] : points)
                {
                  info.scale_point_labels.push_back (std::move (label));
                  info.scale_point_values.push_back (value);
                }
            }

          if (
            const LilvNodesUPtr designation_nodes{
              lilv_port_get_value (plugin_, port, designation.get ()) };
            designation_nodes != nullptr
            && lilv_nodes_size (designation_nodes.get ()) > 0)
            {
              // The latency designation is the full lv2:latency URI; a
              // local-name match would misread any '#latency' URI
              if (
                lilv_node_equals (
                  lilv_nodes_get_first (designation_nodes.get ()),
                  latency_uri.get ()))
                info.is_latency = true;
            }
        }

      if (info.type == PortInfo::Type::Atom)
        {
          info.supports_midi =
            lilv_port_supports_event (plugin_, port, midi_event.get ());
          info.supports_time_position =
            lilv_port_supports_event (plugin_, port, time_position.get ());
          if (
            const LilvNodesUPtr buffer_type_nodes{
              lilv_port_get_value (plugin_, port, buffer_type.get ()) };
            buffer_type_nodes != nullptr
            && lilv_nodes_size (buffer_type_nodes.get ()) > 0)
            {
              info.has_sequence_buffer_type = lilv_node_equals (
                lilv_nodes_get_first (buffer_type_nodes.get ()),
                atom_sequence.get ());
            }

          // rsz:minimumSize requests sequence space for the port's
          // buffer; requests are clamped so a hostile declaration
          // cannot size the buffer arbitrarily
          if (
            const LilvNodesUPtr min_size_nodes{
              lilv_port_get_value (plugin_, port, minimum_size.get ()) };
            min_size_nodes != nullptr
            && lilv_nodes_size (min_size_nodes.get ()) > 0)
            {
              const auto * min_size_node =
                lilv_nodes_get_first (min_size_nodes.get ());
              if (lilv_node_is_int (min_size_node))
                {
                  const auto declared = lilv_node_as_int (min_size_node);
                  if (declared > static_cast<int32_t> (kMaxAtomBufferSize))
                    {
                      z_warning (
                        "LV2: atom port {} of '{}' requests {} bytes of "
                        "sequence space; clamping the buffer to {} bytes",
                        info.symbol, owner_.get_name (), declared,
                        kMaxAtomBufferSize);
                      info.rsz_minimum_size = kMaxAtomBufferSize;
                    }
                  else if (declared > static_cast<int32_t> (kAtomBufferSize))
                    {
                      info.rsz_minimum_size = static_cast<uint32_t> (declared);
                    }
                }
            }
        }

      if (
        const LilvNodesUPtr group_nodes{
          lilv_port_get_value (plugin_, port, group.get ()) };
        group_nodes != nullptr && lilv_nodes_size (group_nodes.get ()) > 0)
        {
          info.group_uri =
            node_to_utf8 (lilv_nodes_get_first (group_nodes.get ()));
        }
    }

  midi_in_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Input
           && p.has_sequence_buffer_type && p.supports_midi;
  });
  midi_out_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Output
           && p.has_sequence_buffer_type && p.supports_midi;
  });
  time_in_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Input
           && p.has_sequence_buffer_type && p.supports_time_position;
  });

  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto count_midi =
        std::ranges::count_if (ports_, [flow] (const PortInfo &p) {
          return p.type == PortInfo::Type::Atom && p.flow == flow
                 && p.has_sequence_buffer_type && p.supports_midi;
        });
      if (count_midi > 1)
        {
          z_warning (
            "LV2: '{}' has {} MIDI atom {} ports; only the first is routed",
            owner_.get_name (), count_midi,
            flow == dsp::PortFlow::Input ? "input" : "output");
        }
    }
  if (
    std::ranges::any_of (ports_, [] (const PortInfo &p) {
      return p.type == PortInfo::Type::Atom
             && (!p.has_sequence_buffer_type || (!p.supports_midi && !p.supports_time_position));
    }))
    {
      z_warning (
        "LV2: '{}' has atom ports this host cannot route (no sequence "
        "buffer type or unsupported event types); they are connected to "
        "scratch buffers",
        owner_.get_name ());
    }

  // Assign the control buffer indices and the latency port
  const auto num_ctrl_ins = std::ranges::count_if (ports_, [] (const PortInfo &p) {
    return p.type == PortInfo::Type::Control && p.flow == dsp::PortFlow::Input;
  });
  const auto num_ctrl_outs =
    std::ranges::count_if (ports_, [] (const PortInfo &p) {
      return p.type == PortInfo::Type::Control && p.flow == dsp::PortFlow::Output;
    });
  control_in_bufs_.assign (num_ctrl_ins, 0.f);
  control_out_bufs_.assign (num_ctrl_outs, 0.f);
  size_t ctrl_in_pos = 0;
  size_t ctrl_out_pos = 0;
  latency_buf_index_ = -1;
  for (auto &port : ports_)
    {
      if (port.type != PortInfo::Type::Control)
        continue;
      if (port.flow == dsp::PortFlow::Input)
        {
          port.control_buffer_index = ctrl_in_pos++;
        }
      else
        {
          port.control_buffer_index = ctrl_out_pos;
          if (port.is_latency)
            latency_buf_index_ = static_cast<int32_t> (ctrl_out_pos);
          ++ctrl_out_pos;
        }
    }

  // Assign the bus and channel positions of the audio and CV ports: the
  // audio buses come from the shared grouping, with the bus's external id
  // being its first port's LV2 index; CV ports are numbered positionally
  // within each flow
  size_t next_cv_per_flow[2] = { 0, 0 };
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto flow_idx = flow == dsp::PortFlow::Input ? 0uz : 1uz;
      for (auto &port : ports_)
        {
          if (port.type != PortInfo::Type::CV || port.flow != flow)
            continue;
          port.cv_index_in_flow = next_cv_per_flow[flow_idx]++;
        }

      const auto buses = audio_bus_groups (flow);
      for (const auto &[bus_index, bus] : utils::views::enumerate (buses))
        {
          const auto external_id = ports_[bus.front ()].index;
          for (const auto &[channel, port_index] : utils::views::enumerate (bus))
            {
              auto &port_info = ports_[port_index];
              port_info.audio_bus_index = bus_index;
              port_info.audio_bus_external_id = external_id;
              port_info.audio_channel = channel;
            }
        }
    }

  // Assign the per-port scratch buffers: one audio/CV slot per port of
  // that type, and one atom buffer per unroutable atom or unknown-type
  // port, sized per port from its rsz:minimumSize
  size_t next_audio_slot = 0;
  size_t next_cv_slot = 0;
  size_t atom_scratch_bytes = 0;
  for (auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          port.audio_scratch_slot = next_audio_slot++;
          break;
        case PortInfo::Type::CV:
          port.cv_scratch_slot = next_cv_slot++;
          break;
        case PortInfo::Type::Unknown:
          port.has_atom_scratch = true;
          port.atom_scratch_byte_offset = atom_scratch_bytes;
          atom_scratch_bytes += atom_port_capacity (port);
          break;
        case PortInfo::Type::Atom:
          {
            const auto ports_idx = static_cast<int32_t> (&port - ports_.data ());
            if (
              ports_idx != midi_in_port_idx_ && ports_idx != midi_out_port_idx_
              && ports_idx != time_in_port_idx_)
              {
                port.has_atom_scratch = true;
                port.atom_scratch_byte_offset = atom_scratch_bytes;
                atom_scratch_bytes += atom_port_capacity (port);
              }
            break;
          }
        default:
          break;
        }
    }

  return true;
}

bool
Lv2Plugin::load_plugin (
  const std::filesystem::path &bundle_path,
  int64_t                      uri_hash,
  bool                         generate_new)
{
  assert (QThread::currentThread () == thread ());

  unload_current_plugin ();

  if (!pimpl_->resolve_plugin (bundle_path, uri_hash))
    {
      return false;
    }

  z_info ("LV2: loading plugin from bundle {}", bundle_path);

  if (!pimpl_->read_port_metadata ())
    {
      // A refused plugin must not leave partially read metadata behind:
      // reset to the unloaded state
      unload_current_plugin ();
      return false;
    }
  create_ports_and_parameters (generate_new);

  return true;
}

void
Lv2Plugin::unload_current_plugin ()
{
  pimpl_->free_instance ();
  pimpl_->plugin_ = nullptr;
  pimpl_->ports_.clear ();
  pimpl_->control_in_bufs_.clear ();
  pimpl_->control_out_bufs_.clear ();
  pimpl_->ctrl_in_params_.clear ();
  pimpl_->param_to_ctrl_in_.clear ();
  pimpl_->latency_buf_index_ = -1;
  pimpl_->audio_in_ports_by_bus_.clear ();
  pimpl_->audio_out_ports_by_bus_.clear ();
  pimpl_->cv_in_ports_by_port_.clear ();
  pimpl_->cv_out_ports_by_port_.clear ();
  pimpl_->audio_scratch_buf_.clear ();
  pimpl_->cv_scratch_buf_.clear ();
  pimpl_->atom_scratch_buf_.clear ();
  pimpl_->unroutable_ports_warned_ = false;
  pimpl_->midi_in_port_idx_ = -1;
  pimpl_->midi_out_port_idx_ = -1;
  pimpl_->time_in_port_idx_ = -1;
  pimpl_->latency_.store (units::samples (0u), std::memory_order_relaxed);
}

void
Lv2Plugin::Lv2PluginImpl::free_instance ()
{
  if (instance_ == nullptr)
    return;

  lilv_instance_deactivate (instance_);
  lilv_instance_free (instance_);
  instance_ = nullptr;
  latency_.store (units::samples (0u), std::memory_order_relaxed);
}

// ============================================================================
// Ports and parameters
// ============================================================================

std::vector<std::vector<size_t>>
Lv2Plugin::Lv2PluginImpl::audio_bus_groups (dsp::PortFlow flow) const
{
  std::vector<std::vector<size_t>>             buses;
  std::unordered_map<std::string_view, size_t> bus_index_by_group;
  for (const auto &[i, port] : utils::views::enumerate (ports_))
    {
      if (port.type != PortInfo::Type::Audio || port.flow != flow)
        continue;

      if (port.group_uri.empty ())
        {
          buses.emplace_back ().push_back (i);
          continue;
        }
      const auto [it, inserted] =
        bus_index_by_group.emplace (port.group_uri.view (), buses.size ());
      if (inserted)
        buses.emplace_back ();
      buses[it->second].push_back (i);
    }
  return buses;
}

std::vector<dsp::AudioBusConfig>
Lv2Plugin::Lv2PluginImpl::build_audio_bus_configs (dsp::PortFlow flow) const
{
  const auto  buses = audio_bus_groups (flow);
  const auto &main_group_uri =
    flow == dsp::PortFlow::Input ? main_in_group_uri_ : main_out_group_uri_;

  std::vector<dsp::AudioBusConfig> configs;
  configs.reserve (buses.size ());
  for (const auto &[i, bus] : utils::views::enumerate (buses))
    {
      const auto arrangement = [this, size = bus.size ()] {
        if (size == 1)
          return dsp::SpeakerArrangement::mono ();
        if (size == 2)
          return dsp::SpeakerArrangement::stereo ();
        // Speaker arrangements hold an 8-bit channel count
        if (size > std::numeric_limits<uint8_t>::max ())
          {
            z_warning (
              "LV2: '{}' has an audio group with {} channels; clamping "
              "the arrangement to 255",
              owner_.get_name (), size);
            return dsp::SpeakerArrangement::discrete_channels (
              std::numeric_limits<uint8_t>::max ());
          }
        return dsp::SpeakerArrangement::discrete_channels (
          static_cast<uint8_t> (size));
      }();
      const auto &first_port = ports_[bus.front ()];
      const auto  name =
        first_port.group_uri.empty ()
          ? first_port.name
          : uri_local_name_string (first_port.group_uri.view ());
      // A group explicitly designated pg:mainInput/pg:mainOutput is the
      // main bus; without a designation the first bus is main and the
      // rest sidechains
      const auto is_main =
        !main_group_uri.empty ()
          ? first_port.group_uri.view () == main_group_uri.view ()
          : i == 0;
      configs.push_back (
        dsp::AudioBusConfig{
          .name = name,
          .arrangement = arrangement,
          .purpose =
            is_main ? dsp::AudioPort::Purpose::Main
                    : dsp::AudioPort::Purpose::Sidechain,
          .active = true,
          .external_id = first_port.index });
    }
  return configs;
}

void
Lv2Plugin::create_ports_and_parameters (bool generate_new)
{
  assert (pimpl_->instance_ == nullptr);

  // On deserialization the MIDI and CV ports are restored from JSON before
  // the configuration change arrives, so they are only created for fresh
  // loads; the routing tables below are rebuilt either way
  if (generate_new)
    {
      if (pimpl_->midi_in_port_idx_ >= 0)
        {
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string ("MIDI Input 1"),
            dsp::PortFlow::Input);
          add_input_port (port_ref);
        }
      if (pimpl_->midi_out_port_idx_ >= 0)
        {
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string ("MIDI Output 1"),
            dsp::PortFlow::Output);
          add_output_port (port_ref);
        }
    }

  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto result = dsp::reconcile_audio_bus_configuration (
        registry (), *this, flow, pimpl_->build_audio_bus_configs (flow));
      if (!generate_new)
        {
          // The ports were restored from a save: a changed topology means
          // the plugin's buses no longer match the saved ones
          if (result.graph_changed)
            {
              z_warning (
                "LV2: the bus topology of '{}' changed against its "
                "restored ports; the graph must be recalculated",
                get_name ());
            }
          else if (result.metadata_changed)
            {
              z_debug (
                "LV2: bus metadata of '{}' changed against its restored "
                "ports",
                get_name ());
            }
        }
    }

  if (generate_new)
    {
      for (const auto &port : pimpl_->ports_)
        {
          if (port.type != Lv2PluginImpl::PortInfo::Type::CV)
            continue;

          auto port_ref = utils::create_object<dsp::CVPort> (
            registry (), port.name, port.flow);
          if (port.flow == dsp::PortFlow::Input)
            add_input_port (port_ref);
          else
            add_output_port (port_ref);
        }
    }

  // Parameters from control input ports, matched by unique id (the port
  // symbol)
  const auto                                  &params = get_parameters ();
  std::unordered_map<std::string_view, size_t> param_index_by_id;
  for (const auto &[i, param_ref] : utils::views::enumerate (params))
    {
      param_index_by_id.emplace (
        type_safe::get (param_ref.get ()->get_unique_id ()).view (), i);
    }

  pimpl_->ctrl_in_params_.assign (pimpl_->control_in_bufs_.size (), {});
  pimpl_->param_to_ctrl_in_.assign (params.size (), -1);

  for (const auto &port : pimpl_->ports_)
    {
      if (
        port.type != Lv2PluginImpl::PortInfo::Type::Control
        || port.flow != dsp::PortFlow::Input)
        continue;

      const auto buf_index = port.control_buffer_index;
      const auto unique_id = dsp::ProcessorParameter::UniqueId (port.symbol);
      const auto unique_id_view = type_safe::get (unique_id).view ();

      dsp::ProcessorParameter * param = nullptr;
      if (
        const auto it = param_index_by_id.find (unique_id_view);
        it != param_index_by_id.end ())
        {
          param = params[it->second].get ();
        }
      else if (generate_new)
        {
          const auto range = [&] {
            if (port.is_toggled)
              return dsp::ParameterRange::make_toggle (port.def > 0.5f);
            if (port.is_enumeration && !port.scale_point_labels.empty ())
              {
                const auto default_it =
                  std::ranges::find (port.scale_point_values, port.def);
                auto default_index =
                  default_it != port.scale_point_values.end ()
                    ? static_cast<size_t> (
                        default_it - port.scale_point_values.begin ())
                    : 0uz;
                if (default_it == port.scale_point_values.end ())
                  {
                    // The declared default matches no scale point: snap to
                    // the nearest one so the parameter starts on a listed
                    // value
                    const auto nearest_it = std::ranges::min_element (
                      port.scale_point_values, {}, [def = port.def] (float v) {
                        return std::abs (v - def);
                      });
                    default_index = static_cast<size_t> (
                      nearest_it - port.scale_point_values.begin ());
                    z_warning (
                      "LV2: enumeration port {} of '{}' declares default {} "
                      "matching no scale point; snapping to {}",
                      port.symbol, get_name (), port.def,
                      port.scale_point_labels[default_index].view ());
                  }
                return dsp::ParameterRange::make_enumeration (
                  port.scale_point_labels, default_index);
              }
            if (port.is_enumeration)
              {
                // Enumeration semantics need scale points; without them the
                // parameter falls back to a linear range
                z_warning (
                  "LV2: enumeration port {} of '{}' declares no scale "
                  "points; treating it as linear",
                  port.symbol, get_name ());
              }
            const auto type =
              port.is_integer ? dsp::ParameterRange::Type::Integer
              : port.is_logarithmic
                ? dsp::ParameterRange::Type::Logarithmic
                : dsp::ParameterRange::Type::Linear;
            dsp::ParameterRange r (type, port.min, port.max, 0.f, port.def);
            r.unit_ = port.unit;
            return r;
          }();

          auto param_ref = utils::create_object<dsp::ProcessorParameter> (
            registry (), registry (), unique_id, range, port.name);
          add_parameter (param_ref);
          param = param_ref.get ();
          // The new parameter extends the parameter-to-control-buffer map
          pimpl_->param_to_ctrl_in_.push_back (-1);
          param->set_automatable (true);
          if (!port.group_uri.empty ())
            {
              param->set_group_path (
                { uri_local_name_string (port.group_uri.view ()) });
            }
          param_index_by_id.emplace (
            type_safe::get (param->get_unique_id ()).view (),
            params.size () - 1);
        }

      auto &ctrl_param = pimpl_->ctrl_in_params_[buf_index];
      ctrl_param.port = &port;
      if (param != nullptr)
        {
          const auto index_it = param_index_by_id.find (
            type_safe::get (param->get_unique_id ()).view ());
          assert (index_it != param_index_by_id.end ());
          ctrl_param.param = param;
          pimpl_->param_to_ctrl_in_[index_it->second] =
            static_cast<int32_t> (buf_index);

          // Seed the control buffer with the current value: the parameter
          // default for fresh loads, the restored base value after
          // deserialization
          pimpl_->control_in_bufs_[buf_index] =
            pimpl_->control_value_for_param (ctrl_param, param->baseValue ());
        }
      else
        {
          pimpl_->control_in_bufs_[buf_index] = port.def;
        }
    }
}

// ============================================================================
// Instantiation and processing preparation
// ============================================================================

bool
Lv2Plugin::Lv2PluginImpl::instantiate (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  free_instance ();
  if (plugin_ == nullptr)
    return false;

  // Sequence buffer capacities from the ports' rsz:minimumSize
  const auto atom_in_capacity = atom_port_capacity (midi_in_port_idx_);
  const auto time_in_capacity = atom_port_capacity (time_in_port_idx_);
  const auto atom_out_capacity = atom_port_capacity (midi_out_port_idx_);

  auto * world_urid_map = &owner_.world_->urid_map ();
  urid_map_feature_ = LV2_URID_Map{
    .handle = world_urid_map,
    .map = urid_map_callback,
  };
  urid_unmap_feature_ = LV2_URID_Unmap{
    .handle = world_urid_map,
    .unmap = urid_unmap_callback,
  };

  host_urids_ = owner_.world_->host_urids ();
  const auto &urids = host_urids_;
  const auto  block_length =
    static_cast<int32_t> (max_block_length.in (units::samples));
  // The parameters extension models parameters as floats; the sample rate
  // option is announced as atom:Float like in reference hosts, while the
  // buffer-size options are integers. Blocks are split at loop points, so
  // the smallest chunk run() can be called with is a single frame, while
  // the nominal and maximum chunk is the full block
  sample_rate_opt_ = static_cast<float> (sample_rate.in (units::sample_rate));
  min_block_opt_ = 1;
  max_block_opt_ = block_length;
  nominal_block_opt_ = block_length;
  sequence_size_opt_ = static_cast<int32_t> (std::max (
    { kAtomBufferSize, atom_in_capacity, time_in_capacity, atom_out_capacity }));
  const auto init_option = [] (LV2_Options_Option &option) {
    option.context = LV2_OPTIONS_INSTANCE;
    option.subject = 0;
  };
  init_option (options_[0]);
  options_[0].key = urids.param_sampleRate;
  options_[0].size = sizeof (float);
  options_[0].type = urids.atom_Float;
  options_[0].value = &sample_rate_opt_;
  const auto int_option =
    [&] (uint32_t key, int32_t * value, LV2_Options_Option &option) {
      init_option (option);
      option.key = key;
      option.size = sizeof (int32_t);
      option.type = urids.atom_Int;
      option.value = value;
    };
  int_option (urids.bufsz_minBlockLength, &min_block_opt_, options_[1]);
  int_option (urids.bufsz_maxBlockLength, &max_block_opt_, options_[2]);
  int_option (urids.bufsz_nominalBlockLength, &nominal_block_opt_, options_[3]);
  int_option (urids.bufsz_sequenceSize, &sequence_size_opt_, options_[4]);
  options_[5] = LV2_Options_Option{};

  features_.clear ();
  feature_ptrs_.clear ();
  const auto push_feature = [this] (const char * uri, void * data) {
    features_.push_back (LV2_Feature{ uri, data });
  };
  push_feature (LV2_URID__map, &urid_map_feature_);
  push_feature (LV2_URID__unmap, &urid_unmap_feature_);
  push_feature (LV2_OPTIONS__options, options_.data ());
  push_feature (LV2_BUF_SIZE__boundedBlockLength, nullptr);
  push_feature (LV2_CORE__hardRTCapable, nullptr);
  for (const auto &feature : features_)
    {
      feature_ptrs_.push_back (&feature);
    }
  feature_ptrs_.push_back (nullptr);

  instance_ = lilv_plugin_instantiate (
    plugin_, sample_rate.in (units::sample_rate), feature_ptrs_.data ());
  if (instance_ == nullptr)
    {
      const auto is_feature_supported = [] (const char * uri) {
        return std::string_view{ uri } == LV2_URID__map
               || std::string_view{ uri } == LV2_URID__unmap
               || std::string_view{ uri } == LV2_OPTIONS__options
               || std::string_view{ uri } == LV2_BUF_SIZE__boundedBlockLength
               || std::string_view{ uri } == LV2_CORE__hardRTCapable;
      };
      std::string         unsupported_features;
      const LilvNodesUPtr required_features{
        lilv_plugin_get_required_features (plugin_)
      };
      if (required_features != nullptr)
        {
          LILV_FOREACH (nodes, iter, required_features.get ())
            {
              const char * uri = lilv_node_as_uri (
                lilv_nodes_get (required_features.get (), iter));
              if (uri != nullptr && !is_feature_supported (uri))
                {
                  unsupported_features += fmt::format (" {}", uri);
                }
            }
        }
      if (!unsupported_features.empty ())
        {
          z_warning (
            "LV2: failed to instantiate '{}': it requires features this "
            "host does not provide:{}",
            owner_.get_name (), unsupported_features);
        }
      else
        {
          z_warning ("LV2: failed to instantiate '{}'", owner_.get_name ());
        }
      return false;
    }

  atom_in_buf_.assign (atom_in_capacity, 0);
  time_in_buf_.assign (time_in_capacity, 0);
  atom_out_buf_.assign (atom_out_capacity, 0);

  // The forge caches the URIDs it stamps into atoms at init time
  lv2_atom_forge_init (&forge_, &urid_map_feature_);

  resize_scratch_buffers (max_block_length);
  const auto block_stride =
    static_cast<std::ptrdiff_t> (scratch_block_stride_.in (units::samples));

  // Every port is connected before activation: routed atom ports to
  // their dedicated buffers, unroutable atom ports (and unknown-type
  // ports) to private scratch buffers holding an empty sequence, and
  // audio/CV ports to their zeroed scratch slots (routed engine buffers
  // replace them per chunk)
  size_t atom_scratch_bytes = 0;
  for (const auto &port : ports_)
    {
      if (port.has_atom_scratch)
        {
          atom_scratch_bytes += atom_port_capacity (port);
        }
    }
  atom_scratch_buf_.assign (atom_scratch_bytes, 0);
  for (auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          lilv_instance_connect_port (
            instance_, port.index,
            audio_scratch_buf_.data ()
              + static_cast<std::ptrdiff_t> (port.audio_scratch_slot)
                  * block_stride);
          break;
        case PortInfo::Type::CV:
          lilv_instance_connect_port (
            instance_, port.index,
            cv_scratch_buf_.data ()
              + static_cast<std::ptrdiff_t> (port.cv_scratch_slot) * block_stride);
          break;
        default:
          if (!port.has_atom_scratch)
            break;
          auto * scratch =
            atom_scratch_buf_.data () + port.atom_scratch_byte_offset;
          lv2_atom_forge_set_buffer (
            &forge_, scratch, atom_port_capacity (port));
          LV2_Atom_Forge_Frame seq_frame;
          lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
          lv2_atom_forge_pop (&forge_, &seq_frame);
          connect_port_rt (port.index, scratch);
          break;
        }
    }

  connect_control_ports ();
  if (midi_out_port_idx_ >= 0)
    {
      lilv_instance_connect_port (
        instance_, ports_[midi_out_port_idx_].index, atom_out_buf_.data ());
    }

  lilv_instance_activate (instance_);
  return true;
}

void
Lv2Plugin::Lv2PluginImpl::resize_scratch_buffers (
  units::sample_u32_t max_block_length)
{
  const auto block_length = max_block_length.in<size_t> (units::samples);
  const auto count_ports = [this] (PortInfo::Type type) {
    return static_cast<size_t> (std::ranges::count_if (
      ports_, [type] (const auto &p) { return p.type == type; }));
  };
  audio_scratch_buf_.assign (
    count_ports (PortInfo::Type::Audio) * block_length, 0.f);
  cv_scratch_buf_.assign (count_ports (PortInfo::Type::CV) * block_length, 0.f);
  scratch_block_stride_ = max_block_length;
}

void
Lv2Plugin::Lv2PluginImpl::connect_control_ports ()
{
  for (const auto &port : ports_)
    {
      if (port.type != PortInfo::Type::Control)
        continue;
      auto &buffers =
        port.flow == dsp::PortFlow::Input ? control_in_bufs_ : control_out_bufs_;
      z_return_if_fail (port.control_buffer_index < buffers.size ());
      auto &buffer = buffers[port.control_buffer_index];
      lilv_instance_connect_port (instance_, port.index, &buffer);
    }
}
void
Lv2Plugin::apply_pending_state ()
{
  if (state_to_apply_.has_value () && pimpl_->instance_ != nullptr)
    {
      if (pimpl_->restore_state_from_string (state_to_apply_->toStdString ()))
        {
          state_to_apply_.reset ();
        }
      else
        {
          // Keep the pending state: it is retried on the next processing
          // preparation
          z_warning (
            "LV2: failed to apply the state of '{}'; it will be retried "
            "on the next processing preparation",
            get_name ());
        }
    }
}

void
Lv2Plugin::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  assert (QThread::currentThread () == thread ());

  if (pimpl_->plugin_ == nullptr)
    return;

  if (pimpl_->degradation_reported_.exchange (false, std::memory_order_relaxed))
    {
      z_warning (
        "LV2: '{}' received blocks larger than the maximum block length "
        "its instance was created for; they were dropped",
        get_name ());
    }

  if (
    const auto dropped_events =
      pimpl_->atom_events_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} input MIDI events that did not fit the "
        "sequence buffer",
        get_name (), dropped_events);
    }

  if (
    const auto dropped_output_events =
      pimpl_->midi_output_events_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_output_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} plugin output MIDI events because the "
        "engine MIDI buffer was full",
        get_name (), dropped_output_events);
    }

  // LV2 binds the sample rate and the buffer-size options at
  // instantiation: a change requires a new instance, carrying the current
  // state over. The scratch buffers are resized inside instantiate(),
  // together with the stride recorded for them.
  const bool instance_stale =
    pimpl_->instance_ != nullptr
    && (pimpl_->last_sample_rate_ != sample_rate || pimpl_->last_max_block_length_ != max_block_length);

  std::optional<std::string> carried_state;
  const auto prepare = [this, sample_rate, max_block_length, &carried_state] () {
    if (pimpl_->instance_ != nullptr)
      {
        carried_state = pimpl_->save_state_to_string ();
        pimpl_->free_instance ();
        z_debug (
          "LV2: re-instantiating '{}' at {} Hz with a maximum block "
          "length of {}",
          get_name (), sample_rate.in (units::sample_rate),
          max_block_length.in (units::samples));
      }

    if (pimpl_->instance_ == nullptr)
      {
        if (!pimpl_->instantiate (sample_rate, max_block_length))
          {
            pimpl_->last_sample_rate_ = sample_rate;
            pimpl_->last_max_block_length_ = max_block_length;
            // Keep the state of the freed instance as pending so it is
            // not lost; it is applied when instantiation succeeds
            if (carried_state.has_value ())
              {
                state_to_apply_ = QByteArray::fromStdString (*carried_state);
              }
            // The plugin was resolved and reported loaded earlier:
            // surface the late instantiation failure to listeners
            Q_EMIT instantiationFinished (false, tr ("Instantiation failed"));
            return;
          }

        const auto state_to_restore = [&] () -> std::optional<std::string> {
          if (carried_state.has_value ())
            return carried_state;
          if (state_to_apply_.has_value ())
            return state_to_apply_->toStdString ();
          return std::nullopt;
        }();
        if (state_to_restore.has_value ())
          {
            if (pimpl_->restore_state_from_string (*state_to_restore))
              {
                state_to_apply_.reset ();
                carried_state.reset ();
              }
            else
              {
                // Keep the state as pending: it is retried on the next
                // processing preparation
                state_to_apply_ = QByteArray::fromStdString (*state_to_restore);
                z_warning (
                  "LV2: failed to restore the state of '{}'; it will be "
                  "retried on the next processing preparation",
                  get_name ());
              }
          }
      }

    pimpl_->pair_ports_with_engine_ports ();
    pimpl_->last_sample_rate_ = sample_rate;
    pimpl_->last_max_block_length_ = max_block_length;
  };

  // Freeing and re-creating a live instance must not overlap audio
  // processing of the same instance
  if (instance_stale)
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ (prepare);
        }
      else
        {
          // Keep the current instance; the next processing preparation
          // retries the re-instantiation
          z_warning (
            "LV2: cannot re-instantiate '{}' while processing; the host "
            "cannot pause processing",
            get_name ());
        }
      return;
    }

  if (pimpl_->instance_ == nullptr)
    {
      prepare ();
      return;
    }

  // Live instance at an unchanged rate and block length: retry any
  // pending state. Restoring writes the control buffers of a running
  // instance, so it is paused
  if (state_to_apply_.has_value ())
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ ([this] () {
            apply_pending_state ();
          });
        }
      else
        {
          z_warning (
            "LV2: cannot retry applying the state of '{}' while "
            "processing; the host cannot pause processing",
            get_name ());
        }
    }

  pimpl_->pair_ports_with_engine_ports ();
  pimpl_->last_sample_rate_ = sample_rate;
  pimpl_->last_max_block_length_ = max_block_length;
}

void
Lv2Plugin::release_resources_impl ()
{
  // Plugin-internal state (LV2 State extension data) cannot survive the
  // instance being freed: snapshot it as pending so it is re-applied at
  // the next processing preparation
  if (pimpl_->instance_ != nullptr)
    {
      if (const auto state = pimpl_->save_state_to_string (); state.has_value ())
        {
          state_to_apply_ = QByteArray::fromStdString (*state);
        }
      else
        {
          z_warning (
            "LV2: failed to snapshot the state of '{}' before releasing "
            "its resources; plugin-internal state is lost",
            get_name ());
        }
    }
  pimpl_->free_instance ();
}

void
Lv2Plugin::Lv2PluginImpl::pair_ports_with_engine_ports ()
{
  // Audio buses are matched with engine ports by external id (the bus's
  // first port index); each port of ports_ carries its bus's id. Buses
  // with no paired engine port are routed to scratch buffers
  const auto pair_audio = [this] (dsp::PortFlow flow, auto &by_bus) {
    by_bus.assign (audio_bus_groups (flow).size (), nullptr);
    const auto attached =
      owner_.get_attached_audio_ports (flow) | std::ranges::to<std::vector> ();
    for (const auto &port_info : ports_)
      {
        if (port_info.type != PortInfo::Type::Audio || port_info.flow != flow)
          continue;
        const auto it = std::ranges::find_if (
          attached, [&port_info] (const dsp::AudioPort * engine_port) {
            return engine_port->external_port_id ()
                   == port_info.audio_bus_external_id;
          });
        by_bus[port_info.audio_bus_index] =
          it != attached.end () ? *it : nullptr;
      }
    if (std::ranges::contains (by_bus, nullptr) && !unroutable_ports_warned_)
      {
        unroutable_ports_warned_ = true;
        z_warning (
          "LV2: '{}' has audio buses with no paired engine port; they are "
          "routed to silent scratch buffers",
          owner_.get_name ());
      }
  };
  pair_audio (dsp::PortFlow::Input, audio_in_ports_by_bus_);
  pair_audio (dsp::PortFlow::Output, audio_out_ports_by_bus_);

  // CV ports have no stable id: they pair positionally with the attached CV
  // ports in creation order
  const auto pair_cv = [this] (dsp::PortFlow flow, auto &by_port) {
    const auto cv_ports = [this, flow] {
      const auto attached =
        flow == dsp::PortFlow::Input
          ? owner_.get_attached_input_ports () | std::ranges::to<std::vector> ()
          : owner_.get_attached_output_ports () | std::ranges::to<std::vector> ();
      return attached | std::views::transform (&dsp::PortUuidReference::get)
             | utils::views::qobject_cast_and_filter<dsp::CVPort>
             | std::ranges::to<std::vector> ();
    }();
    by_port.clear ();
    size_t pos = 0;
    for (const auto &port_info : ports_)
      {
        if (port_info.type != PortInfo::Type::CV || port_info.flow != flow)
          continue;
        by_port.push_back (pos < cv_ports.size () ? cv_ports[pos] : nullptr);
        ++pos;
      }
    if (std::ranges::contains (by_port, nullptr) && !unroutable_ports_warned_)
      {
        unroutable_ports_warned_ = true;
        z_warning (
          "LV2: '{}' has CV ports with no paired engine port; they are "
          "routed to scratch buffers",
          owner_.get_name ());
      }
  };
  pair_cv (dsp::PortFlow::Input, cv_in_ports_by_port_);
  pair_cv (dsp::PortFlow::Output, cv_out_ports_by_port_);
}

// ============================================================================
// Processing
// ============================================================================

void
Lv2Plugin::Lv2PluginImpl::connect_sample_ports (
  units::sample_u32_t local_offset) noexcept
{
  const auto offset = local_offset.in<std::ptrdiff_t> (units::samples);
  const auto block_stride =
    static_cast<std::ptrdiff_t> (scratch_block_stride_.in (units::samples));

  for (const auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          {
            const auto &by_port =
              port.flow == dsp::PortFlow::Input
                ? audio_in_ports_by_bus_
                : audio_out_ports_by_bus_;
            auto * engine_port =
              port.audio_bus_index < by_port.size ()
                ? by_port[port.audio_bus_index]
                : nullptr;
            float * data = nullptr;
            if (
              engine_port != nullptr && engine_port->buffers () != nullptr
              && port.audio_channel < static_cast<size_t> (
                   engine_port->buffers ()->getNumChannels ()))
              {
                data = engine_port->buffers ()->getWritePointer (
                  static_cast<int> (port.audio_channel));
              }
            else
              {
                // Ports without a routed engine buffer are connected to
                // their zeroed scratch slot
                data =
                  audio_scratch_buf_.data ()
                  + static_cast<std::ptrdiff_t> (port.audio_scratch_slot)
                      * block_stride;
              }
            connect_port_rt (port.index, data + offset);
            break;
          }
        case PortInfo::Type::CV:
          {
            const auto &by_port =
              port.flow == dsp::PortFlow::Input
                ? cv_in_ports_by_port_
                : cv_out_ports_by_port_;
            auto * engine_port =
              port.cv_index_in_flow < by_port.size ()
                ? by_port[port.cv_index_in_flow]
                : nullptr;
            float * data =
              engine_port != nullptr && !engine_port->buf_.empty ()
                ? engine_port->buf_.data ()
                : cv_scratch_buf_.data ()
                    + static_cast<std::ptrdiff_t> (port.cv_scratch_slot)
                        * block_stride;
            connect_port_rt (port.index, data + offset);
            break;
          }
        default:
          break;
        }
    }
}

void
Lv2Plugin::process_impl (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  if (pimpl_->instance_ == nullptr)
    return;

  const auto local_offset = time_info.buffer_offset_;
  const auto nframes = time_info.nframes_;

  // The instance and its scratch buffers were created for the recorded
  // maximum block length: running it with a longer block writes past
  // those buffers, so the block is dropped
  if (local_offset + nframes > pimpl_->scratch_block_stride_)
    {
      pimpl_->degrade_to_silence (local_offset, nframes);
      return;
    }

  pimpl_->apply_changed_param_values ();
  pimpl_->connect_sample_ports (local_offset);
  pimpl_->forge_atom_inputs (time_info, transport, tempo_map);
  pimpl_->reset_atom_output ();

  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // Plugin code is not ours; RTSan violations inside it are not actionable
    __rtsan::ScopedDisabler d;
#endif
    lilv_instance_run (pimpl_->instance_, nframes.in<uint32_t> (units::samples));
  }

  pimpl_->parse_atom_outputs (local_offset, nframes);
  pimpl_->read_control_outputs ();
}

void
Lv2Plugin::Lv2PluginImpl::forge_atom_inputs (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto &urids = host_urids_;
  const auto  local_offset = time_info.buffer_offset_;
  const auto  nframes = time_info.nframes_;

  // Forges the position object as a frame-timed event of the sequence
  // currently open in the forge, at frame 0 so it sorts before the
  // chunk's MIDI events. Its size is fixed and far below the buffer
  // capacity, so its forge writes cannot fail
  const auto forge_position_event = [&] {
    const auto transport_context = build_plugin_transport_context (
      transport, tempo_map, units::sample_t{ time_info.transport_position_ });
    lv2_atom_forge_frame_time (&forge_, 0);
    LV2_Atom_Forge_Frame pos_frame;
    lv2_atom_forge_object (&forge_, &pos_frame, 0, urids.time_Position);
    lv2_atom_forge_key (&forge_, urids.time_speed);
    lv2_atom_forge_float (&forge_, transport_context.playing_ ? 1.f : 0.f);
    lv2_atom_forge_key (&forge_, urids.time_frame);
    lv2_atom_forge_long (
      &forge_,
      static_cast<int64_t> (time_info.transport_position_.in (units::samples)));
    lv2_atom_forge_key (&forge_, urids.time_framesPerSecond);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (last_sample_rate_.in (units::sample_rate)));
    lv2_atom_forge_key (&forge_, urids.time_bar);
    lv2_atom_forge_long (&forge_, transport_context.bar_number_);
    lv2_atom_forge_key (&forge_, urids.time_barBeat);
    // Typed as Float, which is what shipped hosts forge, though the time
    // spec types the property xsd:double
    const auto quarters_into_bar =
      (transport_context.position_ - transport_context.bar_start_)
        .in (units::quarter_notes);
    lv2_atom_forge_float (
      &forge_,
      static_cast<float> (
        quarters_into_bar * transport_context.time_sig_denominator_
        / kQuartersPerWholeNote));
    lv2_atom_forge_key (&forge_, urids.time_beatsPerBar);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (transport_context.time_sig_numerator_));
    lv2_atom_forge_key (&forge_, urids.time_beatUnit);
    lv2_atom_forge_int (&forge_, transport_context.time_sig_denominator_);
    lv2_atom_forge_key (&forge_, urids.time_beatsPerMinute);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (transport_context.tempo_.in (units::bpm)));
    lv2_atom_forge_pop (&forge_, &pos_frame);
  };

  if (time_in_port_idx_ >= 0 && time_in_port_idx_ != midi_in_port_idx_)
    {
      // A time:Position port of its own receives its own sequence
      lv2_atom_forge_set_buffer (
        &forge_, time_in_buf_.data (),
        static_cast<uint32_t> (time_in_buf_.size ()));
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
      forge_position_event ();
      lv2_atom_forge_pop (&forge_, &seq_frame);
      connect_port_rt (ports_[time_in_port_idx_].index, time_in_buf_.data ());
    }

  if (midi_in_port_idx_ >= 0)
    {
      // The buffer is forged and connected even with no routed MIDI port:
      // the plugin reads the connected sequence every run(). A port
      // supporting both MIDI and time:Position receives the position
      // object and the MIDI events in the same sequence
      lv2_atom_forge_set_buffer (
        &forge_, atom_in_buf_.data (),
        static_cast<uint32_t> (atom_in_buf_.size ()));
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
      if (time_in_port_idx_ == midi_in_port_idx_)
        forge_position_event ();
      const auto in_chunk = [local_offset, nframes] (const auto &ev) {
        return ev.time () >= local_offset && ev.time () < local_offset + nframes;
      };
      // Events that do not fit the buffer are skipped whole: a partially
      // forged event would claim body bytes that were never written, and
      // plugins would read past the buffer parsing them
      uint32_t dropped_events = 0;
      if (!owner_.midi_in_ports_.empty ())
        {
          for (
            const auto &ev :
            owner_.midi_in_ports_.front ()->buffer_
              | std::views::filter (in_chunk))
            {
              const auto chunk_time =
                (ev.time () - local_offset).in<uint32_t> (units::samples);
              const auto ev_data = ev.data ();
              const auto needed =
                sizeof (LV2_Atom_Event)
                + lv2_atom_pad_size (static_cast<uint32_t> (ev_data.size ()));
              if (forge_.offset + needed > forge_.size)
                {
                  ++dropped_events;
                  continue;
                }
              lv2_atom_forge_frame_time (&forge_, chunk_time);
              lv2_atom_forge_atom (
                &forge_, ev_data.size (), urids.midi_MidiEvent);
              lv2_atom_forge_write (&forge_, ev_data.data (), ev_data.size ());
            }
        }
      if (dropped_events > 0)
        {
          atom_events_dropped_.fetch_add (
            dropped_events, std::memory_order_relaxed);
        }
      lv2_atom_forge_pop (&forge_, &seq_frame);
      connect_port_rt (ports_[midi_in_port_idx_].index, atom_in_buf_.data ());
    }
}

void
Lv2Plugin::Lv2PluginImpl::reset_atom_output () noexcept
{
  // LV2 contract for output atom ports (including legacy event ports
  // routed to scratch): the host announces the buffer capacity by
  // writing an atom:Chunk header before each run, and the plugin
  // replaces it with the sequence it produces
  for (const auto &port : ports_)
    {
      if (
        (port.type != PortInfo::Type::Atom
         && port.type != PortInfo::Type::Unknown)
        || port.flow != dsp::PortFlow::Output)
        continue;
      const auto ports_idx = static_cast<int32_t> (&port - ports_.data ());
      const auto routed = ports_idx == midi_out_port_idx_;
      auto *     buf =
        routed
          ? atom_out_buf_.data ()
          : atom_scratch_buf_.data () + port.atom_scratch_byte_offset;
      auto * atom = reinterpret_cast<LV2_Atom *> (buf);
      atom->type = host_urids_.atom_Chunk;
      atom->size =
        routed
          ? static_cast<uint32_t> (atom_out_buf_.size ())
          : atom_port_capacity (port);
    }
}

void
Lv2Plugin::Lv2PluginImpl::parse_atom_outputs (
  units::sample_u32_t local_offset,
  units::sample_u32_t nframes) noexcept
{
  if (midi_out_port_idx_ < 0 || owner_.midi_out_ports_.empty ())
    return;

  const auto  &urids = host_urids_;
  const auto * seq =
    reinterpret_cast<const LV2_Atom_Sequence *> (atom_out_buf_.data ());
  if (seq->atom.type != urids.atom_Sequence)
    return;

  // The sequence header is plugin-controlled: only accept sizes that
  // fit the buffer the host owns (integer bounds first: the end
  // pointer must not be formed from a hostile size) and only frame-timed
  // sequences (the unit URID of a beat-timed sequence would be misread
  // as frames); malformed data is dropped (this runs on the audio
  // thread, which cannot log)
  if (
    seq->atom.size < sizeof (LV2_Atom_Sequence_Body)
    || seq->atom.size > atom_out_buf_.size () - sizeof (LV2_Atom)
    || seq->body.unit != 0)
    return;
  const auto * seq_end =
    reinterpret_cast<const uint8_t *> (seq) + sizeof (LV2_Atom) + seq->atom.size;

  auto *   midi_out_port = owner_.midi_out_ports_.front ();
  uint32_t dropped_events = 0;
  LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
  {
    if (
      reinterpret_cast<const uint8_t *> (ev) + sizeof (LV2_Atom_Event)
        + ev->body.size
      > seq_end)
      break;
    if (ev->body.type != urids.midi_MidiEvent)
      continue;
    // Events must land inside this chunk: negative times and times past
    // the chunk end carry stale timestamps
    if (
      ev->time.frames < 0
      || ev->time.frames
           >= static_cast<int64_t> (nframes.in<uint32_t> (units::samples)))
      continue;
    const auto time =
      units::samples (static_cast<uint32_t> (ev->time.frames)) + local_offset;
    const auto * data =
      reinterpret_cast<const midi_byte_t *> (LV2_ATOM_BODY_CONST (&ev->body));
    if (!midi_out_port->buffer_.push_back (
          time, std::span<const midi_byte_t> (data, ev->body.size)))
      {
        ++dropped_events;
      }
  }
  if (dropped_events > 0)
    {
      midi_output_events_dropped_.fetch_add (
        dropped_events, std::memory_order_relaxed);
    }
}

void
Lv2Plugin::Lv2PluginImpl::apply_changed_param_values () noexcept
{
  for (const auto &change : owner_.change_tracker ().changes ())
    {
      if (change.index >= param_to_ctrl_in_.size ())
        continue;
      const auto ctrl_in = param_to_ctrl_in_[change.index];
      if (ctrl_in < 0)
        continue;
      const auto &ctrl_param = ctrl_in_params_[static_cast<size_t> (ctrl_in)];
      control_in_bufs_[static_cast<size_t> (ctrl_in)] =
        control_value_for_param (ctrl_param, change.modulated_value);
    }
}

void
Lv2Plugin::Lv2PluginImpl::read_control_outputs () noexcept
{
  if (latency_buf_index_ < 0)
    return;

  // The finite check rejects non-finite reports with defined behavior
  // in default builds; fast-math builds fold it away, where the integer
  // bounds below still reject lround's mapping of non-finite input. A
  // latency report is a non-negative sample count that fits 32 bits
  const auto reported_value =
    control_out_bufs_[static_cast<size_t> (latency_buf_index_)];
  if (!std::isfinite (reported_value))
    return;
  const auto reported = static_cast<int64_t> (std::lround (reported_value));
  if (reported < 0 || reported > std::numeric_limits<int32_t>::max ())
    return;
  const auto new_latency = units::samples (static_cast<uint32_t> (reported));
  if (new_latency != latency_.load (std::memory_order_relaxed))
    {
      latency_.store (new_latency, std::memory_order_relaxed);
      owner_.notify_latency_changed ();
    }
}

void
Lv2Plugin::Lv2PluginImpl::degrade_to_silence (
  units::sample_u32_t local_offset,
  units::sample_u32_t nframes) noexcept
{
  // Only the dropped chunk's window is zeroed: output earlier chunks of
  // the same cycle wrote must survive
  const auto offset = local_offset.in<size_t> (units::samples);
  const auto length = nframes.in<size_t> (units::samples);
  for (auto * port : audio_out_ports_by_bus_)
    {
      if (port == nullptr || port->buffers () == nullptr)
        continue;
      const auto num_samples =
        static_cast<size_t> (port->buffers ()->getNumSamples ());
      if (offset >= num_samples)
        continue;
      port->buffers ()->clear (
        static_cast<int> (offset),
        static_cast<int> (std::min (length, num_samples - offset)));
    }
  for (auto * port : cv_out_ports_by_port_)
    {
      if (port == nullptr || offset >= port->buf_.size ())
        continue;
      std::fill_n (
        port->buf_.begin () + static_cast<std::ptrdiff_t> (offset),
        std::min (length, port->buf_.size () - offset), 0.f);
    }
  degradation_reported_.store (true, std::memory_order_relaxed);
}

units::sample_u32_t
Lv2Plugin::get_single_playback_latency () const
{
  return pimpl_->latency_.load (std::memory_order_relaxed);
}

// ============================================================================
// State
// ============================================================================

std::optional<std::string>
Lv2Plugin::Lv2PluginImpl::save_state_to_string ()
{
  if (instance_ == nullptr || plugin_ == nullptr)
    return std::nullopt;

  const LilvStateUPtr state{ lilv_state_new_from_instance (
    plugin_, instance_, &urid_map_feature_, nullptr, nullptr, nullptr, nullptr,
    &Lv2PluginImpl::state_get_port_value, this,
    LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, feature_ptrs_.data ()) };
  if (state == nullptr)
    {
      z_warning (
        "LV2: failed to snapshot the state of '{}'", owner_.get_name ());
      return std::nullopt;
    }

  // The state TTL is keyed by the plugin URI, so it must be passed as the
  // state subject (lilv_plugin_get_uri returns a borrowed node)
  const LilvNode *      plugin_uri = lilv_plugin_get_uri (plugin_);
  const LilvCharPtrUPtr ttl{ lilv_state_to_string (
    owner_.world_->raw (), &urid_map_feature_, &urid_unmap_feature_,
    state.get (), lilv_node_as_uri (plugin_uri), nullptr) };
  if (ttl == nullptr)
    {
      z_warning (
        "LV2: failed to serialize the state of '{}'", owner_.get_name ());
      return std::nullopt;
    }
  return std::string (ttl.get ());
}

std::string
Lv2Plugin::save_state_impl () const
{
  const auto ttl = pimpl_->save_state_to_string ();
  if (!ttl.has_value ())
    {
      // Not instantiated: keep any pending state instead of dropping it
      if (state_to_apply_.has_value ())
        return utils::to_std_string (state_to_apply_->toBase64 ());
      return {};
    }
  return utils::to_std_string (QByteArray::fromStdString (*ttl).toBase64 ());
}

bool
Lv2Plugin::Lv2PluginImpl::restore_state_from_string (
  const std::string &ttl_state)
{
  if (instance_ == nullptr || plugin_ == nullptr)
    return false;

  const LilvStateUPtr state{ lilv_state_new_from_string (
    owner_.world_->raw (), &urid_map_feature_, ttl_state.c_str ()) };
  if (state == nullptr)
    {
      z_warning ("LV2: failed to parse the state of '{}'", owner_.get_name ());
      return false;
    }

  lilv_state_restore (
    state.get (), instance_, state_set_port_value, this, 0,
    feature_ptrs_.data ());
  return true;
}

void
Lv2Plugin::Lv2PluginImpl::state_set_port_value (
  const char * port_symbol,
  void *       user_data,
  const void * value,
  uint32_t     size,
  uint32_t     type)
{
  auto * impl = static_cast<Lv2PluginImpl *> (user_data);

  // The state blob comes from a saved project file and may hold any
  // literal type: convert the numeric ones and reject everything else
  const auto &urids = impl->host_urids_;
  const auto  new_value = [&] () -> std::optional<float> {
    if (type == urids.atom_Float && size == sizeof (float))
      return *static_cast<const float *> (value);
    if (type == urids.atom_Double && size == sizeof (double))
      return static_cast<float> (*static_cast<const double *> (value));
    if (type == urids.atom_Int && size == sizeof (int32_t))
      return static_cast<float> (*static_cast<const int32_t *> (value));
    if (type == urids.atom_Long && size == sizeof (int64_t))
      return static_cast<float> (*static_cast<const int64_t *> (value));
    return std::nullopt;
  }();
  if (!new_value.has_value ())
    {
      z_warning (
        "LV2: state value of port '{}' of '{}' has an unsupported type or "
        "size; skipping it",
        port_symbol, impl->owner_.get_name ());
      return;
    }

  const auto * port_info = impl->find_control_input (port_symbol);
  if (port_info == nullptr)
    return;

  const auto buf_index = port_info->control_buffer_index;
  impl->control_in_bufs_[buf_index] = *new_value;
  const auto &ctrl_param = impl->ctrl_in_params_[buf_index];
  if (ctrl_param.param != nullptr)
    {
      ctrl_param.param->setBaseValue (
        impl->param_value_0_to_1_for_control (ctrl_param, *new_value));
    }
}

const void *
Lv2Plugin::Lv2PluginImpl::state_get_port_value (
  const char * port_symbol,
  void *       user_data,
  uint32_t *   size,
  uint32_t *   type)
{
  auto * impl = static_cast<Lv2PluginImpl *> (user_data);

  // The LV2 state specification allows save() to run concurrently with
  // run(); the control buffers are plain shared memory for this read
  const auto * port_info = impl->find_control_input (port_symbol);
  if (port_info == nullptr)
    return nullptr;

  *size = sizeof (float);
  *type = impl->host_urids_.atom_Float;
  return &impl->control_in_bufs_[port_info->control_buffer_index];
}

bool
Lv2Plugin::load_state_impl (const std::string &base64_state)
{
  const auto ttl =
    QByteArray::fromBase64 (QByteArray::fromStdString (base64_state))
      .toStdString ();
  if (ttl.empty ())
    return false;

  if (pimpl_->instance_ == nullptr)
    {
      // Not instantiated yet: applied during processing preparation
      state_to_apply_ = QByteArray::fromStdString (ttl);
      return true;
    }

  // LV2 state restore runs on the plugin instance and must not overlap
  // audio processing of the same instance
  if (main_thread_callbacks_.with_paused_processing_)
    {
      auto restored = false;
      main_thread_callbacks_.with_paused_processing_ ([&] () {
        restored = pimpl_->restore_state_from_string (ttl);
      });
      return restored;
    }

  z_warning (
    "LV2: cannot apply state to '{}' while processing; the host cannot "
    "pause processing",
    get_name ());
  return false;
}

void
to_json (nlohmann::json &j, const Lv2Plugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state_impl ();
  if (!state.empty ())
    j[Lv2Plugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, Lv2Plugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (Lv2Plugin::kStateKey))
    p.load_state_impl (j[Lv2Plugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
