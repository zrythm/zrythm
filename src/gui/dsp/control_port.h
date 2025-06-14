// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include <utility>

#include "gui/dsp/port.h"
#include "utils/icloneable.h"
#include "utils/math.h"

/**
 * @brief Control port specifics.
 */
class ControlPort final
    : public QObject,
      public Port,
      public PortConnectionsCacheMixin<CVPort>
{
  Q_OBJECT
  QML_ELEMENT

public:
  /**
   * @brief Gets the current automation value if available (eg, from an
   * automation track).
   *
   * If available, the ControlPort will be overridden to this value during
   * processing.
   *
   * @param global_frame Frame in global coordinates (timeline).
   * @return Optional normalized automation value (0.0-1.0)
   */
  using AutomationValueGetter =
    std::function<std::optional<float> (signed_frame_t global_frame)>;

  /**
   * Used for queueing changes to be applied during processing.
   *
   * Used only for non-plugin ports such as BPM and time signature.
   */
  struct ChangeEvent
  {
    /**
     * Flag to identify the port the change is for.
     *
     * @see BPM.
     */
    PortIdentifier::Flags flag1{};

    /**
     * Flag to identify the port the change is for.
     *
     * @see BEATS_PER_BAR and BEAT_UNIT.
     */
    PortIdentifier::Flags2 flag2{};

    /** Real (not normalized) value to set. */
    float real_val = 0.0f;

    /** Integer val. */
    int ival = 0;

    BeatUnit beat_unit{};
  };

  /**
   * Scale point.
   */
  struct ScalePoint
  {
    float             val_;
    utils::Utf8String label_;

    ScalePoint (float val, utils::Utf8String label)
        : val_ (val), label_ (std::move (label))
    {
    }

    friend auto operator<=> (const ScalePoint &lhs, const ScalePoint &rhs)
    {
      return lhs.val_ <=> rhs.val_;
    }

    friend bool operator== (const ScalePoint &lhs, const ScalePoint &rhs)
    {
      return utils::math::floats_equal (lhs.val_, rhs.val_);
    }
  };

public:
  ControlPort (utils::Utf8String label = {});

  /**
   * Converts normalized value (0.0 to 1.0) to
   * real value (eg. -10.0 to 100.0).
   *
   * @note This behaves differently from
   *   @ref port_set_control_value() and
   *   @ref port_get_control_value() and should be
   *   used in widgets.
   */
  float normalized_val_to_real (float normalized_val) const;

  /**
   * Converts real value (eg. -10.0 to 100.0) to
   * normalized value (0.0 to 1.0).
   *
   * @note This behaves differently from
   *   @ref port_set_control_value() and
   *   @ref port_get_control_value() and should be
   *   used in widgets.
   */
  float real_val_to_normalized (float real_val) const;

  /**
   * Checks if the given value is toggled.
   */
  static bool is_val_toggled (float val) { return val > 0.001f; }

  /**
   * Returns if the control port is toggled.
   */
  bool is_toggled () const { return is_val_toggled (control_); }

  /**
   * Gets the control value for an integer port.
   */
  int get_int () const { return get_int_from_val (control_); }

  /**
   * @brief Set the identifier's port unit from the given string.
   */
  void set_unit_from_str (const utils::Utf8String &str);

  /**
   * Gets the control value for an integer port.
   */
  static int get_int_from_val (float val)
  {
    const auto s32 = utils::math::round_to_signed_32 (val);
    assert (
      s32 >= std::numeric_limits<int>::min ()
      && s32 <= std::numeric_limits<int>::max ());
    return static_cast<int> (s32);
  }

  /**
   * Returns the snapped value (eg, if toggle, returns 0.f or 1.f).
   */
  float get_snapped_val () const
  {
    return get_snapped_val_from_val (get_val ());
  }

  /**
   * Returns the snapped value (eg, if toggle, returns 0.f or 1.f).
   */
  float get_snapped_val_from_val (float val) const;

  /**
   * Get the current real value of the control.
   *
   * TODO "normalize" parameter.
   */
  float get_val () const { return control_; }

  /**
   * Get the current normalized value of the control.
   */
  float get_normalized_val () const
  {
    return real_val_to_normalized (control_);
  }

  /**
   * Get the current real unsnapped value of the
   * control.
   */
  float get_unsnapped_val () const { return unsnapped_control_; }

  /**
   * Get the default real value of the control.
   */
  float get_default_val () const { return deff_; }

  /**
   * Get the default real value of the control.
   */
  void set_real_val (float val) { set_control_value (val, false, false); }

  /**
   * Set the default real value of the control and sends UI events.
   */
  void set_real_val_w_events (float val)
  {
    set_control_value (val, false, true);
  }

  /**
   * Wrapper over port_set_control_value() for toggles.
   */
  void set_toggled (bool toggled, bool forward_events)
  {
    set_control_value (toggled ? 1.f : 0.f, false, forward_events);
  }

  /**
   * Updates the actual value.
   *
   * The given value is always a normalized 0.0-1.0 value and must be translated
   * to the actual value before setting it.
   *
   * @param automating Whether this is from an automation event. This will set
   * Lv2Port's automating field to true, which will cause the plugin to receive
   * a UI event for this change.
   */
  [[gnu::hot]] void set_val_from_normalized (float val, bool automating);

  /**
   * Sets the given control value to the corresponding underlying structure in
   the Port.
   *
   * Note: this is only for setting the base values (eg when automating via an
   automation lane). For CV automations this should not be used.
   *
   * @param is_normalized Whether the given value is normalized between 0 and 1.
   * @param forward_event_to_plugin Whether to forward a port control change
   event to
   * the plugin UI. Only applicable for plugin control ports. If the control is
   * being changed manually or from within Zrythm, this should be true to notify
   * the plugin of the change.
   */
  void
  set_control_value (float val, bool is_normalized, bool forward_event_to_plugin);

  /**
   * @brief Sets the automation value reader.
   *
   * This should be called any time before processing (the automation value
   * reaer is only used during process_block()).
   */
  void set_automation_value_reader (AutomationValueGetter reader)
  {
    automation_reader_ = reader;
  }

  RtTimePoint ms_since_last_change () const
  {
    return time_provider_->get_monotonic_time_usecs () - last_change_time_;
  }

  /**
   * Gets the given control value from the corresponding underlying structure in
   * the Port.
   *
   * @param normalize Whether to get the value normalized or not.
   */
  [[gnu::hot]] float get_control_value (bool normalize) const;

  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  void clear_buffer (std::size_t block_length) override { }

  friend void init_from (
    ControlPort           &obj,
    const ControlPort     &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kControlKey = "control";
  static constexpr std::string_view kRangeKey = "range";
  static constexpr std::string_view kDefaultValueKey = "defaultValue";
  static constexpr std::string_view kCarlaParameterIdKey = "carlaParameterId";
  static constexpr std::string_view kBaseValueKey = "baseValue";
  friend void to_json (nlohmann::json &j, const ControlPort &p)
  {
    to_json (j, static_cast<const Port &> (p));
    j[kControlKey] = p.control_;
    j[kRangeKey] = p.range_;
    j[kDefaultValueKey] = p.deff_;
    j[kCarlaParameterIdKey] = p.carla_param_id_;
    j[kBaseValueKey] = p.base_value_;
  }
  friend void from_json (const nlohmann::json &j, ControlPort &p)
  {
    from_json (j, static_cast<Port &> (p));
    j.at (kControlKey).get_to (p.control_);
    j.at (kRangeKey).get_to (p.range_);
    j.at (kDefaultValueKey).get_to (p.deff_);
    j.at (kCarlaParameterIdKey).get_to (p.carla_param_id_);
    j.at (kBaseValueKey).get_to (p.base_value_);
  }

  /**
   * To be called when a control's value changes so that a message can be sent
   * to the plugin UI.
   */
  // void forward_control_change_event ();

public:
  /**
   * @brief The control value.
   *
   * FIXME for fader, this should be the fader_val (0.0 to 1.0) and not the
   * amplitude.
   *
   * This value will be snapped (eg, if integer or toggle).
   */
  float control_ = 0.f;

  /**
   * For control ports, when a modulator is attached to the port the previous
   * value will be saved here.
   *
   * Automation in AutomationTrack's will overwrite this value.
   */
  float base_value_ = 0.f;

  /** Scale points. */
  std::vector<ScalePoint> scale_points_;

  /* --- MIDI CC info --- */

  /*
   * Next 2 objects are MIDI CC info, if MIDI CC in track processor.
   *
   * Used as a cache.
   */

  /** MIDI channel, starting from 1. */
  midi_byte_t midi_channel_ = 0;

  /** MIDI CC number, if not pitchbend/poly key/channel pressure. */
  midi_byte_t midi_cc_no_ = 0;

  /* --- end MIDI CC info --- */

  /** Default value. */
  float deff_ = 0.f;

  /** Index of the control parameter (for Carla plugin ports). */
  int carla_param_id_ = 0;

  /** Whether this value was set via automation. */
  bool automating_ = false;

  /** Unsnapped value, used by widgets. */
  float unsnapped_control_ = 0.f;

  /** Flag that the value of the port changed from reading automation. */
  bool value_changed_from_reading_{};

  /**
   * Whether the port received a UI event from the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event to the plugin.
   */
  bool received_ui_event_{};

private:
  /**
   * Last timestamp the control changed.
   *
   * This is used when recording automation in "touch" mode.
   */
  RtTimePoint last_change_time_{};

  std::optional<AutomationValueGetter> automation_reader_;

  std::unique_ptr<utils::QElapsedTimeProvider> time_provider_;
};
