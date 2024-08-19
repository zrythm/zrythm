// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Undoable actions.
 */

#ifndef __UNDO_UNDOABLE_ACTION_H__
#define __UNDO_UNDOABLE_ACTION_H__

#include "dsp/port_connections_manager.h"
#include "utils/types.h"

class AudioClip;
class Plugin;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Base class to be inherited by implementing undoable actions.
 */
class UndoableAction : public ISerializable<UndoableAction>
{
public:
  /**
   * Type of UndoableAction.
   */
  enum class Type
  {
    /* ---- Track/Channel ---- */
    TracklistSelections,

    ChannelSend,

    /* ---- end ---- */

    MixerSelections,
    ArrangerSelections,

    /* ---- connections ---- */

    MidiMapping,
    PortConnection,
    Port,

    /* ---- end ---- */

    /* ---- range ---- */

    Range,

    /* ---- end ---- */

    Transport,

    Chord,
  };

  UndoableAction () = default;
  UndoableAction (Type type);
  UndoableAction (Type type, double frames_per_tick, sample_rate_t sample_rate);
  virtual ~UndoableAction () = default;

  /**
   * @brief Create a unique from id object
   *
   * @param type
   * @return std::unique_ptr<UndoableAction>
   *
   * @throw ZrythmException on error.
   */
  static std::unique_ptr<UndoableAction> create_unique_from_type (Type type);

  /**
   * @brief Non virtual function following the NVI pattern.
   *
   * @see init_loaded_impl().
   */
  void init_loaded ();

  /**
   * Returns whether the action requires pausing the engine.
   */
  virtual bool needs_pause () const { return true; };

  /**
   * Returns whether the total transport bars need to be recalculated.
   *
   * @note Some actions already handle this logic so return false here to avoid
   * unnecessary calculations.
   */
  virtual bool needs_transport_total_bar_update (bool perform) const
  {
    return true;
  };

  /**
   * Whether audio region loop/fade/etc. positions are affected by this undoable
   * action.
   *
   * Used to correct off-by-one errors when changing BPM or resampling or
   * something that causes position conversions.
   */
  virtual bool affects_audio_region_internal_positions () const { return true; }

  /**
   * Checks whether the action can contain an audio clip.
   *
   * No attempt is made to remove unused files from the pool for actions that
   * can't contain audio clips.
   */
  virtual bool can_contain_clip () const { return false; };

  /**
   * Checks whether the action actually contains or refers to the given audio
   * clip.
   */
  virtual bool contains_clip (const AudioClip &clip) const { return false; };

  /**
   * @brief Get the plugins referenced in this action.
   *
   * @param plugins
   */
  virtual void get_plugins (std::vector<Plugin *> &plugins) {};

  /**
   * Sets the number of actions for this action.
   *
   * This should be set on the last action to be performed.
   */
  void set_num_actions (int num_actions);

  /**
   * To be used by actions that save/load port connections.
   *
   * @param performing True if doing/performing, false if undoing.
   */
  void save_or_load_port_connections (bool performing);

  /**
   * Performs the action.
   *
   * @note Only to be called by undo manager.
   *
   * @throw ZrythmException on error.
   */
  void perform ();

  /**
   * Undoes the action.
   *
   * @throw ZrythmException on error.
   */
  void undo ();

  /**
   * Stringizes the action to be used in Undo/Redo buttons.
   */
  virtual std::string to_string () const = 0;

protected:
  void copy_members_from (const UndoableAction &other);

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  /** NVI pattern. */
  virtual void init_loaded_impl () = 0;
  virtual void perform_impl () = 0;
  virtual void undo_impl () = 0;

  /** Common handler for both perform and undo. */
  void do_or_undo (bool perform);

public:
  /** Undoable action type. */
  Type undoable_action_type_ = (Type) 0;

  /** A snapshot of AudioEngine.frames_per_tick when the action is executed. */
  double frames_per_tick_ = 0.0;

  /**
   * Sample rate of this action.
   *
   * Used to recalculate UndoableAction.frames_per_tick when the project is
   * loaded under a new samplerate.
   */
  sample_rate_t sample_rate_ = 0;

  /**
   * Index in the stack.
   *
   * Used during deserialization.
   */
  // int stack_idx_ = 0;

  /**
   * Number of actions to perform.
   *
   * This is used to group multiple actions into one logical action (eg,
   * create a group track and route multiple tracks to it).
   *
   * To be set on the last action being performed.
   */
  int num_actions_ = 1;

  /**
   * @brief An (optional) clone of the port connections at the start of the
   * action, used for reverting port connections when undoing.
   */
  std::unique_ptr<PortConnectionsManager> port_connections_before_;

  /** @see port_connections_before_. */
  std::unique_ptr<PortConnectionsManager> port_connections_after_;
};

/**
 * @}
 */

#endif
