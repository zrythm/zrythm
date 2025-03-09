// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_MIXER_SELECTIONS_ACTION_H__
#define __UNDO_MIXER_SELECTIONS_ACTION_H__

#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/plugin_span.h"
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

  using PluginSlotType = zrythm::dsp::PluginSlotType;
  using Plugin = old_dsp::plugins::Plugin;
  using PluginUuid = Plugin::PluginUuid;
  using PluginPtrVariant = old_dsp::plugins::PluginPtrVariant;
  using CarlaNativePlugin = old_dsp::plugins::CarlaNativePlugin;

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
    std::optional<PluginSpanVariant>               ms,
    const PortConnectionsManager *                 connections_mgr,
    Type                                           type,
    std::optional<Track::TrackUuid>                to_track_id,
    std::optional<dsp::PluginSlot>                 to_slot,
    const PluginSetting *                          setting,
    int                                            num_plugins,
    int                                            new_val,
    zrythm::gui::old_dsp::plugins::CarlaBridgeMode new_bridge_mode,
    QObject *                                      parent = nullptr);

  QString to_string () const override;

  void get_plugins (std::vector<Plugin *> &plugins) override
  {
    if (ms_before_)
      PluginSpan{ *ms_before_ }.get_plugins (plugins);
    if (deleted_ms_)
      PluginSpan{ *deleted_ms_ }.get_plugins (plugins);
  }

  void init_after_cloning (
    const MixerSelectionsAction &other,
    ObjectCloneType              clone_type) override;

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
  void clone_ats (PluginSpanVariant plugins, bool deleted, int start_slot);

  void copy_at_regions (AutomationTrack &dest, const AutomationTrack &src);

  /**
   * Reverts automation events from before deletion.
   *
   * @param deleted Whether to use deleted_ats.
   */
  void
  revert_automation (AutomatableTrack &track, dsp::PluginSlot slot, bool deleted);

  /**
   * Save an existing plugin about to be replaced into @p tmp_ms.
   *
   * FIXME: what is the point of @p from_slot ?
   */
  void save_existing_plugin (
    std::vector<PluginPtrVariant> tmp_plugins,
    Track *                       from_tr,
    dsp::PluginSlot               from_slot,
    Track *                       to_tr,
    dsp::PluginSlot               to_slot);

  void revert_deleted_plugin (Track &to_tr, dsp::PluginSlot to_slot);

  void do_or_undo_create_or_delete (bool do_it, bool create);
  void do_or_undo_change_status (bool do_it);
  void do_or_undo_change_load_behavior (bool do_it);
  void do_or_undo_move_or_copy (bool do_it, bool copy);
  void do_or_undo (bool do_it);

public:
  Type mixer_selections_action_type_ = Type ();

  /**
   * Starting target slot.
   *
   * The rest of the slots will start from this so they can be calculated when
   * doing/undoing.
   */
  std::optional<dsp::PluginSlot> to_slot_;

  /** To track position. */
  std::optional<Track::TrackUuid> to_track_uuid_;

  /** Whether the plugins will be copied/moved into
   * a new channel, if applicable. */
  bool new_channel_ = false;

  /** Number of plugins to create, when creating new plugins. */
  int num_plugins_ = 0;

  /** Used when changing status. */
  int new_val_ = 0;

  /** Used when changing load behavior. */
  zrythm::gui::old_dsp::plugins::CarlaBridgeMode new_bridge_mode_ =
    zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None;

  /**
   * PluginSetting to use when creating.
   */
  std::unique_ptr<PluginSetting> setting_;

  /**
   * Clone of mixer selections at start.
   */
  std::optional<std::vector<PluginPtrVariant>> ms_before_;

  std::optional<std::vector<PluginUuid>> ms_before_simple_;

  /**
   * Deleted plugins (ie, plugins replaced during move/copy).
   *
   * Used during undo to bring them back.
   */
  std::optional<std::vector<PluginPtrVariant>> deleted_ms_;

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
    const Track         &to_track,
    dsp::PluginSlot      to_slot,
    const PluginSetting &setting,
    int                  num_plugins = 1)
      : MixerSelectionsAction (
          std::nullopt,
          nullptr,
          MixerSelectionsAction::Type::Create,
          to_track.get_uuid (),
          to_slot,
          &setting,
          num_plugins,
          0,
          zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsTargetedAction : public MixerSelectionsAction
{
public:
  MixerSelectionsTargetedAction (
    PluginSpanVariant             plugins,
    const PortConnectionsManager &connections_mgr,
    MixerSelectionsAction::Type   type,
    const Track *                 to_track,
    dsp::PluginSlot               to_slot)
      : MixerSelectionsAction (
          plugins,
          &connections_mgr,
          type,
          to_track ? std::make_optional (to_track->get_uuid ()) : std::nullopt,
          to_slot,
          nullptr,
          0,
          0,
          zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsCopyAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsCopyAction (
    PluginSpanVariant             plugins,
    const PortConnectionsManager &connections_mgr,
    const Track *                 to_track,
    dsp::PluginSlot               to_slot)
      : MixerSelectionsTargetedAction (
          plugins,
          connections_mgr,
          MixerSelectionsAction::Type::Copy,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsPasteAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsPasteAction (
    PluginSpanVariant             plugins,
    const PortConnectionsManager &connections_mgr,
    const Track *                 to_track,
    dsp::PluginSlot               to_slot)
      : MixerSelectionsTargetedAction (
          plugins,
          connections_mgr,
          MixerSelectionsAction::Type::Paste,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsMoveAction : public MixerSelectionsTargetedAction
{
public:
  MixerSelectionsMoveAction (
    PluginSpanVariant             plugins,
    const PortConnectionsManager &connections_mgr,
    const Track *                 to_track,
    dsp::PluginSlot               to_slot)
      : MixerSelectionsTargetedAction (
          plugins,
          connections_mgr,
          MixerSelectionsAction::Type::Move,
          to_track,
          to_slot)
  {
  }
};

class MixerSelectionsDeleteAction : public MixerSelectionsAction
{
public:
  MixerSelectionsDeleteAction (
    PluginSpanVariant             plugins,
    const PortConnectionsManager &connections_mgr)
      : MixerSelectionsAction (
          plugins,
          &connections_mgr,
          MixerSelectionsAction::Type::Delete,
          std::nullopt,
          std::nullopt,
          nullptr,
          0,
          0,
          zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsChangeStatusAction : public MixerSelectionsAction
{
public:
  MixerSelectionsChangeStatusAction (PluginSpanVariant plugins, int new_val)
      : MixerSelectionsAction (
          plugins,
          nullptr,
          MixerSelectionsAction::Type::ChangeStatus,
          std::nullopt,
          std::nullopt,
          nullptr,
          0,
          new_val,
          zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None)
  {
  }
};

class MixerSelectionsChangeLoadBehaviorAction : public MixerSelectionsAction
{
public:
  MixerSelectionsChangeLoadBehaviorAction (
    PluginSpanVariant                              plugins,
    zrythm::gui::old_dsp::plugins::CarlaBridgeMode new_bridge_mode)
      : MixerSelectionsAction (
          plugins,
          nullptr,
          MixerSelectionsAction::Type::ChangeLoadBehavior,
          std::nullopt,
          std::nullopt,
          nullptr,
          0,
          0,
          new_bridge_mode)
  {
  }
};

}; // namespace zrythm::actions

#endif
