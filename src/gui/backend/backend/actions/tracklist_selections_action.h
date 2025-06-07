// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_connections_manager.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/io/file_descriptor.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_span.h"
#include "utils/color.h"

namespace zrythm::gui::actions
{

/**
 * Tracklist selections (tracks) action.
 */
class TracklistSelectionsAction : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (TracklistSelectionsAction)

  using TrackSpan = structure::tracks::TrackSpan;
  using Track = structure::tracks::Track;
  using TrackPtrVariant = structure::tracks::TrackPtrVariant;
  using ChannelSend = structure::tracks::ChannelSend;

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

  using Position = zrythm::dsp::Position;
  using Color = zrythm::utils::Color;
  using PortType = zrythm::dsp::PortType;
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

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
    std::optional<TrackSpan>       tls_before_var,
    std::optional<TrackSpan>       tls_after_var,
    const PortConnectionsManager * port_connections_mgr,
    std::optional<TrackPtrVariant> track_var,
    Track::Type                    track_type,
    const PluginConfiguration *    pl_setting,
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
    const utils::Utf8String *      new_txt,
    bool                           already_edited);

  /**
   * @brief Constructor for a Create action.

   */
  TracklistSelectionsAction (
    Track::Type                 track_type,
    const PluginConfiguration * pl_setting,
    const FileDescriptor *      file_descr,
    int                         track_pos,
    const Position *            pos,
    int                         num_tracks,
    int                         disable_track_pos)
      : TracklistSelectionsAction (
          Type::Create,
          std::nullopt,
          std::nullopt,
          nullptr,
          std::nullopt,
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

  QString to_string () const final;

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

  void get_plugins (
    std::vector<zrythm::gui::old_dsp::plugins::Plugin *> &plugins) override
  {
    if (tls_before_)
      {
        TrackSpan{ *tls_before_ }.get_plugins (plugins);
      }
    if (tls_after_)
      {
        TrackSpan{ *tls_after_ }.get_plugins (plugins);
      }
    if (foldable_tls_before_)
      {
        TrackSpan{ *foldable_tls_before_ }.get_plugins (plugins);
      }
  }

  bool can_contain_clip () const override { return pool_id_.has_value (); }

  bool contains_clip (const AudioClip &clip) const override;

  friend void init_from (
    TracklistSelectionsAction       &obj,
    const TracklistSelectionsAction &other,
    utils::ObjectCloneType           clone_type);

private:
  void init_loaded_impl () final;
  void perform_impl () final;
  void undo_impl () final;

  /**
   * @brief Copies the track positions from @p sel into @p track_positions,
   * and also updates @ref num_tracks_.
   */
  void copy_track_positions_from_selections (
    std::vector<int> &track_positions,
    TrackSpan         selections_var);

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
  Type tracklist_selections_action_type_{};

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
  Track::Type track_type_{};

  /** Flag to know if we are making an empty track. */
  bool is_empty_ = false;

  /** PluginConfiguration, if making an instrument or bus track
   * from a plugin.
   *
   * If this is empty and the track type is instrument, it is assumed that
   * it's an empty track. */
  std::unique_ptr<PluginConfiguration> pl_setting_;

  /**
   * The basename of the file, if any.
   *
   * This will be used as the track name.
   */
  fs::path file_basename_;

  /**
   * If this is an action to create a MIDI track
   * from a MIDI file, this is the base64
   * representation so that the file does not need
   * to be stored in the project.
   *
   * @note For audio files,
   *   TracklistSelectionsAction.pool_id_ is used.
   */
  utils::Utf8String base64_midi_;

  /**
   * If this is an action to create an Audio track from an audio file, this
   * is the pool ID of the audio file.
   *
   * If this is not -1, this means that an audio file exists in the pool.
   */
  std::optional<AudioClip::Uuid> pool_id_;

  /** Source sends that need to be deleted/ recreated on do/undo. */
  std::vector<std::unique_ptr<ChannelSend>> src_sends_;

  /**
   * Direct out tracks of the original tracks.
   */
  std::vector<std::optional<dsp::PortIdentifier::TrackUuid>> out_track_uuids_;

  /**
   * Number of tracks under folder affected.
   *
   * Counter to be filled while doing to be used when undoing.
   */
  int num_fold_change_tracks_{};

  EditType edit_type_{};

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
  std::optional<std::vector<TrackPtrVariant>> tls_before_;

  /** Clone of the TracklistSelections, if applicable. */
  std::optional<std::vector<TrackPtrVariant>> tls_after_;

  /**
   * Foldable tracks before the change, used when undoing to set the correct
   * sizes.
   */
  std::optional<std::vector<TrackPtrVariant>> foldable_tls_before_;

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
  Color              new_color_;

  utils::Utf8String new_txt_;

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
    Track::Type                 track_type,
    const PluginConfiguration * pl_setting,
    const FileDescriptor *      file_descr,
    int                         track_pos,
    const Position *            pos,
    int                         num_tracks,
    int                         disable_track_pos)
      : TracklistSelectionsAction (
          Type::Create,
          std::nullopt,
          std::nullopt,
          nullptr,
          std::nullopt,
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
    Track::Type                 track_type,
    const PluginConfiguration * pl_setting,
    int                         track_pos,
    int                         num_tracks = 1)
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
    EditType                 edit_type,
    std::optional<TrackSpan> tls_before,
    std::optional<TrackSpan> tls_after,
    bool                     already_edited)
      : TracklistSelectionsAction (
          Type::Edit,
          tls_before,
          tls_after,
          nullptr,
          std::nullopt,
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
    EditType                       type,
    std::optional<TrackPtrVariant> track,
    float                          val_before,
    float                          val_after,
    bool                           already_edited)
      : TracklistSelectionsAction (
          Type::Edit,
          std::nullopt,
          std::nullopt,
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
    EditType                       type,
    std::optional<TrackPtrVariant> track,
    int                            val_after,
    bool                           already_edited)
      : TracklistSelectionsAction (
          Type::Edit,
          std::nullopt,
          std::nullopt,
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
    EditType                 type,
    std::optional<TrackSpan> tls_before,
    int                      val_after,
    bool                     already_edited)
      : TracklistSelectionsAction (
          Type::Edit,
          tls_before,
          std::nullopt,
          nullptr,
          std::nullopt,
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
  MuteTracksAction (TrackSpan tls_before, bool mute_new)
      : MultiTrackIntAction (EditType::Mute, tls_before, mute_new, false)
  {
  }
};

class MuteTrackAction : public MuteTracksAction
{
public:
  MuteTrackAction (TrackPtrVariant track, bool mute_new)
      : MuteTracksAction (TrackSpan{ track }, mute_new)
  {
  }
};

class TrackLaneIntAction : public TracklistSelectionsAction
{
protected:
  template <structure::arrangement::RegionSubclass T>
  TrackLaneIntAction (
    EditType                                   edit_type,
    const structure::tracks::TrackLaneImpl<T> &track_lane,
    int                                        value_after)
      : TracklistSelectionsAction (
          Type::Edit,
          std::nullopt,
          std::nullopt,
          nullptr,
          convert_to_variant<TrackPtrVariant> (track_lane.get_track ()),
          (Track::Type) 0,
          nullptr,
          nullptr,
          -1,
          track_lane.get_index_in_track (),
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
  template <structure::arrangement::RegionSubclass T>
  MuteTrackLaneAction (
    const structure::tracks::TrackLaneImpl<T> &track_lane,
    bool                                       mute_new)
      : TrackLaneIntAction (EditType::MuteLane, track_lane, mute_new)
  {
  }
};

class SoloTracksAction : public MultiTrackIntAction
{
public:
  SoloTracksAction (TrackSpan tls_before, bool solo_new)
      : MultiTrackIntAction (EditType::Solo, tls_before, solo_new, false)
  {
  }
};

class SoloTrackAction : public SoloTracksAction
{
public:
  SoloTrackAction (TrackPtrVariant track, bool solo_new)
      : SoloTracksAction (TrackSpan{ track }, solo_new)
  {
  }
};

class SoloTrackLaneAction : public TrackLaneIntAction
{
public:
  template <structure::arrangement::RegionSubclass T>
  SoloTrackLaneAction (
    const structure::tracks::TrackLaneImpl<T> &track_lane,
    bool                                       solo_new)
      : TrackLaneIntAction (EditType::SoloLane, track_lane, solo_new)
  {
  }
};

class ListenTracksAction : public MultiTrackIntAction
{
public:
  ListenTracksAction (TrackSpan tls_before, bool listen_new)
      : MultiTrackIntAction (EditType::Listen, tls_before, listen_new, false)
  {
  }
};

class ListenTrackAction : public ListenTracksAction
{
public:
  ListenTrackAction (TrackPtrVariant track, bool listen_new)
      : ListenTracksAction (TrackSpan (track), listen_new)
  {
  }
};

class EnableTracksAction : public MultiTrackIntAction
{
public:
  EnableTracksAction (TrackSpan tls_before, bool enable_new)
      : MultiTrackIntAction (EditType::Enable, tls_before, enable_new, false)
  {
  }
};

class EnableTrackAction : public EnableTracksAction
{
public:
  EnableTrackAction (TrackPtrVariant track, bool enable_new)
      : EnableTracksAction (TrackSpan (track), enable_new)
  {
  }
};

class FoldTracksAction : public MultiTrackIntAction
{
public:
  FoldTracksAction (std::optional<TrackSpan> tls_before, bool fold_new)
      : MultiTrackIntAction (EditType::Fold, tls_before, fold_new, false)
  {
  }
};

class TracksDirectOutAction : public TracklistSelectionsAction
{
public:
  TracksDirectOutAction (
    TrackSpan                      tls,
    const PortConnectionsManager  &port_connections_mgr,
    std::optional<TrackPtrVariant> direct_out)
      : TracklistSelectionsAction (
          Type::Edit,
          tls,
          std::nullopt,
          &port_connections_mgr,
          std::nullopt,
          (Track::Type) 0,
          nullptr,
          nullptr,
          -1,
          -1,
          nullptr,
          -1,
          EditType::DirectOut,
          direct_out
            ? std::visit ([&] (auto &&track) { return track->get_index (); }, *direct_out)
            : -1,
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr,
    TrackPtrVariant               direct_out)
      : TracksDirectOutAction (tls, port_connections_mgr, direct_out)
  {
  }
};

class RemoveTracksDirectOutAction : public TracksDirectOutAction
{
public:
  RemoveTracksDirectOutAction (
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr)
      : TracksDirectOutAction (tls, port_connections_mgr, std::nullopt)
  {
  }
};

class EditTracksColorAction : public TracklistSelectionsAction
{
public:
  EditTracksColorAction (TrackSpan tls, const Color &color)
      : TracklistSelectionsAction (
          Type::Edit,
          tls,
          std::nullopt,
          nullptr,
          std::nullopt,
          {},
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
  EditTrackColorAction (TrackPtrVariant track, const Color &color)
      : EditTracksColorAction (TrackSpan (track), color)
  {
  }
};

class EditTracksTextAction : public TracklistSelectionsAction
{
protected:
  EditTracksTextAction (
    EditType                 edit_type,
    TrackSpan                tls,
    const utils::Utf8String &txt)
      : TracklistSelectionsAction (
          Type::Edit,
          tls,
          std::nullopt,
          nullptr,
          std::nullopt,
          {},
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
  EditTracksCommentAction (TrackSpan tls, const utils::Utf8String &comment)
      : EditTracksTextAction (EditType::Comment, tls, comment)
  {
  }
};

class EditTrackCommentAction : public EditTracksCommentAction
{
public:
  EditTrackCommentAction (TrackPtrVariant track, const utils::Utf8String &comment)
      : EditTracksCommentAction (TrackSpan (track), comment)
  {
  }
};

class EditTracksIconAction : public EditTracksTextAction
{
public:
  EditTracksIconAction (TrackSpan tls, const utils::Utf8String &icon)
      : EditTracksTextAction (EditType::Icon, tls, icon)
  {
  }
};

class EditTrackIconAction : public EditTracksIconAction
{
public:
  EditTrackIconAction (TrackPtrVariant track, const utils::Utf8String &icon)
      : EditTracksIconAction (TrackSpan (track), icon)
  {
  }
};

class RenameTrackAction : public TracklistSelectionsAction
{
public:
  RenameTrackAction (
    TrackPtrVariant               track,
    const PortConnectionsManager &port_connections_mgr,
    const utils::Utf8String      &name)
      : TracklistSelectionsAction (
          Type::Edit,
          std::nullopt,
          std::nullopt,
          &port_connections_mgr,
          track,
          {},
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
  template <structure::arrangement::RegionSubclass T>
  RenameTrackLaneAction (
    const structure::tracks::TrackLaneImpl<T> &track_lane,
    const utils::Utf8String                   &name)
      : TracklistSelectionsAction (
          Type::Edit,
          std::nullopt,
          std::nullopt,
          nullptr,
          convert_to_variant<TrackPtrVariant> (track_lane.get_track ()),
          {},
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
  MoveTracksAction (TrackSpan tls, int track_pos)
      : TracklistSelectionsAction (
          Type::Move,
          tls,
          std::nullopt,
          nullptr,
          std::nullopt,
          {},
          nullptr,
          nullptr,
          track_pos,
          -1,
          nullptr,
          -1,
          {},
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr,
    int                           track_pos)
      : TracklistSelectionsAction (
          Type::Copy,
          tls,
          std::nullopt,
          &port_connections_mgr,
          std::nullopt,
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
  MoveTracksInsideFoldableTrackAction (TrackSpan tls, int track_pos)
      : TracklistSelectionsAction (
          Type::MoveInside,
          tls,
          std::nullopt,
          nullptr,
          std::nullopt,
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr,
    int                           track_pos)
      : TracklistSelectionsAction (
          Type::CopyInside,
          tls,
          std::nullopt,
          &port_connections_mgr,
          std::nullopt,
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr)
      : TracklistSelectionsAction (
          Type::Delete,
          tls,
          std::nullopt,
          &port_connections_mgr,
          std::nullopt,
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr,
    bool                          pin)
      : TracklistSelectionsAction (
          pin ? Type::Pin : Type::Unpin,
          tls,
          std::nullopt,
          &port_connections_mgr,
          std::nullopt,
          {},
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
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr)
      : PinOrUnpinTracksAction (tls, port_connections_mgr, true)
  {
  }
};

class UnpinTracksAction : public PinOrUnpinTracksAction
{
public:
  UnpinTracksAction (
    TrackSpan                     tls,
    const PortConnectionsManager &port_connections_mgr)
      : PinOrUnpinTracksAction (tls, port_connections_mgr, false)
  {
  }
};

}; // namespace zrythm::gui::actions
