// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_SEND_H__
#define __AUDIO_CHANNEL_SEND_H__

#include "dsp/plugin_slot.h"
#include "gui/dsp/audio_port.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/track.h"
#include "utils/icloneable.h"

class StereoPorts;
class ChannelTrack;
class Port;
class PortConnectionsManager;
class PortConnection;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * The slot where post-fader sends begin (starting from 0).
 */
constexpr int CHANNEL_SEND_POST_FADER_START_SLOT = 6;

/**
 * Target type.
 */
enum class ChannelSendTargetType
{
  /** Remove send. */
  None,

  /** Send to track inputs. */
  Track,

  /** Send to plugin sidechain inputs. */
  PluginSidechain,
};

/**
 * Send target (used in list views).
 */
struct ChannelSendTarget
{
  ChannelSendTargetType type = {};

  int track_pos = 0;

  gui::old_dsp::plugins::Plugin::Uuid pl_id;

  std::string port_group;

  /**
   * Returns a string describing this target (track/plugin name/etc.).
   */
  std::string describe () const;

  std::string get_icon () const;

  static void free_func (void * target) { delete (ChannelSendTarget *) target; }
};

/**
 * Channel send.
 *
 * The actual connection is tracked separately by PortConnectionsManager.
 */
class ChannelSend final
    : public ICloneable<ChannelSend>,
      public dsp::IProcessable,
      public utils::serialization::ISerializable<ChannelSend>,
      public IPortOwner
{
public:
  using PortType = dsp::PortType;
  struct SlotTag
  {
    int value_{};
  };

public:
  ChannelSend (const DeserializationDependencyHolder &dh);

  /**
   * To be used when creating a new (identity) ChannelSend.
   */
  ChannelSend (
    ChannelTrack  &track,
    TrackRegistry &track_registry,
    PortRegistry  &port_registry,
    int            slot)
      : ChannelSend (track_registry, port_registry, track, slot, true)
  {
  }

  /**
   * @brief To be used when instantiating or cloning an existing identity.
   */
  ChannelSend (TrackRegistry &track_registry, PortRegistry &port_registry)
      : ChannelSend (track_registry, port_registry, std::nullopt, std::nullopt, false)
  {
  }

private:
  /**
   * @brief Internal implementation detail.
   */
  ChannelSend (
    TrackRegistry            &track_registry,
    PortRegistry             &port_registry,
    OptionalRef<ChannelTrack> track,
    std::optional<int>        slot,
    bool                      new_identity);

private:
  auto &get_port_registry () { return port_registry_; }
  auto &get_port_registry () const { return port_registry_; }

public:
  void init_loaded (ChannelTrack * track);

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  std::string
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  bool is_prefader () const
  {
    return slot_ < CHANNEL_SEND_POST_FADER_START_SLOT;
  }

  /**
   * Gets the owner track.
   */
  ChannelTrack * get_track () const;

  std::string get_node_name () const override;

  bool is_enabled () const;

  bool is_empty () const { return !is_enabled (); }

  /**
   * Returns whether the channel send target is a
   * sidechain port (rather than a target track).
   */
  bool is_target_sidechain ();

  /**
   * Gets the target track.
   */
  Track * get_target_track ();

  /**
   * Gets the amount to be used in widgets (0.0-1.0).
   */
  float get_amount_for_widgets () const;

  /**
   * Sets the amount from a widget amount (0.0-1.0).
   */
  void set_amount_from_widget (float val);

  float get_amount_value () const { return get_amount_port ().control_; }

  /**
   * Connects a send to stereo ports.
   *
   * @throw ZrythmException if the connection fails.
   */
  bool connect_stereo (
    AudioPort &l,
    AudioPort &r,
    bool       sidechain,
    bool       recalc_graph,
    bool       validate);

  /**
   * Connects a send to a midi port.
   *
   * @throw ZrythmException if the connection fails.
   */
  bool connect_midi (MidiPort &port, bool recalc_graph, bool validate);

  /**
   * Removes the connection at the given send.
   */
  void disconnect (bool recalc_graph);

  void set_amount (float amount);

  std::pair<AudioPort &, AudioPort &> get_stereo_in_ports () const
  {
    if (!stereo_in_left_id_.has_value ())
      {
        throw ZrythmException ("stereo_in_left_id_ not set");
      }
    auto * l = std::get<AudioPort *> (stereo_in_left_id_->get_object ());
    auto * r = std::get<AudioPort *> (stereo_in_right_id_->get_object ());
    return { *l, *r };
  }
  MidiPort &get_midi_in_port () const
  {
    return *std::get<MidiPort *> (midi_in_id_->get_object ());
  }
  std::pair<AudioPort &, AudioPort &> get_stereo_out_ports () const
  {
    if (!stereo_out_left_id_.has_value ())
      {
        throw ZrythmException ("stereo_out_left_id_ not set");
      }
    auto * l = std::get<AudioPort *> (stereo_out_left_id_->get_object ());
    auto * r = std::get<AudioPort *> (stereo_out_right_id_->get_object ());
    return { *l, *r };
  }
  MidiPort &get_midi_out_port () const
  {
    return *std::get<MidiPort *> (midi_out_id_->get_object ());
  }
  ControlPort &get_amount_port () const
  {
    return *std::get<ControlPort *> (amount_id_->get_object ());
  }
  ControlPort &get_enabled_port () const
  {
    return *std::get<ControlPort *> (enabled_id_->get_object ());
  }

  /**
   * Get the name of the destination, or a placeholder
   * text if empty.
   */
  std::string get_dest_name () const;

  void copy_values_from (const ChannelSend &other);

  void init_after_cloning (const ChannelSend &other, ObjectCloneType clone_type)
    override;

  /**
   * Connects the ports to the owner track if not connected.
   *
   * Only to be called on project sends.
   */
  void connect_to_owner ();

  void append_ports (std::vector<Port *> &ports);

  /**
   * Appends the connection(s), if non-empty, to the given array (if not
   * nullptr) and returns the number of connections added.
   */
  int append_connection (
    const PortConnectionsManager * mgr,
    std::vector<PortConnection *> &arr) const;

  void prepare_process ();

  void process_block (EngineProcessTimeInfo time_nfo) override;

  bool is_connected_to (std::pair<PortUuid, PortUuid> stereo) const
  {
    return is_connected_to (stereo, std::nullopt);
  }
  bool is_connected_to (const PortUuid &midi) const
  {
    return is_connected_to (std::nullopt, midi);
  }

  /**
   * Finds the project send from a given send instance.
   */
  ChannelSend * find_in_project () const;

  bool is_audio () const { return get_signal_type () == PortType::Audio; }
  bool is_midi () const { return get_signal_type () == PortType::Event; }

  bool validate ();

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  PortType get_signal_type () const;

  void disconnect_midi ();
  void disconnect_audio ();

  void construct_for_slot (ChannelTrack &track, int slot);

  PortConnectionsManager * get_port_connections_manager () const;

  /**
   * Returns whether the send is connected to the given ports.
   */
  bool is_connected_to (
    std::optional<std::pair<PortUuid, PortUuid>> stereo,
    std::optional<PortUuid>                      midi) const;

public:
  PortRegistry  &port_registry_;
  TrackRegistry &track_registry_;

  /** Slot index in the channel sends. */
  int slot_ = 0;

  /**
   * Stereo input if audio send.
   *
   * Prefader or fader stereo out should connect here.
   */
  std::optional<PortUuidReference> stereo_in_left_id_;
  std::optional<PortUuidReference> stereo_in_right_id_;

  /**
   * MIDI input if MIDI send.
   *
   * Prefader or fader MIDI out should connect here.
   */
  std::optional<PortUuidReference> midi_in_id_;

  /**
   * Stereo output if audio send.
   *
   * This should connect to the send destination, if any.
   */
  std::optional<PortUuidReference> stereo_out_left_id_;
  std::optional<PortUuidReference> stereo_out_right_id_;

  /**
   * MIDI output if MIDI send.
   *
   * This should connect to the send destination, if any.
   */
  std::optional<PortUuidReference> midi_out_id_;

  /** Send amount (amplitude), 0 to 2 for audio, velocity multiplier for
   * MIDI. */
  std::optional<PortUuidReference> amount_id_;

  /**
   * Whether the send is currently enabled.
   *
   * If enabled, corresponding connection(s) will exist in
   * PortConnectionsManager.
   */
  std::optional<PortUuidReference> enabled_id_;

  /** If the send is a sidechain. */
  bool is_sidechain_ = false;

  /** Pointer back to owner track. */
  // ChannelTrack * track_ = nullptr;

  /** Owner track ID. */
  TrackUuid track_id_;

  /**
   * @brief Use this if set (via the new identity constructo).
   */
  OptionalRef<ChannelTrack> track_;
};

/**
 * @}
 */

#endif
