// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/graph_node.h"
#include "dsp/midi_port.h"
#include "utils/math_utils.h"
#include "utils/traits.h"
#include "utils/units.h"
#include "utils/uuid_identifiable_object.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

class ParameterRange
{
  Q_GADGET
  QML_VALUE_TYPE (parameterRange)

public:
  enum class Type : std::uint8_t
  {
    /**
     * @brief Linearly-scaled float parameter.
     */
    Linear,

    /**
     * @brief Whether the port is a toggle (on/off).
     *
     * @see Trigger
     */
    Toggle,

    /** Whether the port is an integer. */
    Integer,

    /**
     * @brief Parameter is a gain amplitude (0-2.0)
     *
     * @note This is a special logarithmic-scaled parameter for gain
     * parameters.
     */
    GainAmplitude,

    /** Logarithmic-scaled float parameter. */
    Logarithmic,

    /** Port's only reasonable values are its scale points. */
    Enumeration,

    /**
     * Trigger parameters are set to on to trigger a change during
     * processing and then turned off at the end of each cycle.
     *
     * Used trigger eg, transport play/pause.
     */
    Trigger,
  };
  Q_ENUM (Type)

  /**
   * Unit to be displayed in the UI.
   */
  enum class Unit : std::uint8_t
  {
    None,
    Hz,
    MHz,
    Db,
    Degrees,
    Seconds,

    /** Milliseconds. */
    Ms,

    /** Microseconds. */
    Us,
  };

  static utils::Utf8String unit_to_string (Unit unit);

public:
  ParameterRange () = default;

  ParameterRange (Type type, float min, float max, float zero = 0.f, float def = 0.f)
      : type_ (type), minf_ (min), maxf_ (max),
        zerof_ (std::clamp (zero, min, max)), deff_ (std::clamp (def, min, max))
  {
  }

  static ParameterRange make_toggle (bool default_val)
  {
    return { Type::Toggle, 0.f, 1.f, 0.f, default_val ? 1.f : 0.f };
  }
  static ParameterRange make_gain (float max_val)
  {
    return { Type::GainAmplitude, 0.f, max_val, 0.f, 1.f };
  }

  static ParameterRange make_enumeration (
    std::vector<utils::Utf8String> labels,
    size_t                         default_index = 0)
  {
    if (labels.empty ())
      {
        throw std::invalid_argument (
          "Enumeration parameter requires at least one label");
      }
    if (default_index >= labels.size ())
      {
        throw std::invalid_argument (
          "default_index out of range for enumeration labels");
      }
    const auto     count = static_cast<float> (labels.size ());
    ParameterRange range{
      Type::Enumeration, 0.f, count - 1.f, 0.f,
      static_cast<float> (default_index)
    };
    range.enum_labels_ = std::move (labels);
    return range;
  }

  /**
   * @brief Returns the number of enum entries.
   */
  size_t enum_count () const { return enum_labels_.size (); }

  /**
   * @brief Converts a normalized value to an enum index.
   */
  size_t enum_index (float normalized_val) const
  {
    const auto real = convertFrom0To1 (normalized_val);
    assert (!enum_labels_.empty ());
    const auto idx = static_cast<size_t> (std::max (std::round (real), 0.f));
    return std::min (idx, enum_labels_.size () - 1);
  }

  /**
   * @brief Returns the normalized value for a given enum index.
   */
  float normalized_from_index (size_t index) const
  {
    assert (index < enum_labels_.size ());
    if (enum_labels_.size () == 1)
      return 0.f;
    return convertTo0To1 (static_cast<float> (index));
  }

  /**
   * @brief Returns the label for a given enum index.
   */
  const auto &enum_label (size_t index) const
  {
    return enum_labels_.at (index);
  }

  template <EnumType E> float normalized_from_enum (E value) const
  {
    assert (type_ == Type::Enumeration);
    const auto index = static_cast<size_t> (value);
    assert (index < enum_labels_.size ());
    return normalized_from_index (index);
  }

  template <EnumType E> E enum_value (float normalized_val) const
  {
    assert (type_ == Type::Enumeration);
    return static_cast<E> (enum_index (normalized_val));
  }

  constexpr float clamp_to_range (float val) const
  {
    return std::clamp (val, minf_, maxf_);
  }

  Q_INVOKABLE float convertFrom0To1 (float normalized_val) const;
  Q_INVOKABLE float convertTo0To1 (float real_val) const;

  bool is_toggled (float normalized_val) const
  {
    assert (type_ == ParameterRange::Type::Toggle);
    return utils::math::floats_equal (convertFrom0To1 (normalized_val), 1.f);
  }

  friend void to_json (nlohmann::json &j, const ParameterRange &p);
  friend void from_json (const nlohmann::json &j, ParameterRange &p);

public:
  Type type_{ Type::Linear };

  /** Parameter unit. */
  Unit unit_{};

  /**
   * Minimum, maximum and zero values for this parameter.
   */
  float minf_{ 0.f };
  float maxf_{ 1.f };

  /**
   * The zero position of the port.
   *
   * For example, in balance controls, this will be the middle.
   */
  float zerof_{ 0.f };

  /**
   * @brief Default value.
   */
  float deff_{ 0.f };

  /**
   * @brief Labels for Enumeration type parameters.
   */
  std::vector<utils::Utf8String> enum_labels_;

  BOOST_DESCRIBE_CLASS (
    ParameterRange,
    (),
    (type_, unit_, minf_, maxf_, zerof_, deff_),
    (),
    ())
};

/**
 * @brief Processor parameter that accepts automation and modulation sources and
 * integrates with QML and the DSP graph.
 *
 * While ProcessorParameter inherits from IProcessable, it is not part of the
 * DSP graph and is expected to be processed manually by its processor.
 */
class ProcessorParameter
    : public QObject,
      public dsp::graph::IProcessable,
      public utils::UuidIdentifiableObject<ProcessorParameter>
{
  Q_OBJECT
  Q_PROPERTY (
    float baseValue READ baseValue WRITE setBaseValue NOTIFY baseValueChanged)
  Q_PROPERTY (QString label READ label CONSTANT)
  Q_PROPERTY (QString description READ description CONSTANT)
  Q_PROPERTY (zrythm::dsp::ParameterRange range READ range CONSTANT)
  Q_PROPERTY (bool automatable READ automatable CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  struct UniqueId final
      : type_safe::strong_typedef<UniqueId, utils::Utf8String>,
        type_safe::strong_typedef_op::equality_comparison<UniqueId>,
        type_safe::strong_typedef_op::relational_comparison<UniqueId>
  {
    using type_safe::strong_typedef<UniqueId, utils::Utf8String>::strong_typedef;

    explicit UniqueId () = default;

    static_assert (StrongTypedef<UniqueId>);

    std::size_t hash () const { return qHash (type_safe::get (*this).view ()); }
  };
  static_assert (std::regular<UniqueId>);

  using Resolver =
    std::function<QPointer<ProcessorParameter> (const UniqueId &unique_id)>;

  ProcessorParameter (
    PortRegistry     &port_registry,
    UniqueId          unique_id,
    ParameterRange    range,
    utils::Utf8String label,
    QObject *         parent = nullptr);

  /**
   * @brief Provides the automation value for a given sample position.
   *
   * @param sample_position The sample position in the timeline to get the
   * automation value for.
   *
   * This allows for flexibility in how automation is obtained (e.g. call once
   * to get the automation value for the entire cycle for efficiency, or call for
   * each sample position for sample-accurate automation), or every few samples.
   *
   * @return The normalized automation value (0.0-1.0) at the given sample
   * position, or std::nullopt if no automation is available.
   */
  using AutomationValueProvider =
    std::function<std::optional<float> (units::sample_t sample_position)>;

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString label () const { return label_.to_qstring (); }
  QString description () const { return description_->to_qstring (); }
  bool    automatable () const { return automatable_; }

  const auto &range () const { return range_; }

  float baseValue () const { return base_value_.load (); }
  void  setBaseValue (float newValue) [[clang::blocking]]
  {
    newValue = std::clamp (newValue, 0.f, 1.f);
    if (qFuzzyCompare (base_value_, newValue))
      return;
    base_value_ = newValue;
    Q_EMIT baseValueChanged (newValue);
  }
  Q_INVOKABLE void resetBaseValueToDefault ()
  {
    setBaseValue (range_.convertTo0To1 (range_.deff_));
  }
  Q_SIGNAL void baseValueChanged (float value);

  /**
   * @brief Returns the current (normalized) value after any automation and
   * modulation has been applied.
   */
  Q_INVOKABLE float currentValue () const { return last_modulated_value_; }

  /**
   * @brief Returns the value after automation, but before modulation has been
   * applied.
   *
   * Intended to be used in the UI.
   */
  Q_INVOKABLE float valueAfterAutomationApplied () const
  {
    return last_automated_value_.load ();
  }

  Q_INVOKABLE void beginUserGesture ()
  {
    during_gesture_.store (true);
    Q_EMIT userGestureStarted ();
  }
  Q_INVOKABLE void endUserGesture ()
  {
    during_gesture_.store (false);
    Q_EMIT userGestureFinished ();
  }
  Q_SIGNAL void userGestureStarted ();
  Q_SIGNAL void userGestureFinished ();

  // ========================================================================

  // ========================================================================
  // IProcessable Implementation
  // ========================================================================

  utils::Utf8String get_node_name () const override;

  /**
   * @brief Processes automation and modulation.
   *
   * Since parameters are not part of the graph, this should be spinned manually
   * by the owning processor.
   */
  void process_block (
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap              &tempo_map) noexcept override;

  void prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override;
  void release_resources () override;

  // ========================================================================

  void set_automation_provider (AutomationValueProvider provider)
  {
    automation_value_provider_ = provider;
  }
  void unset_automation_provider () { automation_value_provider_.reset (); }

  PortUuidReference get_modulation_input_port_ref () const
  {
    return modulation_input_uuid_;
  }

  void set_description (utils::Utf8String descr)
  {
    description_ = std::move (descr);
  }

  void set_automatable (bool automatable) { automatable_ = automatable; }

  bool hidden () const { return hidden_; }

  const auto &get_unique_id () const { return unique_id_; }

private:
  // Serialization keys
  static constexpr auto kUniqueIdKey = "uniqueId"sv;
  static constexpr auto kRangeKey = "range"sv;
  static constexpr auto kLabelKey = "label"sv;
  static constexpr auto kSymbolKey = "symbol"sv;
  static constexpr auto kDescriptionKey = "description"sv;
  static constexpr auto kAutomatableKey = "automatable"sv;
  static constexpr auto kHiddenKey = "hidden"sv;
  static constexpr auto kBaseValueKey = "baseValue"sv;
  static constexpr auto kModulationSourcePortIdKey = "modulationSourcePortId"sv;
  friend void to_json (nlohmann::json &j, const ProcessorParameter &p);
  friend void from_json (const nlohmann::json &j, ProcessorParameter &p);

private:
  /**
   * @brief Unique ID of this parameter.
   */
  UniqueId unique_id_;

  /**
   * @brief Parameter range (min, max, default, etc.).
   *
   * This member must not be mutated while the audio engine is active (i.e.,
   * while process_block() may be called concurrently). It is only written to
   * during construction and during project deserialization (from_json), both
   * of which occur while the engine is stopped.
   */
  ParameterRange range_;

  /** Human readable label. */
  utils::Utf8String label_;

  /**
   * @brief Base (normalized) value set by the user in the UI.
   */
  std::atomic<float> base_value_;

  /**
   * @brief Whether we are currently doing a user gesture.
   *
   * If true, automation and modulation will be skipped.
   */
  std::atomic_bool during_gesture_;

  /**
   * @brief Cached value of the base value after automation and before
   * modulation is applied.
   *
   * @warning Only intended to be used for UI integration (read-only). Do not
   * use for any other purpose.
   */
  std::atomic<float> last_automated_value_;

  /**
   * @brief Cached value of the base value after automation and modulation are
   * applied.
   *
   * @warning Only intended to be used for UI integration (read-only). Do not
   * use for any other purpose.
   *
   * This should be equal to juce parameter's getValue().
   */
  std::atomic<float> last_modulated_value_;

  /**
   * @brief Input CV port for modulation, if parameter is modulatable.
   *
   * Real-time modulation sources (sources that act on the base parameter
   * value) are expected to connect to this.
   */
  PortUuidReference modulation_input_uuid_;

  // Processing cache
  dsp::CVPort * modulation_input_{};

  /**
   * @brief Automation value provider.
   *
   * This will normally be an automation track.
   */
  std::optional<AutomationValueProvider> automation_value_provider_;

  /** Unique symbol. */
  std::optional<utils::Utf8String> symbol_;

  /** Description, if any, to be shown in tooltips. */
  std::optional<utils::Utf8String> description_;

  // Automatable (via automation provider or modulation)
  bool automatable_{ true };

  // Not on GUI
  bool hidden_{};

  BOOST_DESCRIBE_CLASS (
    ProcessorParameter,
    (utils::UuidIdentifiableObject<ProcessorParameter>),
    (),
    (),
    (unique_id_,
     range_,
     label_,
     base_value_,
     modulation_input_uuid_,
     symbol_,
     description_,
     automatable_,
     hidden_))
};

inline auto
format_as (const ProcessorParameter::UniqueId &id)
{
  return type_safe::get (id).view ();
}

using ProcessorParameterPtrVariant = std::variant<ProcessorParameter *>;
}

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::dsp::ProcessorParameter::Uuid)

namespace zrythm::dsp
{
/**
 * @brief Wrapper over a Uuid registry that provides (slow) lookup by unique ID.
 *
 * These helpers are mainly intended for use by plugins during project load so
 * we know which plugin parameter corresponds to which UUID (via UniqueId match).
 */
class ProcessorParameterRegistry
    : public utils::
        OwningObjectRegistry<ProcessorParameterPtrVariant, ProcessorParameter>
{
public:
  ProcessorParameterRegistry (
    dsp::PortRegistry &port_registry,
    QObject *          parent = nullptr)
      : utils::OwningObjectRegistry<
          ProcessorParameterPtrVariant,
          ProcessorParameter> (parent),
        port_registry_ (port_registry)
  {
  }

  ProcessorParameter *
  find_by_unique_id (const ProcessorParameter::UniqueId &id) const
  {
    const auto &map = get_hash_map ();
    for (const auto &kv : map)
      {
        if (std::get<ProcessorParameter *> (kv.second)->get_unique_id () == id)
          {
            return std::get<ProcessorParameter *> (kv.second);
          }
      }
    return nullptr;
  }

  ProcessorParameter *
  find_by_unique_id_or_throw (const ProcessorParameter::UniqueId &id) const
  {
    auto * val = find_by_unique_id (id);
    if (val == nullptr) [[unlikely]]
      {
        throw std::runtime_error (
          fmt::format ("Processor Parameter with unique id {} not found", id));
      }
    return val;
  }

private:
  struct ProcessorParameterRegistryBuilder
  {
    ProcessorParameterRegistryBuilder (dsp::PortRegistry &port_registry)
        : port_registry_ (port_registry)
    {
    }

    template <typename T> std::unique_ptr<T> build () const
    {
      return std::make_unique<T> (
        port_registry_, ProcessorParameter::UniqueId (u8""), ParameterRange{},
        u8"");
    }

    dsp::PortRegistry &port_registry_;
  };

  friend void
  from_json (const nlohmann::json &j, ProcessorParameterRegistry &reg)
  {
    from_json_with_builder (
      j, reg, ProcessorParameterRegistryBuilder{ reg.port_registry_ });
  }

private:
  dsp::PortRegistry &port_registry_;
};

using ProcessorParameterUuidReference = utils::UuidReference<
  utils::OwningObjectRegistry<ProcessorParameterPtrVariant, ProcessorParameter>>;

} // namespace zrythm::dsp

namespace std
{
template <> struct hash<zrythm::dsp::ProcessorParameter::UniqueId>
{
  size_t operator() (const zrythm::dsp::ProcessorParameter::UniqueId &id) const
  {
    return id.hash ();
  }
};
}
