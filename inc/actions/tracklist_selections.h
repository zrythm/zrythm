// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTIONS_TRACKLIST_SELECTIONS_ACTION_H__
#define __ACTIONS_TRACKLIST_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/channel_send.h"
#include "dsp/port_connections_manager.h"
#include "dsp/track.h"
#include "gui/backend/tracklist_selections.h"
#include "io/file_descriptor.h"
#include "settings/plugin_settings.h"
#include "utils/color.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Tracklist selections (tracks) action.
 */
class TracklistSelectionsAction
    : public UndoableAction,
      public ICloneable<TracklistSelectionsAction>,
      public ISerializable<TracklistSelectionsAction>
{
public:
  enum class Type
  {
    Copy,
    CopyInside,
    Create,
    Delete,
    Edit,
    Move,
    MoveInside,
    Pin,
    Unpin,
  };

  enum class EditType
  {
    Solo,
    SoloLane,
    Mute,
    MuteLane,
    Listen,
    Enable,
    Fold,
    Volume,
    Pan,

    /** Direct out change. */
    DirectOut,

    /** Rename track. */
    Rename,

    /** Rename lane. */
    RenameLane,

    Color,
    Comment,
    Icon,

    MidiFaderMode,
  };

public:
  TracklistSelectionsAction () = default;

  /**
   * Creates a new TracklistSelectionsAction.
   *
   * @param tls_before Tracklist selections to act upon.
   * @param port_connections_mgr Port connections manager at the start of the
   * action.
   * @param pos Position to make the tracks at.
   * @param pl_setting Plugin setting, if any.
   * @param track Track, if single-track action. Used if @p tls_before and @p
   * tls_after are NULL.
   * @throw ZrythmException on error.
   */
  TracklistSelectionsAction (
    Type                           type,
    const TracklistSelections *    tls_before,
    const TracklistSelections *    tls_after,
    const PortConnectionsManager * port_connections_mgr,
    const Track *                  track,
    Track::Type                    track_type,
    const PluginSetting *          pl_setting,
    const FileDescriptor *         file_descr,
    int                            track_pos,
    int                            lane_pos,
    const Position *               pos,
    int                            num_tracks,
    EditType                       edit_type,
    int                            ival_after,
    const Color *                  color_new,
    float                          val_before,
    float                          val_after,
    const std::string *            new_txt,
    bool                           already_edited);

  /**
   * @brief Constructor for a Create action.

   */
  TracklistSelectionsAction (
    Track::Type            track_type,
    const PluginSetting *  pl_setting,
    const FileDescriptor * file_descr,
    int                    track_pos,
    const Position *       pos,
    int                    num_tracks,
    int                    disable_track_pos)
      : TracklistSelectionsAction (
        Type::Create,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        track_type,
        pl_setting,
        file_descr,
        track_pos,
        -1,
        pos,
        num_tracks,
        EditType::Color,
        disable_track_pos,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }

  std::string to_string () const final;

  bool needs_transport_total_bar_update (bool perform) const override
  {
    if (tracklist_selections_action_type_ == Type::Edit)
      {
        if (
          edit_type_ == EditType::Mute || edit_type_ == EditType::Solo
          || edit_type_ == EditType::Listen || edit_type_ == EditType::Volume
          || edit_type_ == EditType::Pan)
          {
            return false;
          }
      }

    return true;
  }

  bool needs_pause () const override
  {
    if (tracklist_selections_action_type_ == Type::Edit)
      {
        if (
          edit_type_ == EditType::Mute || edit_type_ == EditType::Solo
          || edit_type_ == EditType::Listen || edit_type_ == EditType::Volume
          || edit_type_ == EditType::Pan)
          {
            return false;
          }
      }
    return true;
  }

  void get_plugins (std::vector<Plugin *> &plugins) override
  {
    if (tls_before_)
      tls_before_->get_plugins (plugins);
    if (tls_after_)
      tls_after_->get_plugins (plugins);
    if (foldable_tls_before_)
      foldable_tls_before_->get_plugins (plugins);
  }

  bool can_contain_clip () const override { return pool_id_ >= 0; }

  bool contains_clip (const AudioClip &clip) const override;

  void init_after_cloning (const TracklistSelectionsAction &other) final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () final
  {
    if (tls_before_)
      {
        tls_before_->init_loaded ();
      }
    if (tls_after_)
      {
        tls_after_->init_loaded ();
      }
    if (foldable_tls_before_)
      {
        foldable_tls_before_->init_loaded ();
      }
    for (auto &send : src_sends_)
      {
        send->init_loaded (nullptr);
      }
  }

  void perform_impl () final;
  void undo_impl () final;

  /**
   * @brief Copies the track positions from @p sel into @p track_positions, and
   * also updates @ref num_tracks_.
   */
  void copy_track_positions_from_selections (
    std::vector<int>          &track_positions,
    const TracklistSelections &sel);

  /**
   * Resets the foldable track sizes when undoing an action.
   *
   * @note Must only be used during undo.
   */
  void reset_foldable_track_sizes ();

  /**
   * @brief Validates this instance.
   *
   * Only used in the constructor to throw an exception if the action is
   * invalid (due to invalid parameters).
   */
  bool validate () const;

  /**
   * @brief Creates a track at index @p idx.
   *
   * @param idx
   */
  void create_track (int idx);

  /* all these may throw ZrythmException */

  void do_or_undo_create_or_delete (bool do_it, bool create);

  /**
   * @param inside Whether moving/copying inside a foldable track.
   */
  void do_or_undo_move_or_copy (bool do_it, bool copy, bool inside);

  void do_or_undo_edit (bool do_it);

  void do_or_undo (bool do_it);

public:
  /** Type of action. */
  Type tracklist_selections_action_type_ = (Type) 0;

  /** Position to make the tracks at.
   *
   * Used when undoing too. */
  int track_pos_ = 0;

  /** Lane position, if editing lane. */
  int lane_pos_ = 0;

  /** Position to add the audio region to, if
   * applicable. */
  Position pos_;

  bool have_pos_ = false;

  /** Track type. */
  Track::Type track_type_ = (Track::Type) 0;

  /** Flag to know if we are making an empty track. */
  bool is_empty_ = false;

  /** PluginSetting, if making an instrument or bus track from a plugin.
   *
   * If this is empty and the track type is instrument, it is assumed that
   * it's an empty track. */
  std::unique_ptr<PluginSetting> pl_setting_;

  /**
   * The basename of the file, if any.
   *
   * This will be used as the track name.
   */
  std::string file_basename_;

  /**
   * If this is an action to create a MIDI track
   * from a MIDI file, this is the base64
   * representation so that the file does not need
   * to be stored in the project.
   *
   * @note For audio files,
   *   TracklistSelectionsAction.pool_id_ is used.
   */
  std::string base64_midi_;

  /**
   * If this is an action to create an Audio track from an audio file, this
   * is the pool ID of the audio file.
   *
   * If this is not -1, this means that an audio file exists in the pool.
   */
  int pool_id_ = -1;

  /** Source sends that need to be deleted/ recreated on do/undo. */
  std::vector<std::unique_ptr<ChannelSend>> src_sends_;

  /**
   * Direct out tracks of the original tracks.
   *
   * These are track name hashes.
   */
  std::vector<unsigned int> out_track_hashes_;

  /**
   * Number of tracks under folder affected.
   *
   * Counter to be filled while doing to be used when undoing.
   */
  int num_fold_change_tracks_ = 0;

  EditType edit_type_ = (EditType) 0;

  /**
   * Track positions.
   *
   * Used for actions where full selection clones are not needed.
   */
  std::vector<int> track_positions_before_;
  std::vector<int> track_positions_after_;

  /**
   * @brief Number of tracks.
   *
   * This counter is used in various cases.
   */
  int num_tracks_ = 0;

  /** Clone of the TracklistSelections, if applicable. */
  std::unique_ptr<TracklistSelections> tls_before_;

  /** Clone of the TracklistSelections, if applicable. */
  std::unique_ptr<TracklistSelections> tls_after_;

  /**
   * Foldable tracks before the change, used when undoing to set the correct
   * sizes.
   */
  std::unique_ptr<TracklistSelections> foldable_tls_before_;

  /* --------------- DELTAS ---------------- */

  /**
   * Int value.
   *
   * Also used for bool.
   */
  std::vector<int> ival_before_;
  int              ival_after_ = 0;

  /* -------------- end DELTAS ------------- */

  std::vector<Color> colors_before_;
  Color              new_color_ = { 0, 0, 0, 0 };

  std::string new_txt_;

  /** Skip do if true. */
  bool already_edited_ = false;

  /** Float values. */
  float val_before_ = 0.f;
  float val_after_ = 0.f;
};

class CreateTracksAction : public TracklistSelectionsAction
{
public:
  /**
   * @brief Create a Tracks Action object
   *
   * @param disable_track_pos Position of track to disable, or -1.
   */
  CreateTracksAction (
    Track::Type            track_type,
    const PluginSetting *  pl_setting,
    const FileDescriptor * file_descr,
    int                    track_pos,
    const Position *       pos,
    int                    num_tracks,
    int                    disable_track_pos)
      : TracklistSelectionsAction (
        Type::Create,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        track_type,
        pl_setting,
        file_descr,
        track_pos,
        -1,
        pos,
        num_tracks,
        EditType::Color,
        disable_track_pos,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

/**
 * @brief To be used when creating tracks of a given type with a plugin.
 *
 */
class CreatePluginTrackAction : public CreateTracksAction
{
public:
  CreatePluginTrackAction (
    Track::Type           track_type,
    const PluginSetting * pl_setting,
    int                   track_pos,
    int                   num_tracks = 1)
      : CreateTracksAction (
        track_type,
        pl_setting,
        nullptr,
        track_pos,
        nullptr,
        num_tracks,
        -1)
  {
  }
};

/**
 * @brief To be used when creating tracks from a given type with no
 * plugins/files.
 */
class CreatePlainTrackAction : public CreatePluginTrackAction
{
public:
  CreatePlainTrackAction (
    Track::Type track_type,
    int         track_pos,
    int         num_tracks = 1)
      : CreatePluginTrackAction (track_type, nullptr, track_pos, num_tracks)
  {
  }
};

/**
 * @brief Generic edit action.
 */
class EditTracksAction : public TracklistSelectionsAction
{
public:
  EditTracksAction (
    EditType                    edit_type,
    const TracklistSelections * tls_before,
    const TracklistSelections * tls_after,
    bool                        already_edited)
      : TracklistSelectionsAction (
        Type::Edit,
        tls_before,
        tls_after,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        edit_type,
        0,
        nullptr,
        0.f,
        0.f,
        nullptr,
        already_edited)
  {
  }
};

/**
 * @brief Wrapper for editing single-track float values.
 */
class SingleTrackFloatAction : public TracklistSelectionsAction
{
public:
  SingleTrackFloatAction (
    EditType      type,
    const Track * track,
    float         val_before,
    float         val_after,
    bool          already_edited)
      : TracklistSelectionsAction (
        Type::Edit,
        nullptr,
        nullptr,
        nullptr,
        track,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        type,
        0,
        nullptr,
        val_before,
        val_after,
        nullptr,
        already_edited)
  {
  }
};

/**
 * @brief Wrapper for editing single-track int values.
 */
class SingleTrackIntAction : public TracklistSelectionsAction
{
public:
  SingleTrackIntAction (
    EditType      type,
    const Track * track,
    int           val_after,
    bool          already_edited)
      : TracklistSelectionsAction (
        Type::Edit,
        nullptr,
        nullptr,
        nullptr,
        track,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        type,
        val_after,
        nullptr,
        0.f,
        0.f,
        nullptr,
        already_edited)
  {
  }
};

/**
 * @brief Wrapper for editing multi-track int values.
 */
class MultiTrackIntAction : public TracklistSelectionsAction
{
protected:
  MultiTrackIntAction (
    EditType                    type,
    const TracklistSelections * tls_before,
    int                         val_after,
    bool                        already_edited)
      : TracklistSelectionsAction (
        Type::Edit,
        tls_before,
        nullptr,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        type,
        val_after,
        nullptr,
        0.f,
        0.f,
        nullptr,
        already_edited)
  {
  }
};

class MuteTracksAction : public MultiTrackIntAction
{
public:
  MuteTracksAction (const TracklistSelections &tls_before, bool mute_new)
      : MultiTrackIntAction (EditType::Mute, &tls_before, mute_new, false)
  {
  }
};

class MuteTrackAction : public MuteTracksAction
{
public:
  MuteTrackAction (const Track &track, bool mute_new)
      : MuteTracksAction (TracklistSelections (track), mute_new)
  {
  }
};

class TrackLaneIntAction : public TracklistSelectionsAction
{
protected:
  template <RegionSubclass T>
  TrackLaneIntAction (
    EditType                edit_type,
    const TrackLaneImpl<T> &track_lane,
    int                     value_after)
      : TracklistSelectionsAction (
        Type::Edit,
        nullptr,
        nullptr,
        nullptr,
        track_lane.get_track (),
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        track_lane.pos_,
        nullptr,
        -1,
        edit_type,
        value_after,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class MuteTrackLaneAction : public TrackLaneIntAction
{
public:
  template <RegionSubclass T>
  MuteTrackLaneAction (const TrackLaneImpl<T> &track_lane, bool mute_new)
      : TrackLaneIntAction (EditType::MuteLane, track_lane, mute_new)
  {
  }
};

class SoloTracksAction : public MultiTrackIntAction
{
public:
  SoloTracksAction (const TracklistSelections &tls_before, bool solo_new)
      : MultiTrackIntAction (EditType::Solo, &tls_before, solo_new, false)
  {
  }
};

class SoloTrackAction : public SoloTracksAction
{
public:
  SoloTrackAction (const Track &track, bool solo_new)
      : SoloTracksAction (TracklistSelections (track), solo_new)
  {
  }
};

class SoloTrackLaneAction : public TrackLaneIntAction
{
public:
  template <RegionSubclass T>
  SoloTrackLaneAction (const TrackLaneImpl<T> &track_lane, bool solo_new)
      : TrackLaneIntAction (EditType::SoloLane, track_lane, solo_new)
  {
  }
};

class ListenTracksAction : public MultiTrackIntAction
{
public:
  ListenTracksAction (const TracklistSelections &tls_before, bool listen_new)
      : MultiTrackIntAction (EditType::Listen, &tls_before, listen_new, false)
  {
  }
};

class ListenTrackAction : public ListenTracksAction
{
public:
  ListenTrackAction (const Track &track, bool listen_new)
      : ListenTracksAction (TracklistSelections (track), listen_new)
  {
  }
};

class EnableTracksAction : public MultiTrackIntAction
{
public:
  EnableTracksAction (const TracklistSelections &tls_before, bool enable_new)
      : MultiTrackIntAction (EditType::Enable, &tls_before, enable_new, false)
  {
  }
};

class EnableTrackAction : public EnableTracksAction
{
public:
  EnableTrackAction (const Track &track, bool enable_new)
      : EnableTracksAction (TracklistSelections (track), enable_new)
  {
  }
};

class FoldTracksAction : public MultiTrackIntAction
{
public:
  FoldTracksAction (const TracklistSelections * tls_before, bool fold_new)
      : MultiTrackIntAction (EditType::Fold, tls_before, fold_new, false)
  {
  }
};

class TracksDirectOutAction : public TracklistSelectionsAction
{
public:
  TracksDirectOutAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr,
    const Track *                 direct_out)
      : TracklistSelectionsAction (
        Type::Edit,
        &tls,
        nullptr,
        &port_connections_mgr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        EditType::DirectOut,
        direct_out ? direct_out->pos_ : -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class ChangeTracksDirectOutAction : public TracksDirectOutAction
{
public:
  ChangeTracksDirectOutAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr,
    const Track                  &direct_out)
      : TracksDirectOutAction (tls, port_connections_mgr, &direct_out)
  {
  }
};

class RemoveTracksDirectOutAction : public TracksDirectOutAction
{
public:
  RemoveTracksDirectOutAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr)
      : TracksDirectOutAction (tls, port_connections_mgr, nullptr)
  {
  }
};

class EditTracksColorAction : public TracklistSelectionsAction
{
public:
  EditTracksColorAction (const TracklistSelections &tls, const Color &color)
      : TracklistSelectionsAction (
        Type::Edit,
        &tls,
        nullptr,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        EditType::Color,
        -1,
        &color,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class EditTrackColorAction : public EditTracksColorAction
{
public:
  EditTrackColorAction (const Track &track, const Color &color)
      : EditTracksColorAction (TracklistSelections (track), color)
  {
  }
};

class EditTracksTextAction : public TracklistSelectionsAction
{
protected:
  EditTracksTextAction (
    EditType                   edit_type,
    const TracklistSelections &tls,
    const std::string         &txt)
      : TracklistSelectionsAction (
        Type::Edit,
        &tls,
        nullptr,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        edit_type,
        -1,
        nullptr,
        0.f,
        0.f,
        &txt,
        false)
  {
  }
};

class EditTracksCommentAction : public EditTracksTextAction
{
public:
  EditTracksCommentAction (
    const TracklistSelections &tls,
    const std::string         &comment)
      : EditTracksTextAction (EditType::Comment, tls, comment)
  {
  }
};

class EditTrackCommentAction : public EditTracksCommentAction
{
public:
  EditTrackCommentAction (const Track &track, const std::string &comment)
      : EditTracksCommentAction (TracklistSelections (track), comment)
  {
  }
};

class EditTracksIconAction : public EditTracksTextAction
{
public:
  EditTracksIconAction (const TracklistSelections &tls, const std::string &icon)
      : EditTracksTextAction (EditType::Icon, tls, icon)
  {
  }
};

class EditTrackIconAction : public EditTracksIconAction
{
public:
  EditTrackIconAction (const Track &track, const std::string &icon)
      : EditTracksIconAction (TracklistSelections (track), icon)
  {
  }
};

class RenameTrackAction : public TracklistSelectionsAction
{
public:
  RenameTrackAction (
    const Track                  &track,
    const PortConnectionsManager &port_connections_mgr,
    const std::string            &name)
      : TracklistSelectionsAction (
        Type::Edit,
        nullptr,
        nullptr,
        &port_connections_mgr,
        &track,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        EditType::Rename,
        -1,
        nullptr,
        -1,
        0.f,
        &name,
        false)
  {
  }
};

class RenameTrackLaneAction : public TracklistSelectionsAction
{
public:
  template <RegionSubclass T>
  RenameTrackLaneAction (
    const TrackLaneImpl<T> &track_lane,
    const std::string      &name)
      : TracklistSelectionsAction (
        Type::Edit,
        nullptr,
        nullptr,
        nullptr,
        track_lane.get_track (),
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        EditType::RenameLane,
        -1,
        nullptr,
        -1,
        0.f,
        &name,
        false)
  {
  }
};

class MoveTracksAction : public TracklistSelectionsAction
{
public:
  /**
   * Move @p tls to @p track_pos.
   *
   * Tracks starting at @p track_pos will be pushed down.
   *
   * @param track_pos Track position indicating the track to push down and
   * insert the selections above. This is the track position before the move
   * will be executed.
   */
  MoveTracksAction (const TracklistSelections &tls, int track_pos)
      : TracklistSelectionsAction (
        Type::Move,
        &tls,
        nullptr,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        track_pos,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class CopyTracksAction : public TracklistSelectionsAction
{
public:
  CopyTracksAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr,
    int                           track_pos)
      : TracklistSelectionsAction (
        Type::Copy,
        &tls,
        nullptr,
        &port_connections_mgr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        track_pos,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class MoveTracksInsideFoldableTrackAction : public TracklistSelectionsAction
{
public:
  /**
   * Move inside a foldable track.
   *
   * @param track_pos Foldable track index.
   *
   * When foldable tracks are included in @p tls, all their children must be
   * marked as selected as well before calling this.
   *
   * @note This should be called in combination with a move action to move the
   * tracks to the required index after putting them inside a group.
   */
  MoveTracksInsideFoldableTrackAction (
    const TracklistSelections &tls,
    int                        track_pos)
      : TracklistSelectionsAction (
        Type::MoveInside,
        &tls,
        nullptr,
        nullptr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        track_pos,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class CopyTracksInsideFoldableTrackAction : public TracklistSelectionsAction
{
public:
  CopyTracksInsideFoldableTrackAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr,
    int                           track_pos)
      : TracklistSelectionsAction (
        Type::CopyInside,
        &tls,
        nullptr,
        &port_connections_mgr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        track_pos,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class DeleteTracksAction : public TracklistSelectionsAction
{
public:
  DeleteTracksAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr)
      : TracklistSelectionsAction (
        Type::Delete,
        &tls,
        nullptr,
        &port_connections_mgr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class PinOrUnpinTracksAction : public TracklistSelectionsAction
{
protected:
  PinOrUnpinTracksAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr,
    bool                          pin)
      : TracklistSelectionsAction (
        pin ? Type::Pin : Type::Unpin,
        &tls,
        nullptr,
        &port_connections_mgr,
        nullptr,
        (Track::Type) 0,
        nullptr,
        nullptr,
        -1,
        -1,
        nullptr,
        -1,
        (EditType) 0,
        -1,
        nullptr,
        0.f,
        0.f,
        nullptr,
        false)
  {
  }
};

class PinTracksAction : public PinOrUnpinTracksAction
{
public:
  PinTracksAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr)
      : PinOrUnpinTracksAction (tls, port_connections_mgr, true)
  {
  }
};

class UnpinTracksAction : public PinOrUnpinTracksAction
{
public:
  UnpinTracksAction (
    const TracklistSelections    &tls,
    const PortConnectionsManager &port_connections_mgr)
      : PinOrUnpinTracksAction (tls, port_connections_mgr, false)
  {
  }
};

/**
 * @}
 */

#endif
