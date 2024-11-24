// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_MIXER_SELECTIONS_ACTION_H__
#define __UNDO_MIXER_SELECTIONS_ACTION_H__

#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/mixer_selections.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port_connections_manager.h"
#include "gui/dsp/track.h"

namespace zrythm::gui::actions
{

/**
 * Action for manipulating plugins (plugin slot selections in the mixer).
 *
 * @see MixerSelections
 */
class MixerSelectionsAction
    : public QObject,
      public UndoableAction,
      public ICloneable<MixerSelectionsAction>,
      public zrythm::utils::serialization::ISerializable<MixerSelectionsAction>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (MixerSelectionsAction)

public:
  enum class Type
  {
    /** Duplicate from existing plugins. */
    Copy,
    /** Create new from clipboard. */
    Paste,
    /** Create new from PluginSetting. */
    Create,
    Delete,
    Move,
    ChangeStatus,
    ChangeLoadBehavior,
  };

  MixerSelectionsAction (QObject * parent = nullptr);

  /**
   * Create a new action.
   *
   * @param ms The mixer selections before the action is performed.
   * @param slot_type Target slot type.
   * @param to_track_name_hash Target track name hash, or 0 for new channel.
   * @param to_slot Target slot.
   * @param setting The plugin setting, if creating plugins.
   * @param num_plugins The number of plugins to create, if creating plugins.
   */
  MixerSelectionsAction (
    const FullMixerSelections *                ms,
    const PortConnectionsManager *             connections_mgr,
    Type                                       type,
    zrythm::gui::dsp::plugins::PluginSlotType  slot_type,
    unsigned int                               to_track_name_hash,
    int                                        to_slot,
    const PluginSetting *                      setting,
    int                                        num_plugins,
    int                                        new_val,
    zrythm::gui::dsp::plugins::CarlaBridgeMode new_bridge_mode,
    QObject *                                  parent = nullptr);

  QString to_string () const override;

  void
  get_plugins (std::vector<zrythm::gui::dsp::plugins::Plugin *> &plugins) override
  {
    if (ms_before_)
      ms_before_->get_plugins (plugins);
    if (deleted_ms_)
      deleted_ms_->get_plugins (plugins);
  }

  void init_after_cloning (const MixerSelectionsAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override;
  void perform_impl () override;
  void undo_impl () override;

  /**
   * Clone automation tracks.
   *
   * @param deleted Use deleted_ats.
   * @param start_slot Slot in @p ms to start processing.
   */
  void clone_ats (const FullMixerSelections &ms, bool deleted, int start_slot);

  void copy_automation_from_track1_to_track2 (
    const AutomatableTrack                   &from_track,
    AutomatableTrack                         &to_track,
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    int                                       from_slot,
    int                                       to_slot);

  void copy_at_regions (AutomationTrack &dest, const AutomationTrack &src);

  /**
   * Reverts automation events from before deletion.
   *
   * @param deleted Whether to use deleted_ats.
   */
  void revert_automation (
    AutomatableTrack    &track,
    FullMixerSelections &ms,
    int                  slot,
    bool                 deleted);

  /**
   * Save an existing plugin about to be replaced into @p tmp_ms.
   */
  void save_existing_plugin (
    FullMixerSelections *                     tmp_ms,
    Track *                                   from_tr,
    zrythm::gui::dsp::plugins::PluginSlotType from_slot_type,
    int                                       from_slot,
    Track *                                   to_tr,
    zrythm::gui::dsp::plugins::PluginSlotType to_slot_type,
    int                                       to_slot);

  void revert_deleted_plugin (Track &to_tr, int to_slot);

  void do_or_undo_create_or_delete (bool do_it, bool create);
  void do_or_undo_change_status (bool do_it);
  void do_or_undo_change_load_behavior (bool do_it);
  void do_or_undo_move_or_copy (bool do_it, bool copy);
  void do_or_undo (bool do_it);

public:
  Type mixer_selections_action_type_ = Type ();

  /** Type of starting slot to move plugins to. */
  zrythm::gui::dsp::plugins::PluginSlotType slot_type_ =
    zrythm::gui::dsp::plugins::PluginSlotType ();

  /**
   * Starting target slot.
   *
   * The rest of the slots will start from this so they can be calculated when
   * doing/undoing.
   */
  int to_slot_ = 0;

  /** To track position. */
  unsigned int to_track_name_hash_ = 0;

  /** Whether the plugins will be copied/moved into
   * a new channel, if applicable. */
  bool new_channel_ = false;

  /** Number of plugins to create, when creating new plugins. */
  int num_plugins_ = 0;

  /** Used when changing status. */
  int new_val_ = 0;

  /** Used when changing load behavior. */
  zrythm::gui::dsp::plugins::CarlaBridgeMode new_bridge_mode_ =
    zrythm::gui::dsp::plugins::CarlaBridgeMode::None;

  /**
   * PluginSetting to use when creating.
   */
  std::unique_ptr<PluginSetting> setting_;

  /**
   * Clone of mixer selections at start.
   */
  std::unique_ptr<FullMixerSelections> ms_before_;

  std::unique_ptr<MixerSelections> ms_before_simple_;

  /**
   * Deleted plugins (ie, plugins replaced during move/copy).
   *
   * Used during undo to bring them back.
   */
  std::unique_ptr<FullMixerSelections> deleted_ms_;

  /**
   * Automation tracks associated with the deleted plugins.
   *
   * These are used when undoing so we can readd the automation events, if
   * applicable.
   */
  std::vector<std::unique_ptr<AutomationTrack>> deleted_ats_;

  /**
   * Automation tracks associated with the plugins.
   *
   * These are used when undoing so we can readd
   * the automation events, if applicable.
   */
  std::vector<std::unique_ptr<AutomationTrack>> ats_;
};

class MixerSelectionsCreateAction : public MixerSelectionsAction
{
public:
  MixerSelectionsCreateAction (
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    const Track                              &to_track,
    int                                       to_slot,
    const PluginSetting                      &setting,
    int                                       num_plugins = 1)
      : MixerSelectionsAction (
          nullptr,
          nullptr,
          MixerSelectionsAction::Type::Create,
          slot_type,
          to_track.name_hash_,
          to_slot,
          &setting,
          num_plugins,
          0,
          zrythm::gui::dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsTargetedAction : public MixerSelectionsAction
{
public:
  MixerSelectionsTargetedAction (
    const FullMixerSelections                &ms,
    const PortConnectionsManager             &connections_mgr,
    MixerSelectionsAction::Type               type,
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    const Track *                             to_track,
    int                                       to_slot)
      : MixerSelectionsAction (
          &ms,
          &connections_mgr,
          type,
          slot_type,
          to_track ? to_track->name_hash_ : 0,
          to_slot,
          nullptr,
          0,
          0,
          zrythm::gui::dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsCopyAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsCopyAction (
    const FullMixerSelections                &ms,
    const PortConnectionsManager             &connections_mgr,
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    const Track *                             to_track,
    int                                       to_slot)
      : MixerSelectionsTargetedAction (
          ms,
          connections_mgr,
          MixerSelectionsAction::Type::Copy,
          slot_type,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsPasteAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsPasteAction (
    const FullMixerSelections                &ms,
    const PortConnectionsManager             &connections_mgr,
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    const Track *                             to_track,
    int                                       to_slot)
      : MixerSelectionsTargetedAction (
          ms,
          connections_mgr,
          MixerSelectionsAction::Type::Paste,
          slot_type,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsMoveAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsMoveAction (
    const FullMixerSelections                &ms,
    const PortConnectionsManager             &connections_mgr,
    zrythm::gui::dsp::plugins::PluginSlotType slot_type,
    const Track *                             to_track,
    int                                       to_slot)
      : MixerSelectionsTargetedAction (
          ms,
          connections_mgr,
          MixerSelectionsAction::Type::Move,
          slot_type,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsDeleteAction : public MixerSelectionsAction
{
public:
  MixerSelectionsDeleteAction (
    const FullMixerSelections    &ms,
    const PortConnectionsManager &connections_mgr)
      : MixerSelectionsAction (
          &ms,
          &connections_mgr,
          MixerSelectionsAction::Type::Delete,
          zrythm::gui::dsp::plugins::PluginSlotType::Invalid,
          0,
          0,
          0,
          0,
          0,
          zrythm::gui::dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsChangeStatusAction : public MixerSelectionsAction
{
public:
  MixerSelectionsChangeStatusAction (const FullMixerSelections &ms, int new_val)
      : MixerSelectionsAction (
          &ms,
          nullptr,
          MixerSelectionsAction::Type::ChangeStatus,
          zrythm::gui::dsp::plugins::PluginSlotType::Invalid,
          0,
          0,
          0,
          0,
          new_val,
          zrythm::gui::dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsChangeLoadBehaviorAction : public MixerSelectionsAction
{
public:
  MixerSelectionsChangeLoadBehaviorAction (
    const FullMixerSelections                 &ms,
    zrythm::gui::dsp::plugins::CarlaBridgeMode new_bridge_mode)
      : MixerSelectionsAction (
          &ms,
          nullptr,
          MixerSelectionsAction::Type::ChangeLoadBehavior,
          zrythm::gui::dsp::plugins::PluginSlotType::Invalid,
          0,
          0,
          0,
          0,
          0,
          new_bridge_mode)
  {
  }
};

}; // namespace zrythm::actions

#endif
