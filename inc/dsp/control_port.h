// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CONTROL_PORT_H__
#define __AUDIO_CONTROL_PORT_H__

#include "dsp/port.h"
#include "utils/icloneable.h"
#include "utils/math.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief Control port specifics.
 */
class ControlPort final
    : public Port,
      public ICloneable<ControlPort>,
      public ISerializable<ControlPort>
{
public:
  /**
   * Used for queueing changes to be applied during
   * processing.
   *
   * Used only for non-plugin ports such as BPM and
   * time signature.
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
     * @see BEATS_PER_BAR and
     *   BEAT_UNIT.
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
    float       val_;
    std::string label_;

    ScalePoint (float val, std::string label) : val_ (val), label_ (label) { }
    ~ScalePoint () = default;

    bool operator< (const ScalePoint &other) { return val_ - other.val_ > 0.f; }
  };

public:
  ControlPort () = default;
  ControlPort (
    std::string               label,
    PortIdentifier::OwnerType owner_type = (PortIdentifier::OwnerType) 0,
    void *                    owner = nullptr)
      : Port (label, PortType::Control, PortFlow::Input, 0.f, 1.f, 0.f, owner_type, owner)
  {
  }

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
  void set_unit_from_str (const std::string &str);

  /**
   * Gets the control value for an integer port.
   */
  static int get_int_from_val (float val)
  {
    return math_round_float_to_signed_32 (val);
  }

  /**
   * Returns the snapped value (eg, if toggle, returns 0.f or 1.f).
   */
  inline float get_snapped_val () const
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
  inline float get_val () const { return control_; }
  static float val_getter (void * data)
  {
    return ((ControlPort *) data)->get_val ();
  }

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
  inline float get_default_val () const { return deff_; }
  static float default_val_getter (void * data)
  {
    return ((ControlPort *) data)->get_default_val ();
  }

  /**
   * Get the default real value of the control.
   */
  inline void set_real_val (float val)
  {
    set_control_value (val, false, false);
  }
  static void real_val_setter (void * data, float val)
  {
    ((ControlPort *) data)->set_real_val (val);
  }

  /**
   * Get the default real value of the control and sends UI events.
   */
  void set_real_val_w_events (float val)
  {
    set_control_value (val, false, true);
  }
  static void real_val_setter_w_events (void * data, float val)
  {
    ((ControlPort *) data)->set_real_val_w_events (val);
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
  HOT void set_val_from_normalized (float val, bool automating);

  /**
   * Function to get a port's value from its string symbol.
   *
   * Used when saving the LV2 state. This function MUST set size and type
   * appropriately.
   */
  static const void * get_value_from_symbol (
    const char * port_sym,
    void *       user_data,
    uint32_t *   size,
    uint32_t *   type);

  /**
   * Sets the given control value to the corresponding underlying structure in
   the Port.
   *
   * Note: this is only for setting the base values (eg when automating via an
   automation lane). For CV automations this should not be used.
   *
   * @param is_normalized Whether the given value is normalized between 0 and 1.
   * @param forward_event Whether to forward a port   control change event to
   the plugin UI. Only
   *   applicable for plugin control ports. If the control is being changed
   manually or from within Zrythm, this should be true to notify the plugin of
   the change.
   */
  void set_control_value (
    const float val,
    const bool  is_normalized,
    const bool  forward_event);

  /**
   * Gets the given control value from the corresponding underlying structure in
   * the Port.
   *
   * @param normalize Whether to get the value normalized or not.
   */
  HOT float get_control_value (const bool normalize) const;

  void allocate_bufs () override { }

  void
  process (const EngineProcessTimeInfo time_nfo, const bool noroll) override;

  void clear_buffer (AudioEngine &engine) override { }

  void copy_metadata_from_project (const Port &project_port) override
  {
    control_ = static_cast<const ControlPort &> (project_port).control_;
  }

  void restore_from_non_project (const Port &non_project) override
  {
    copy_metadata_from_project (non_project);
  }

  void init_after_cloning (const ControlPort &other) override
  {
    Port::copy_members_from (other);
    control_ = other.control_;
    base_value_ = other.base_value_;
    deff_ = other.deff_;
    carla_param_id_ = other.carla_param_id_;
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * To be called when a control's value changes so that a message can be sent
   * to the UI.
   */
  void forward_control_change_event ();

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

  /**
   * Last timestamp the control changed.
   *
   * This is used when recording automation in "touch" mode.
   */
  RtTimePoint last_change_time_;

  /** Default value. */
  float deff_ = 0.f;

  /** Index of the control parameter (for Carla plugin ports). */
  int carla_param_id_ = 0;

  /** Whether this value was set via automation. */
  bool automating_ = false;

  /** Unsnapped value, used by widgets. */
  float unsnapped_control_ = 0.f;

  /** Flag that the value of the port changed from reading automation. */
  bool value_changed_from_reading_ = false;

  /**
   * @brief Automation track this port is attached to.
   *
   * To be set at runtime only (not serialized).
   */
  AutomationTrack * at_ = nullptr;

  /**
   * Whether the port received a UI event from the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event to the plugin.
   */
  bool received_ui_event_ = 0;

  /**
   * Control widget, if applicable.
   *
   * Only used for generic UIs.
   */
  PluginGtkController * widget_ = 0;
};

/**
 * @}
 */

#endif
