// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_SEND_H__
#define __AUDIO_CHANNEL_SEND_H__

#include "common/dsp/audio_port.h"
#include "common/dsp/control_port.h"
#include "common/dsp/midi_port.h"
#include "common/utils/icloneable.h"

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

  zrythm::plugins::PluginIdentifier pl_id{};

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
      public ISerializable<ChannelSend>
{
public:
public:
  ChannelSend () = default;
  /**
   * Creates a channel send instance.
   */
  ChannelSend (unsigned int track_name_hash, int slot, ChannelTrack * track);
  void init_loaded (ChannelTrack * track);

  bool is_in_active_project () const;

  bool is_prefader () const
  {
    return slot_ < CHANNEL_SEND_POST_FADER_START_SLOT;
  }

  /**
   * Gets the owner track.
   */
  ChannelTrack * get_track () const;

  bool is_enabled () const;

  inline bool is_empty () const { return !is_enabled (); }

  /**
   * Returns whether the channel send target is a
   * sidechain port (rather than a target track).
   */
  bool is_target_sidechain ();

  /**
   * Gets the target track.
   *
   * @param owner The owner track of the send (optional).
   */
  Track * get_target_track (const ChannelTrack * owner);

  /**
   * Gets the target sidechain port as a newly allocated instance of
   * StereoPorts.
   */
  std::unique_ptr<StereoPorts> get_target_sidechain ();

  /**
   * Gets the amount to be used in widgets (0.0-1.0).
   */
  float get_amount_for_widgets () const;

  /**
   * Sets the amount from a widget amount (0.0-1.0).
   */
  void set_amount_from_widget (float val);

  /**
   * Connects a send to stereo ports.
   *
   * This function takes either @ref stereo or both @ref l and @ref r.
   *
   * @throw ZrythmException if the connection fails.
   */
  bool connect_stereo (
    StereoPorts * stereo,
    AudioPort *   l,
    AudioPort *   r,
    bool          sidechain,
    bool          recalc_graph,
    bool          validate);

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

  /**
   * Get the name of the destination, or a placeholder
   * text if empty.
   */
  std::string get_dest_name () const;

  void copy_values_from (const ChannelSend &other);

  void init_after_cloning (const ChannelSend &other) override;

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

  void process (nframes_t local_offset, nframes_t nframes);

  /**
   * Returns whether the send is connected to the given ports.
   */
  bool is_connected_to (const StereoPorts * stereo, const Port * midi) const;

  /**
   * Finds the project send from a given send instance.
   */
  ChannelSend * find_in_project () const;

  bool validate ();

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  PortType get_signal_type () const;

  void disconnect_midi ();
  void disconnect_audio ();

  void construct_for_slot (int slot);

public:
  /** Slot index in the channel sends. */
  int slot_ = 0;

  /**
   * Stereo input if audio send.
   *
   * Prefader or fader stereo out should connect here.
   */
  std::unique_ptr<StereoPorts> stereo_in_;

  /**
   * MIDI input if MIDI send.
   *
   * Prefader or fader MIDI out should connect here.
   */
  std::unique_ptr<MidiPort> midi_in_;

  /**
   * Stereo output if audio send.
   *
   * This should connect to the send destination, if any.
   */
  std::unique_ptr<StereoPorts> stereo_out_;

  /**
   * MIDI output if MIDI send.
   *
   * This should connect to the send destination, if any.
   */
  std::unique_ptr<MidiPort> midi_out_;

  /** Send amount (amplitude), 0 to 2 for audio, velocity multiplier for
   * MIDI. */
  std::unique_ptr<ControlPort> amount_;

  /**
   * Whether the send is currently enabled.
   *
   * If enabled, corresponding connection(s) will exist in
   * PortConnectionsManager.
   */
  std::unique_ptr<ControlPort> enabled_;

  /** If the send is a sidechain. */
  bool is_sidechain_ = false;

  /** Pointer back to owner track. */
  ChannelTrack * track_ = nullptr;

  /** Track name hash (used in actions). */
  unsigned int track_name_hash_ = 0;
};

/**
 * @}
 */

#endif
