// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/cv_port.h"
#include "dsp/graph_node.h"
#include "utils/format.h"
#include "utils/math.h"
#include "utils/serialization.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{

class ParameterRange
{
  Q_GADGET

public:
  enum class Type : std::uint_fast8_t
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

  /**
   * Unit to be displayed in the UI.
   */
  enum class Unit : std::uint_fast8_t
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

  static utils::Utf8String unit_to_string (Unit unit)
  {
    constexpr std::array<std::u8string_view, 8> port_unit_strings = {
      u8"none", u8"Hz", u8"MHz", u8"dB", u8"°", u8"s", u8"ms", u8"μs",
    };
    return port_unit_strings.at (ENUM_VALUE_TO_INT (unit));
  }

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

  float clamp_to_range (float val) const
  {
    return std::clamp (val, minf_, maxf_);
  }

  Q_INVOKABLE float convertFrom0To1 (float normalized_val) const
  {
    if (type_ == Type::Logarithmic)
      {
        const auto minf =
          utils::math::floats_equal (minf_, 0.f) ? 1e-20f : minf_;
        const auto maxf =
          utils::math::floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;
        normalized_val =
          utils::math::floats_equal (normalized_val, 0.f) ? 1e-20f : normalized_val;

        /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
        return minf * std::pow (maxf / minf, normalized_val);
      }
    if (type_ == Type::Toggle)
      {
        return normalized_val >= 0.001f ? 1.f : 0.f;
      }
    if (type_ == Type::GainAmplitude)
      {
        return utils::math::get_amp_val_from_fader (normalized_val);
      }

    return minf_ + (normalized_val * (maxf_ - minf_));
  }

  Q_INVOKABLE float convertTo0To1 (float real_val) const
  {

    if (type_ == Type::Logarithmic)
      {
        const auto minf =
          utils::math::floats_equal (minf_, 0.f) ? 1e-20f : minf_;
        const auto maxf =
          utils::math::floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;
        real_val = utils::math::floats_equal (real_val, 0.f) ? 1e-20f : real_val;

        /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
        return std::log (real_val / minf) / std::log (maxf / minf);
      }
    if (type_ == Type::Toggle)
      {
        return real_val;
      }
    if (type_ == Type::GainAmplitude)
      {
        return utils::math::get_fader_val_from_amp (real_val);
      }

    const auto sizef = maxf_ - minf_;
    return (sizef - (maxf_ - real_val)) / sizef;
  }

  bool is_toggled (float normalized_val) const
  {
    assert (type_ == ParameterRange::Type::Toggle);
    return utils::math::floats_equal (convertFrom0To1 (normalized_val), 1.f);
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (
    ParameterRange,
    type_,
    minf_,
    maxf_,
    zerof_,
    deff_,
    unit_)

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
  Q_PROPERTY (ParameterRange range READ range CONSTANT)
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
    std::function<std::optional<float> (unsigned_frame_t sample_position)>;

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString label () const { return label_.to_qstring (); }
  QString description () const { return description_->to_qstring (); }
  bool    automatable () const { return automatable_; }

  ParameterRange range () const { return range_; }

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
  void process_block (EngineProcessTimeInfo time_nfo) noexcept override;

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

  const auto &get_unique_id () const { return unique_id_; }

private:
  // Some of these don't need serialization because they'll be created
  // even when loading from json.
  // We just need to serialize IDs, the value, and possibly the label too for
  // debugging.
  static constexpr auto kUniqueIdKey = "uniqueId"sv;
  // static constexpr auto kTypeKey = "type"sv;
  // static constexpr auto kRangeKey = "range"sv;
  // static constexpr auto kUnitKey = "unit"sv;
  static constexpr auto kLabelKey = "label"sv;
  // static constexpr auto kSymbolKey = "symbol"sv;
  // static constexpr auto kDescriptionKey = "description"sv;
  // static constexpr auto kAutomatableKey = "automatable"sv;
  // static constexpr auto kHiddenKey = "hidden"sv;
  static constexpr auto kBaseValueKey = "baseValue"sv;
  static constexpr auto kModulationSourcePortIdKey = "modulationSourcePortId"sv;
  friend void           to_json (nlohmann::json &j, const ProcessorParameter &p)
  {
    j[kUniqueIdKey] = p.unique_id_;
    j[kLabelKey] = p.label_;
    j[kBaseValueKey] = p.base_value_.load ();
    j[kModulationSourcePortIdKey] = p.modulation_input_uuid_;
  }
  friend void from_json (const nlohmann::json &j, ProcessorParameter &p)
  {
    j.at (kUniqueIdKey).get_to (p.unique_id_);
    j.at (kLabelKey).get_to (p.label_);
    float base_val{};
    j.at (kBaseValueKey).get_to (base_val);
    p.base_value_.store (base_val);
    j.at (kModulationSourcePortIdKey).get_to (p.modulation_input_uuid_);
  }

private:
  /**
   * @brief Unique ID of this parameter.
   */
  UniqueId unique_id_;

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
    for (const auto &kv : map.asKeyValueRange ())
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

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::dsp::ProcessorParameter::Uuid)
