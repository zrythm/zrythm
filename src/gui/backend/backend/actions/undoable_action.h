// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/plugin.h"
#include "structure/tracks/port_connections_manager.h"
#include "utils/types.h"

class AudioClip;

namespace zrythm::gui::actions
{

#define DEFINE_UNDOABLE_ACTION_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* helpers */ \
  /* ================================================================ */ \
  Q_INVOKABLE QString getDescription () const \
  { \
    return to_string (); \
  }

/**
 * Base class to be inherited by implementing undoable actions.
 */
class UndoableAction
{
public:
  using PortConnectionsManager = structure::tracks::PortConnectionsManager;

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
  UndoableAction (
    Type               type,
    dsp::FramesPerTick frames_per_tick,
    sample_rate_t      sample_rate);
  virtual ~UndoableAction () = default;

  /**
   * @brief Non virtual function following the NVI pattern.
   *
   * @see init_loaded_impl().
   */
  void init_loaded (sample_rate_t engine_sample_rate);

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
   *
   * @deprecated Will be removed in the future in favor of UuidReference's to
   * AudioClip's.
   */
  virtual bool can_contain_clip () const { return false; };

  /**
   * Checks whether the action actually contains or refers to the given audio
   * clip.
   *
   * @deprecated Will be removed in the future in favor of UuidReference's to
   * AudioClip's.
   */
  virtual bool contains_clip (const AudioClip &clip) const { return false; };

  /**
   * @brief Get the plugins referenced in this action.
   *
   * @param plugins
   */
  virtual void
  get_plugins (std::vector<gui::old_dsp::plugins::Plugin *> &plugins) { };

  auto get_frames_per_tick () const { return frames_per_tick_; }
  auto get_ticks_per_frame () const
  {
    return to_ticks_per_frame (get_frames_per_tick ());
  }

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
  virtual QString to_string () const = 0;

protected:
  friend void init_from (
    UndoableAction        &obj,
    const UndoableAction  &other,
    utils::ObjectCloneType clone_type);

private:
  /** NVI pattern. */
  virtual void init_loaded_impl () = 0;
  virtual void perform_impl () = 0;
  virtual void undo_impl () = 0;

  /** Common handler for both perform and undo. */
  void do_or_undo (bool perform);

public:
  /** Undoable action type. */
  Type undoable_action_type_{};

  /** A snapshot of AudioEngine.frames_per_tick when the action is executed. */
  dsp::FramesPerTick frames_per_tick_;

  /**
   * Sample rate of this action.
   *
   * Used to recalculate UndoableAction.frames_per_tick when the project is
   * loaded under a new samplerate.
   */
  sample_rate_t sample_rate_ = 0;

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
  std::unique_ptr<structure::tracks::PortConnectionsManager>
    port_connections_before_;

  /** @see port_connections_before_. */
  std::unique_ptr<structure::tracks::PortConnectionsManager>
    port_connections_after_;
};

class ArrangerSelectionsAction;
class ChannelSendAction;
class ChordAction;
class MidiMappingAction;
class MixerSelectionsAction;
class PortAction;
class PortConnectionAction;
class RangeAction;
class TracklistSelectionsAction;
class TransportAction;
using UndoableActionVariant = std::variant<
  ArrangerSelectionsAction,
  ChannelSendAction,
  ChordAction,
  MidiMappingAction,
  MixerSelectionsAction,
  PortAction,
  PortConnectionAction,
  RangeAction,
  TracklistSelectionsAction,
  TransportAction>;
using UndoableActionPtrVariant = to_pointer_variant<UndoableActionVariant>;

template <typename T>
concept UndoableActionSubclass =
  std::derived_from<T, UndoableAction> && !std::is_same_v<T, UndoableAction>;

}; // namespace zrythm::gui::actions
