// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_bus_track.h"
#include "common/dsp/audio_group_track.h"
#include "common/dsp/audio_region.h"
#include "common/dsp/audio_track.h"
#include "common/dsp/foldable_track.h"
#include "common/dsp/folder_track.h"
#include "common/dsp/group_target_track.h"
#include "common/dsp/instrument_track.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/midi_bus_track.h"
#include "common/dsp/midi_group_track.h"
#include "common/dsp/midi_track.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/io/file_descriptor.h"
#include "common/io/midi_file.h"
#include "common/plugins/plugin.h"
#include "common/utils/base64.h"
#include "common/utils/debug.h"
#include "common/utils/exceptions.h"
#include "common/utils/io.h"
#include "common/utils/traits.h"
#include "common/utils/types.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

void
TracklistSelectionsAction::init_after_cloning (
  const TracklistSelectionsAction &other)
{
  UndoableAction::copy_members_from (other);
  tracklist_selections_action_type_ = other.tracklist_selections_action_type_;
  track_type_ = other.track_type_;
  if (other.pl_setting_)
    pl_setting_ = other.pl_setting_->clone_unique ();
  is_empty_ = other.is_empty_;
  track_pos_ = other.track_pos_;
  lane_pos_ = other.lane_pos_;
  have_pos_ = other.have_pos_;
  pos_ = other.pos_;
  ival_before_ = other.ival_before_;
  colors_before_ = other.colors_before_;
  track_positions_before_ = other.track_positions_before_;
  track_positions_after_ = other.track_positions_after_;
  num_tracks_ = other.num_tracks_;
  ival_after_ = other.ival_after_;
  new_color_ = other.new_color_;
  file_basename_ = other.file_basename_;
  base64_midi_ = other.base64_midi_;
  pool_id_ = other.pool_id_;
  if (other.tls_before_)
    tls_before_ = other.tls_before_->clone_unique ();
  if (other.tls_after_)
    tls_after_ = other.tls_after_->clone_unique ();
  if (other.foldable_tls_before_)
    foldable_tls_before_ = other.foldable_tls_before_->clone_unique ();
  out_track_hashes_ = other.out_track_hashes_;
  clone_unique_ptr_container (src_sends_, other.src_sends_);
  edit_type_ = other.edit_type_;
  new_txt_ = other.new_txt_;
  val_before_ = other.val_before_;
  val_after_ = other.val_after_;
  num_fold_change_tracks_ = other.num_fold_change_tracks_;
}

void
TracklistSelectionsAction::copy_track_positions_from_selections (
  std::vector<int>          &track_positions,
  const TracklistSelections &sel)
{
  num_tracks_ = sel.tracks_.size ();
  for (const auto &track : sel.tracks_)
    {
      track_positions.push_back (track->pos_);
    }
  std::sort (track_positions.begin (), track_positions.end ());
}

/**
 * Resets the foldable track sizes when undoing an action.
 *
 * @note Must only be used during undo.
 */
void
TracklistSelectionsAction::reset_foldable_track_sizes ()
{
  for (auto own_tr : foldable_tls_before_->tracks_ | type_is<FoldableTrack> ())
    {
      auto prj_tr_var = TRACKLIST->find_track_by_name (own_tr->name_);
      std::visit (
        [&] (auto &&prj_tr) {
          using TrackT = base_type<decltype (prj_tr)>;
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              prj_tr->size_ = own_tr->size_;
            }
        },
        *prj_tr_var);
    }
}

bool
TracklistSelectionsAction::contains_clip (const AudioClip &clip) const
{
  return clip.pool_id_ == pool_id_;
}

bool
TracklistSelectionsAction::validate () const
{
  if (tracklist_selections_action_type_ == Type::Delete)
    {
      if (!tls_before_ || tls_before_->contains_undeletable_track ())
        {
          return false;
        }
    }
  else if (tracklist_selections_action_type_ == Type::MoveInside)
    {
      if (!tls_before_ || tls_before_->contains_track_index (track_pos_))
        return false;
    }

  return true;
}

TracklistSelectionsAction::TracklistSelectionsAction (
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
  bool                           already_edited)
    : UndoableAction (
        UndoableAction::Type::TracklistSelections,
        AUDIO_ENGINE->frames_per_tick_,
        AUDIO_ENGINE->sample_rate_),
      tracklist_selections_action_type_ (type), track_pos_ (track_pos),
      lane_pos_ (lane_pos), track_type_ (track_type),
      pl_setting_ (pl_setting ? pl_setting->clone_unique () : nullptr),
      edit_type_ (edit_type), ival_after_ (ival_after),
      already_edited_ (already_edited), val_before_ (val_before),
      val_after_ (val_after)
{
  num_tracks = std::max (num_tracks, 0);

  /* --- validation --- */

  if (tracklist_selections_action_type_ == Type::Create)
    {
      assert (num_tracks > 0);
    }

  if (
    (tracklist_selections_action_type_ == Type::Copy
     || tracklist_selections_action_type_ == Type::CopyInside)
    && tls_before->contains_uncopyable_track ())
    {
      throw ZrythmException (QObject::tr (
        "Cannot copy tracks: selection contains an uncopyable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::Delete
    && tls_before->contains_undeletable_track ())
    {
      throw ZrythmException (QObject::tr (
        "Cannot delete tracks: selection contains an undeletable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::MoveInside
    || tracklist_selections_action_type_ == Type::CopyInside)
    {
      auto _foldable_tr = TRACKLIST->get_track (track_pos);
      std::visit (
        [&] (auto &&foldable_tr) { assert (foldable_tr->is_foldable ()); },
        _foldable_tr);
    }

    /* --- end validation --- */

/* leftover from before - not sure what it was used for but left for reference*/
#if 0
  if (
    tls_before == TRACKLIST_SELECTIONS
    && (type_ != Type::Edit || edit_type_ != EditType::Fold))
    {
      tracklist_selections_select_foldable_children (tls_before);
    }
#endif

  if (pl_setting)
    {
      pl_setting_->validate ();
    }
  else if (!file_descr)
    {
      is_empty_ = 1;
    }
  if (type == Type::Pin)
    {
      track_pos_ = TRACKLIST->pinned_tracks_cutoff_;
    }
  else if (type == Type::Unpin)
    {
      track_pos_ = TRACKLIST->tracks_.size () - 1;
    }

  if (pos)
    {
      pos_ = *pos;
      have_pos_ = true;
    }

  /* calculate number of tracks */
  if (file_descr && track_type == Track::Type::Midi)
    {
      MidiFile mf (file_descr->abs_path_);
      num_tracks_ = mf.get_num_tracks (true);
    }
  else
    {
      num_tracks_ = num_tracks;
    }

  /* create the file in the pool or save base64 if MIDI */
  if (file_descr)
    {
      /* TODO: use async API */
      if (track_type == Track::Type::Midi)
        {
          try
            {
              auto contents =
                utils::io::read_file_contents (file_descr->abs_path_);
              base64_midi_ = utils::base64::encode (contents).toStdString ();
            }
          catch (const std::exception &e)
            {
              throw ZrythmException (fmt::format (
                "Failed to get MIDI file contents: {}", e.what ()));
            }
        }
      else if (track_type == Track::Type::Audio)
        {
          pool_id_ = AUDIO_POOL->add_clip (
            std::make_unique<AudioClip> (file_descr->abs_path_));
        }
      else
        {
          z_return_if_reached ();
        }

      file_basename_ = utils::io::path_get_basename (file_descr->abs_path_);
    }

  bool need_full_selections = true;
  if (type == Type::Edit)
    {
      need_full_selections = false;
    }

  if (tls_before)
    {
      if (tls_before->tracks_.empty ())
        {
          throw ZrythmException (QObject::tr ("No tracks selected"));
        }

      if (need_full_selections)
        {
          tls_before_ = tls_before->clone_unique ();
          tls_before_->sort ();
          foldable_tls_before_ = std::make_unique<TracklistSelections> ();
          for (auto &tr : TRACKLIST->tracks_)
            {
              std::visit (
                [&] (auto &&track_inner) {
                  using TrackT = base_type<decltype (track_inner)>;
                  if constexpr (std::derived_from<TrackT, FoldableTrack>)
                    {
                      auto new_track = track_inner->clone_unique ();
                      foldable_tls_before_->add_track (std::move (new_track));
                    }
                },
                tr);
            }
        }
      else
        {
          copy_track_positions_from_selections (
            track_positions_before_, *tls_before);
        }
    }

  if (tls_after)
    {
      if (need_full_selections)
        {
          tls_after_ = tls_after->clone_unique ();
          tls_after_->sort ();
        }
      else
        {
          copy_track_positions_from_selections (
            track_positions_after_, *tls_after);
        }
    }

  /* if need to clone tls_before */
  if (tls_before && need_full_selections)
    {
      /* save the outputs & incoming sends */
      for (auto &clone_track : tls_before_->tracks_)
        {
          auto clone_channel_track =
            dynamic_cast<ChannelTrack *> (clone_track.get ());
          if (clone_channel_track && clone_channel_track->channel_->has_output_)
            {
              out_track_hashes_.push_back (
                clone_channel_track->channel_->output_name_hash_);
            }
          else
            {
              out_track_hashes_.push_back (0);
            }

          for (
            const auto cur_track : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
            {
              for (
                const auto &send :
                cur_track->channel_->sends_
                  | std::views::filter (&ChannelSend::is_enabled))
                {
                  Track * target_track = send->get_target_track (cur_track);
                  z_return_if_fail (target_track);

                  if (target_track->pos_ == clone_track->pos_)
                    {
                      src_sends_.emplace_back (send->clone_unique ());
                    }
                }
            }
        }
    }

  if (tracklist_selections_action_type_ == Type::Edit && track)
    {
      num_tracks_ = 1;
      track_positions_before_.push_back (track->pos_);
      track_positions_after_.push_back (track->pos_);
    }

  if (color_new)
    {
      new_color_ = *color_new;
    }
  if (new_txt)
    {
      new_txt_ = *new_txt;
    }

  ival_before_.resize (num_tracks_, 0);
  colors_before_.resize (num_tracks_);

  if (port_connections_mgr)
    {
      port_connections_before_ = port_connections_mgr->clone_unique ();
    }

  if (!validate ())
    {
      z_error ("failed to validate tracklist selections action");
    }
}

void
TracklistSelectionsAction::create_track (int idx)
{
  int pos = track_pos_ + idx;

  if (is_empty_)
    {
      auto track_type_str = Track_Type_to_string (track_type_, true);
      auto label = format_qstr (QObject::tr ("{} Track"), track_type_str);
      auto track = Track::create_track (track_type_, label.toStdString (), pos);
      TRACKLIST->insert_track (
        std::move (track), pos, *AUDIO_ENGINE, false, false);
    }
  else /* else if track is not empty */
    {
      std::unique_ptr<zrythm::plugins::Plugin> pl;
      std::unique_ptr<Track>                   track;
      zrythm::plugins::Plugin *                added_pl = nullptr;

      /* if creating audio track from file */
      if (track_type_ == Track::Type::Audio && pool_id_ >= 0)
        {
          track = *AudioTrack::create_unique (file_basename_, pos, sample_rate_);
        }
      /* else if creating MIDI track from file */
      else if (track_type_ == Track::Type::Midi && !base64_midi_.empty ())
        {
          track = *MidiTrack::create_unique (file_basename_, pos);
        }
      /* at this point we can assume it has a plugin */
      else
        {
          const auto &setting = pl_setting_;
          const auto &descr = setting->descr_;

          track = Track::create_track (track_type_, descr->name_, pos);

          pl = setting->create_plugin (track->get_name_hash ());
          pl->instantiate ();
          pl->activate ();
        }

      auto added_track = TRACKLIST->insert_track (
        std::move (track), pos, *AUDIO_ENGINE, false, false);

      if (pl && added_track->has_channel ())
        {
          bool is_instrument = added_track->type_ == Track::Type::Instrument;
          auto channel_track = dynamic_cast<ChannelTrack *> (added_track);
          added_pl = channel_track->channel_->add_plugin (
            std::move (pl),
            is_instrument
              ? zrythm::plugins::PluginSlotType::Instrument
              : zrythm::plugins::PluginSlotType::Insert,
            is_instrument ? -1 : pl->id_.slot_, true, false, true, false, false);
        }

      Position start_pos = have_pos_ ? pos_ : Position ();
      if (track_type_ == Track::Type::Audio)
        {
          /* create an audio region & add to track*/
          added_track->add_region (
            new AudioRegion (
              pool_id_, nullptr, true, nullptr, 0, nullptr, 0,
              ENUM_INT_TO_VALUE (BitDepth, 0), start_pos,
              added_track->get_name_hash (), 0, 0),
            nullptr, 0, true, false);
        }
      else if (
        track_type_ == Track::Type::Midi && !base64_midi_.empty ()
        && !file_basename_.empty ())
        {
          /* create a temporary midi file */
          auto full_path_file = utils::io::make_tmp_file (
            std::make_optional<std::string> ("data.MID"));
          fs::path full_path (full_path_file->fileName ().toStdString ());
          auto     data =
            utils::base64::decode (QByteArray::fromStdString (base64_midi_));
          utils::io::set_file_contents (
            full_path, data.constData (), data.size ());

          /* create a MIDI region from the MIDI file & add to track */
          added_track->add_region (
            new MidiRegion (
              pos_, full_path, added_track->get_name_hash (), 0, 0, idx),
            nullptr, 0, true, false);
        }

      if (
        added_pl && ZRYTHM_HAVE_UI
        && gui::SettingsManager::get_instance ()
             ->get_openPluginsOnInstantiation ())
        {
          added_pl->visible_ = true;
          /* EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, added_pl); */
        }
    }
}

void
TracklistSelectionsAction::do_or_undo_create_or_delete (bool _do, bool create)
{
  PORT_CONNECTIONS_MGR->print ();

  /* if creating tracks (do create or undo delete) */
  if ((create && _do) || (!create && !_do))
    {
      if (create)
        {
          for (int i = 0; i < num_tracks_; i++)
            {
              create_track (i);

              /* TODO select each plugin that was selected */
            }

          /* disable given track, if any (e.g., when bouncing) */
          if (ival_after_ > -1)
            {
              z_return_if_fail (
                ival_after_ < static_cast<int> (TRACKLIST->tracks_.size ()));
              auto _tr_to_disable = TRACKLIST->get_track (ival_after_);
              std::visit (
                [&] (auto &&tr_to_disable) {
                  z_return_if_fail (tr_to_disable);
                  tr_to_disable->set_enabled (false, false, false, true);
                },
                _tr_to_disable);
            }
        }
      /* else if delete undo */
      else
        {
          int num_tracks = tls_before_->tracks_.size ();

          for (int i = 0; i < num_tracks; i++)
            {
              auto &own_track = tls_before_->tracks_[i];

              /* clone our own track */
              auto track =
                clone_unique_with_variant<TrackVariant> (own_track.get ());

              /* remove output */
              if (track->has_channel ())
                {
                  auto channel_track =
                    dynamic_cast<ChannelTrack *> (track.get ());
                  channel_track->get_channel ()->has_output_ = false;
                  channel_track->get_channel ()->output_name_hash_ = 0;
                }

              /* remove the sends (will be added later) */
              if (track->has_channel ())
                {
                  auto channel_track =
                    dynamic_cast<ChannelTrack *> (track.get ());
                  for (auto &send : channel_track->get_channel ()->sends_)
                    {
                      send->enabled_->control_ = 0.f;
                    }
                }

              /* insert it to the tracklist at its original pos */
              auto added_track = TRACKLIST->insert_track (
                std::move (track), track->pos_, *AUDIO_ENGINE, false, false);

              /* if group track, readd all children */
              if (added_track->can_be_group_target ())
                {
                  auto &own_group_target_track =
                    dynamic_cast<GroupTargetTrack &> (*own_track);
                  dynamic_cast<GroupTargetTrack *> (added_track)
                    ->add_children (
                      own_group_target_track.children_, false, false, false);
                }

              if (added_track->type_ == Track::Type::Instrument)
                {
#if 0
                  auto &own_instrument_track =
                    dynamic_cast<InstrumentTrack &> (*own_track);
                  auto added_instrument_track =
                    dynamic_cast<InstrumentTrack *> (added_track);
                  if (own_instrument_track.get_channel ()->instrument_->visible_)
                    {
                      // EVENTS_PUSH (
                      //   EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                      //   added_instrument_track->get_channel ()
                      //     ->instrument_.get ());
                    }
#endif
                }
            }

          for (int i = 0; i < num_tracks; i++)
            {
              auto &own_track = tls_before_->tracks_[i];

              /* get the project track */
              auto &_prj_track = TRACKLIST->tracks_[own_track->pos_];
              std::visit (
                [&] (auto &&prj_track) {
                  using TrackT = base_type<decltype (prj_track)>;
                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {

                      auto &own_channel_track =
                        dynamic_cast<TrackT &> (*own_track);

                      /* reconnect output */
                      if (out_track_hashes_[i] != 0)
                        {
                          auto out_track =
                            prj_track->get_channel ()->get_output_track ();
                          out_track->remove_child (
                            prj_track->get_name_hash (), true, false, false);
                          out_track = std::visit (
                            [&] (auto &&t) {
                              return dynamic_cast<GroupTargetTrack *> (t);
                            },
                            TRACKLIST
                              ->find_track_by_name_hash (out_track_hashes_[i])
                              .value ());
                          out_track->add_child (
                            prj_track->get_name_hash (), true, false, false);
                        }

                      /* reconnect any sends sent from the track */
                      for (size_t j = 0; j < STRIP_SIZE; j++)
                        {
                          auto &clone_send =
                            own_channel_track.get_channel ()->sends_[j];
                          auto &send = prj_track->get_channel ()->sends_[j];
                          send->copy_values_from (*clone_send);
                        }

                      /* reconnect any custom connections */
                      std::vector<Port *> ports;
                      own_channel_track.append_ports (ports, true);
                      for (auto port : ports)
                        {
                          Port * prj_port =
                            Port::find_from_identifier (*port->id_);
                          prj_port->restore_from_non_project (*port);
                        }
                    }
                },
                _prj_track);
            }

          /* re-connect any source sends */
          for (auto &clone_send : src_sends_)
            {

              /* get the original send and connect it */
              auto prj_send = clone_send->find_in_project ();
              prj_send->copy_values_from (*clone_send);
            }

          /* reset foldable track sizes */
          reset_foldable_track_sizes ();
        } /* if delete undo */

      /* EVENTS_PUSH (EventType::ET_TRACKS_ADDED, nullptr); */
      /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
    }
  /* else if deleting tracks (delete do or create
   * undo) */
  else
    {
      /* if create undo */
      if (create)
        {
          for (int i = num_tracks_ - 1; i >= 0; i--)
            {
              auto tr = TRACKLIST->get_track (track_pos_ + i);
              std::visit (
                [&] (auto &&track) {
                  z_return_if_fail (track);
                  z_return_if_fail (
                    TRACKLIST->get_track_pos (*track) == track->pos_);

                  TRACKLIST->remove_track (*track, true, true, false, false);
                },
                tr);
            }

          /* reenable given track, if any (eg when bouncing) */
          if (ival_after_ > -1)
            {
              z_return_if_fail (
                ival_after_ < static_cast<int> (TRACKLIST->tracks_.size ()));
              auto &tr_to_enable = TRACKLIST->tracks_.at (ival_after_);
              std::visit (
                [&] (auto &&tr) { tr->set_enabled (true, false, false, true); },
                tr_to_enable);
            }
        }
      /* else if delete do */
      else
        {
          /* remove any sends pointing to any track */
          for (auto &clone_send : src_sends_)
            {
              /* get the original send and disconnect it */
              ChannelSend * send = clone_send->find_in_project ();
              send->disconnect (false);
            }

          for (int i = tls_before_->tracks_.size () - 1; i >= 0; i--)
            {
              auto &own_track = tls_before_->tracks_[i];

              /* get track from pos */
              auto &_prj_track = TRACKLIST->tracks_[own_track->pos_];
              std::visit (
                [&] (auto &&prj_track) {
                  z_return_if_fail (prj_track);

                  /* remember any custom connections */
                  std::vector<Port *> prj_ports;
                  prj_track->append_ports (prj_ports, true);
                  std::vector<Port *> clone_ports;
                  own_track->append_ports (clone_ports, true);
                  for (auto prj_port : prj_ports)
                    {
                      Port * clone_port = nullptr;
                      for (auto cur_clone_port : clone_ports)
                        {
                          if (cur_clone_port->id_ == prj_port->id_)
                            {
                              clone_port = cur_clone_port;
                              break;
                            }
                        }
                      z_return_if_fail (clone_port);

                      clone_port->copy_metadata_from_project (*prj_port);
                    }

                  /* if group track, remove all children */
                  if (prj_track->can_be_group_target ())
                    {
                      dynamic_cast<GroupTargetTrack &> (*prj_track)
                        .remove_all_children (true, false, false);
                    }

                  /* remove it */
                  TRACKLIST->remove_track (*prj_track, true, true, false, false);
                },
                _prj_track);
            }
        }

      /* EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr); */
      /* EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr); */
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  TRACKLIST->set_caches (ALL_CACHE_TYPES);
  TRACKLIST->validate ();

  ROUTER->recalc_graph (false);

  TRACKLIST->validate ();
  MIXER_SELECTIONS->validate ();
}

void
TracklistSelectionsAction::
  do_or_undo_move_or_copy (bool _do, bool copy, bool inside)
{
  bool move = !copy;
  bool pin = tracklist_selections_action_type_ == Type::Pin;
  bool unpin = tracklist_selections_action_type_ == Type::Unpin;

  /* if moving, this will be set back */
  auto prev_clip_editor_region_opt = CLIP_EDITOR->get_region ();

  if (_do)
    {
      FoldableTrack * foldable_tr = nullptr;
      num_fold_change_tracks_ = 0;
      if (inside)
        {
          auto tr = TRACKLIST->get_track (track_pos_);
          std::visit (
            [&] (auto &&track) {
              using TrackT = base_type<decltype (track)>;
              if constexpr (std::derived_from<TrackT, FoldableTrack>)
                {
                  foldable_tr = track;
                }
            },
            tr);
          z_return_if_fail (foldable_tr);
        }

      Track * prev_track = nullptr;
      if (move)
        {
          /* calculate how many tracks are not already in the folder */
          for (const auto &track : tls_before_->tracks_)
            {
              auto prj_track = TRACKLIST->find_track_by_name (track->name_);
              z_return_if_fail (prj_track);

              std::visit (
                [&] (auto &&prj_tr) {
                  if (inside)
                    {
                      std::vector<FoldableTrack *> parents;
                      prj_tr->add_folder_parents (parents, false);
                      if (
                        std::find (parents.begin (), parents.end (), foldable_tr)
                        == parents.end ())
                        num_fold_change_tracks_++;
                    }
                },
                *prj_track);
            }

          for (size_t i = 0; i < tls_before_->tracks_.size (); i++)
            {
              auto _prj_track =
                TRACKLIST->find_track_by_name (tls_before_->tracks_[i]->name_);
              z_return_if_fail (_prj_track);

              std::visit (
                [&] (auto &&prj_track) {
                  int target_pos = -1;
                  /* if not first track to be moved */
                  if (prev_track)
                    {
                      /* move to last track's index + 1 */
                      target_pos = prev_track->pos_ + 1;
                    }
                  /* else if first track to be moved */
                  else
                    {
                      /* move to given pos */
                      target_pos = track_pos_;

                      /* if moving inside, skip folder track */
                      if (inside)
                        target_pos++;
                    }

                  /* save index */
                  auto &own_track = tls_before_->tracks_[i];
                  own_track->pos_ = prj_track->pos_;

                  std::vector<FoldableTrack *> parents;
                  prj_track->add_folder_parents (parents, false);

                  TRACKLIST->move_track (
                    *prj_track, target_pos, true, false, false);
                  prev_track = prj_track;

                  /* adjust parent sizes */
                  for (auto parent : parents)
                    {
                      /* if new pos is outside parent */
                      if (
                        prj_track->pos_ < parent->pos_
                        || prj_track->pos_ >= parent->pos_ + parent->size_)
                        {
                          z_debug (
                            "new pos of {} ({}) is outside parent {}: parent--",
                            prj_track->name_, prj_track->pos_, parent->name_);
                          --parent->size_;
                        }

                      /* if foldable track is child of parent (size will be
                       * readded later) */
                      if (inside && parent->is_child (*foldable_tr))
                        {
                          z_debug (
                            "foldable track {} is child of parent {}: parent--",
                            foldable_tr->name_, parent->name_);
                          parent->size_--;
                        }
                    }

                  if (i == 0)
                    TRACKLIST_SELECTIONS->select_single (*prj_track, false);
                  else
                    TRACKLIST_SELECTIONS->add_track (*prj_track, false);

                  TRACKLIST->print_tracks ();
                },
                *_prj_track);
            }

          /* EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr); */
        }
      else if (copy)
        {
          unsigned int num_tracks = tls_before_->tracks_.size ();

          if (inside)
            {
              num_fold_change_tracks_ = tls_before_->tracks_.size ();
            }

          /* get outputs & sends */
          std::vector<GroupTargetTrack *> outputs_in_prj (num_tracks);
          std::vector<std::array<std::unique_ptr<ChannelSend>, STRIP_SIZE>>
            sends (num_tracks);
          std::vector<
            std::array<PortConnectionsManager::ConnectionsVector, STRIP_SIZE>>
            send_conns (num_tracks);
          for (size_t i = 0; i < num_tracks; i++)
            {
              auto &own_track = tls_before_->tracks_[i];
              if (own_track->has_channel ())
                {
                  auto own_channel_track =
                    dynamic_cast<ChannelTrack *> (own_track.get ());
                  outputs_in_prj.push_back (
                    own_channel_track->get_channel ()->get_output_track ());

                  for (size_t j = 0; j < STRIP_SIZE; ++j)
                    {
                      auto &send =
                        own_channel_track->get_channel ()->sends_.at (j);
                      sends.at (i).at (j) = send->clone_unique ();

                      send->append_connection (
                        port_connections_before_.get (),
                        send_conns.at (i).at (j));
                    }
                }
              else
                {
                  outputs_in_prj.push_back (nullptr);
                }
            }

          TRACKLIST_SELECTIONS->clear (false);

          /* create new tracks routed to master */
          std::vector<Track *> new_tracks;
          for (size_t i = 0; i < num_tracks; i++)
            {
              auto &own_track = tls_before_->tracks_[i];

              std::visit (
                [&] (const auto &&own_track_ptr) {
                  using TrackT = base_type<decltype (own_track_ptr)>;

                  /* create a new clone to use in the project */
                  auto track = own_track_ptr->clone_unique ();
                  z_return_if_fail (track);

                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      /* remove output */
                      track->get_channel ()->has_output_ = false;
                      track->get_channel ()->output_name_hash_ = 0;

                      /* remove sends */
                      for (auto &send : track->get_channel ()->sends_)
                        {
                          send->enabled_->control_ = 0.f;
                        }
                    }

                  /* remove children */
                  if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
                    {
                      track->children_.clear ();
                    }

                  int target_pos = track_pos_ + i;
                  if (inside)
                    target_pos++;

                  /* add to tracklist at given pos */
                  auto added_track = TRACKLIST->insert_track (
                    std::move (track), target_pos, *AUDIO_ENGINE, false, false);

                  /* select it */
                  added_track->select (true, false, false);
                  new_tracks.push_back (added_track);
                },
                convert_to_variant<TrackPtrVariant> (own_track.get ()));
            }

          /* reroute new tracks to correct outputs & sends */
          for (size_t i = 0; i < num_tracks; i++)
            {
              auto track = new_tracks[i];
              if (outputs_in_prj[i])
                {
                  auto channel_track = dynamic_cast<ChannelTrack *> (track);
                  auto out_track =
                    channel_track->get_channel ()->get_output_track ();
                  out_track->remove_child (
                    track->get_name_hash (), true, false, false);
                  outputs_in_prj[i]->add_child (
                    track->get_name_hash (), true, false, false);
                }

              if (track->has_channel ())
                {
                  auto channel_track = dynamic_cast<ChannelTrack *> (track);
                  for (size_t j = 0; j < STRIP_SIZE; j++)
                    {
                      const auto &own_send = sends.at (i).at (j);
                      const auto &own_conns = send_conns.at (i).at (j);
                      const auto &track_send =
                        channel_track->get_channel ()->sends_.at (j);
                      track_send->copy_values_from (*own_send);
                      if (
                        !own_conns.empty ()
                        && track->out_signal_type_ == PortType::Audio)
                        {
                          PORT_CONNECTIONS_MGR->ensure_connect (
                            *track_send->stereo_out_->get_l ().id_,
                            *own_conns.at (0)->dest_id_, 1.f, true, true);
                          PORT_CONNECTIONS_MGR->ensure_connect (
                            *track_send->stereo_out_->get_r ().id_,
                            *own_conns.at (1)->dest_id_, 1.f, true, true);
                        }
                      else if (
                        !own_conns.empty ()
                        && track->out_signal_type_ == PortType::Event)
                        {
                          PORT_CONNECTIONS_MGR->ensure_connect (
                            *track_send->midi_out_->id_,
                            *own_conns.front ()->dest_id_, 1.f, true, true);
                        }
                    }

                } /* endif track has channel */

            } /* endforeach track */

          /* EVENTS_PUSH (EventType::ET_TRACK_ADDED, nullptr); */
          /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */

        } /* endif copy */

      if (inside)
        {
          /* update foldable track sizes (incl. parents) */
          foldable_tr->add_to_size (num_fold_change_tracks_);
        }
    }
  /* if undoing */
  else
    {
      if (move)
        {
          for (int i = tls_before_->tracks_.size () - 1; i >= 0; i--)
            {
              auto &own_track = tls_before_->tracks_[i];

              auto _prj_track = TRACKLIST->find_track_by_name (own_track->name_);
              z_return_if_fail (_prj_track);
              std::visit (
                [&] (auto &&prj_track) {
                  int target_pos = own_track->pos_;
                  TRACKLIST->move_track (
                    *prj_track, target_pos, false, false, false);

                  if (i == 0)
                    {
                      TRACKLIST_SELECTIONS->select_single (*prj_track, false);
                    }
                  else
                    {
                      TRACKLIST_SELECTIONS->add_track (*prj_track, false);
                    }
                },
                *_prj_track);
            }

          /* EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr); */
        }
      else if (copy)
        {
          for (int i = tls_before_->tracks_.size () - 1; i >= 0; i--)
            {
              /* get the track from the inserted pos */
              int target_pos = track_pos_ + i;
              if (inside)
                target_pos++;

              auto &_prj_track = TRACKLIST->tracks_[target_pos];
              std::visit (
                [&] (auto &&prj_track) {
                  z_return_if_fail (prj_track);

                  /* remove it */
                  TRACKLIST->remove_track (*prj_track, true, true, false, false);
                },
                _prj_track);
            }
          /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
          /* EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr); */
        }

      if (inside)
        {
          /* update foldable track sizes (incl. parents) */
          auto _foldable_tr = TRACKLIST->get_track (track_pos_);
          std::visit (
            [&] (auto &&foldable_tr) {
              using TrackT = base_type<decltype (foldable_tr)>;
              if constexpr (std::derived_from<TrackT, FoldableTrack>)
                {
                  foldable_tr->add_to_size (-num_fold_change_tracks_);
                }
              else
                {
                  z_return_if_reached ();
                }
            },
            _foldable_tr);
        }

      /* reset foldable track sizes */
      reset_foldable_track_sizes ();
    }

  if ((pin && _do) || (unpin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff_ += tls_before_->tracks_.size ();
    }
  else if ((unpin && _do) || (pin && !_do))
    {
      TRACKLIST->pinned_tracks_cutoff_ -= tls_before_->tracks_.size ();
    }

  if (move)
    {
      CLIP_EDITOR->set_region (prev_clip_editor_region_opt, false);
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  TRACKLIST->set_caches (ALL_CACHE_TYPES);
  TRACKLIST->validate ();

  ROUTER->recalc_graph (false);
}

void
TracklistSelectionsAction::do_or_undo_edit (bool _do)
{
  if (_do && already_edited_)
    {
      already_edited_ = false;
      return;
    }

  bool need_recalc_graph = false;
  bool need_tracklist_cache_update = false;

  for (int i = 0; i < num_tracks_; i++)
    {
      auto &_track = TRACKLIST->tracks_[track_positions_before_[i]];
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          z_return_if_fail (track);

          switch (edit_type_)
            {
            case EditType::Solo:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  bool soloed = track->get_soloed ();
                  track->set_soloed (
                    _do ? ival_after_ : ival_before_[i], false, false, false);

                  ival_before_[i] = soloed;
                }
              break;
            case EditType::SoloLane:
              {
                if constexpr (std::derived_from<TrackT, LanedTrack>)
                  {
                    auto lane_var = track->lanes_.at (lane_pos_);
                    std::visit (
                      [&] (auto &&lane) {
                        bool soloed = lane->get_soloed ();
                        lane->set_soloed (
                          _do ? ival_after_ : ival_before_[i], false, false);
                        ival_before_[i] = soloed;
                      },
                      lane_var);
                  }
              }
              break;
            case EditType::Mute:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  bool muted = track->get_muted ();
                  track->set_muted (
                    _do ? ival_after_ : ival_before_[i], false, false, false);

                  ival_before_[i] = muted;
                }
              break;
            case EditType::MuteLane:
              {
                if constexpr (std::derived_from<TrackT, LanedTrack>)
                  {
                    auto lane_var = track->lanes_.at (lane_pos_);
                    std::visit (
                      [&] (auto &&lane) {
                        bool muted = lane->get_muted ();
                        lane->set_muted (
                          _do ? ival_after_ : ival_before_[i], false, false);

                        ival_before_[i] = muted;
                      },
                      lane_var);
                  }
              }
              break;
            case EditType::Listen:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  bool listened = track->get_listened ();
                  track->set_listened (
                    _do ? ival_after_ : ival_before_[i], false, false, false);

                  ival_before_[i] = listened;
                }
              break;
            case EditType::Enable:
              {
                bool enabled = track->is_enabled ();
                track->set_enabled (
                  _do ? ival_after_ : ival_before_[i], false, false, false);

                ival_before_[i] = enabled;
              }
              break;
            case EditType::Fold:
              {
                if constexpr (std::derived_from<TrackT, FoldableTrack>)
                  {
                    bool folded = track->folded_;
                    track->set_folded (
                      _do ? ival_after_ : ival_before_[i], false, false, true);

                    ival_before_[i] = folded;
                  }
              }
              break;
            case EditType::Volume:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  z_return_if_fail (track);
                  track->channel_->fader_->set_amp (
                    _do ? val_after_ : val_before_);
                }
              break;
            case EditType::Pan:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  z_return_if_fail (track);
                  track->get_channel ()->set_balance_control (
                    _do ? val_after_ : val_before_);
                }
              break;
            case EditType::MidiFaderMode:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  z_return_if_fail (track);
                  if (_do)
                    {
                      ival_before_[i] =
                        static_cast<int> (track->channel_->fader_->midi_mode_);
                      track->channel_->fader_->set_midi_mode (
                        static_cast<Fader::MidiFaderMode> (ival_after_), false,
                        true);
                    }
                  else
                    {
                      track->channel_->fader_->set_midi_mode (
                        static_cast<Fader::MidiFaderMode> (ival_before_[i]),
                        false, true);
                    }
                }
              break;
            case EditType::DirectOut:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  {
                    z_return_if_fail (track);

                    int cur_direct_out_pos = -1;
                    if (track->get_channel ()->has_output_)
                      {
                        auto cur_direct_out_track =
                          track->get_channel ()->get_output_track ();
                        cur_direct_out_pos = cur_direct_out_track->pos_;
                      }

                    /* disconnect from the current track */
                    if (track->get_channel ()->has_output_)
                      {
                        auto target_track = std::visit (
                          [&] (auto &&t) {
                            return dynamic_cast<GroupTargetTrack *> (t);
                          },
                          TRACKLIST
                            ->find_track_by_name_hash (
                              track->get_channel ()->output_name_hash_)
                            .value ());
                        target_track->remove_child (
                          track->get_name_hash (), true, false, true);
                      }

                    int target_pos = _do ? ival_after_ : ival_before_[i];

                    /* reconnect to the new track */
                    if (target_pos != -1)
                      {
                        z_return_if_fail_cmp (
                          target_pos, !=, track->channel_->track_->pos_);
                        auto _group_target_track =
                          TRACKLIST->get_track (target_pos);
                        std::visit (
                          [&] (auto &&group_target_track) {
                            using GroupTrackT =
                              base_type<decltype (group_target_track)>;
                            if constexpr (
                              std::derived_from<GroupTrackT, GroupTargetTrack>)
                              {
                                group_target_track->add_child (
                                  track->get_name_hash (), true, false, true);
                              }
                            else
                              {
                                z_return_if_reached ();
                              }
                          },
                          _group_target_track);
                      }

                    /* remember previous pos */
                    ival_before_[i] = cur_direct_out_pos;

                    need_recalc_graph = true;
                  }
                }
              break;
            case EditType::Rename:
              {
                const auto &cur_name = track->get_name ();
                track->set_name (*TRACKLIST, new_txt_, false);

                /* remember the new name */
                new_txt_ = cur_name;

                need_tracklist_cache_update = true;
                need_recalc_graph = true;
              }
              break;
            case EditType::RenameLane:
              {
                if constexpr (std::derived_from<TrackT, LanedTrack>)
                  {
                    auto lane_var = track->lanes_.at (lane_pos_);
                    std::visit (
                      [&] (auto &&lane) {
                        const auto &cur_name = lane->name_;
                        lane->rename (new_txt_, false);

                        /* remember the new name */
                        new_txt_ = cur_name;
                      },
                      lane_var);
                  }
              }
              break;
            case EditType::Color:
              {
                auto cur_color = track->color_;
                track->set_color (
                  _do ? new_color_ : colors_before_[i], false, true);

                /* remember color */
                colors_before_[i] = cur_color;
              }
              break;
            case EditType::Icon:
              {
                auto cur_icon = track->icon_name_;
                track->set_icon (new_txt_, false, true);

                new_txt_ = cur_icon;
              }
              break;
            case EditType::Comment:
              {
                auto cur_comment = track->comment_;
                track->set_comment (new_txt_, false);

                new_txt_ = cur_comment;
              }
              break;
            }

          /* EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track.get ()); */
        },
        _track);
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  if (need_tracklist_cache_update)
    {
      TRACKLIST->set_caches (ALL_CACHE_TYPES);
    }

  if (need_recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }
}

void
TracklistSelectionsAction::do_or_undo (bool _do)
{
  switch (tracklist_selections_action_type_)
    {
    case Type::Copy:
    case Type::CopyInside:
      do_or_undo_move_or_copy (
        _do, true, tracklist_selections_action_type_ == Type::CopyInside);
      break;
    case Type::Create:
      do_or_undo_create_or_delete (_do, true);
      break;
    case Type::Delete:
      do_or_undo_create_or_delete (_do, false);
      break;
    case Type::Edit:
      do_or_undo_edit (_do);
      break;
    case Type::Move:
    case Type::MoveInside:
    case Type::Pin:
    case Type::Unpin:
      do_or_undo_move_or_copy (
        _do, false, tracklist_selections_action_type_ == Type::MoveInside);
      break;
    default:
      z_return_if_reached ();
      break;
    }
}

void
TracklistSelectionsAction::perform_impl ()
{
  do_or_undo (true);
}

void
TracklistSelectionsAction::undo_impl ()
{
  do_or_undo (false);
}

QString
TracklistSelectionsAction::to_string () const
{
  switch (tracklist_selections_action_type_)
    {
    case Type::Copy:
    case Type::CopyInside:
      if (tls_before_->tracks_.size () == 1)
        {
          if (tracklist_selections_action_type_ == Type::CopyInside)
            return QObject::tr ("Copy Track inside");
          else
            return QObject::tr ("Copy Track");
        }
      else
        {
          if (tracklist_selections_action_type_ == Type::CopyInside)
            return format_qstr (
              QObject::tr ("Copy {} Tracks inside"),
              tls_before_->tracks_.size ());
          else
            return format_qstr (
              QObject::tr ("Copy {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Create:
      {
        auto type = Track_Type_to_string (track_type_);
        if (num_tracks_ == 1)
          {
            return format_qstr (QObject::tr ("Create {} Track"), type);
          }
        else
          {
            return format_qstr (
              QObject::tr ("Create {} {} Tracks"), num_tracks_, type);
          }
      }
    case Type::Delete:
      if (tls_before_->tracks_.size () == 1)
        {
          return QObject::tr ("Delete Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Delete {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Edit:
      if (
        (tls_before_ && tls_before_->tracks_.size () == 1) || (num_tracks_ == 1))
        {
          switch (edit_type_)
            {
            case EditType::Solo:
              if (ival_after_)
                return QObject::tr ("Solo Track");
              else
                return QObject::tr ("Unsolo Track");
            case EditType::SoloLane:
              if (ival_after_)
                return QObject::tr ("Solo Lane");
              else
                return QObject::tr ("Unsolo Lane");
            case EditType::Mute:
              if (ival_after_)
                return QObject::tr ("Mute Track");
              else
                return QObject::tr ("Unmute Track");
            case EditType::MuteLane:
              if (ival_after_)
                return QObject::tr ("Mute Lane");
              else
                return QObject::tr ("Unmute Lane");
            case EditType::Listen:
              if (ival_after_)
                return QObject::tr ("Listen Track");
              else
                return QObject::tr ("Unlisten Track");
            case EditType::Enable:
              if (ival_after_)
                return QObject::tr ("Enable Track");
              else
                return QObject::tr ("Disable Track");
            case EditType::Fold:
              if (ival_after_)
                return QObject::tr ("Fold Track");
              else
                return QObject::tr ("Unfold Track");
            case EditType::Volume:
              return format_qstr (
                QObject::tr ("Change Fader from {:.1f} to {:.1f}"), val_before_,
                val_after_);
            case EditType::Pan:
              return format_qstr (
                QObject::tr ("Change Pan from {:.1f} to {:.1f}"), val_before_,
                val_after_);
            case EditType::DirectOut:
              return QObject::tr ("Change direct out");
            case EditType::Rename:
              return QObject::tr ("Rename track");
            case EditType::RenameLane:
              return QObject::tr ("Rename lane");
            case EditType::Color:
              return QObject::tr ("Change color");
            case EditType::Icon:
              return QObject::tr ("Change icon");
            case EditType::Comment:
              return QObject::tr ("Change comment");
            case EditType::MidiFaderMode:
              return QObject::tr ("Change MIDI fader mode");
            }
        }
      else
        {
          switch (edit_type_)
            {
            case EditType::Solo:
              if (ival_after_)
                return format_qstr (QObject::tr ("Solo {} Tracks"), num_tracks_);
              else
                return format_qstr (
                  QObject::tr ("Unsolo {} Tracks"), num_tracks_);
            case EditType::Mute:
              if (ival_after_)
                return format_qstr (QObject::tr ("Mute {} Tracks"), num_tracks_);
              else
                return format_qstr (
                  QObject::tr ("Unmute {} Tracks"), num_tracks_);
            case EditType::Listen:
              if (ival_after_)
                return format_qstr (
                  QObject::tr ("Listen {} Tracks"), num_tracks_);
              else
                return format_qstr (
                  QObject::tr ("Unlisten {} Tracks"), num_tracks_);
            case EditType::Enable:
              if (ival_after_)
                return format_qstr (
                  QObject::tr ("Enable {} Tracks"), num_tracks_);
              else
                return format_qstr (
                  QObject::tr ("Disable {} Tracks"), num_tracks_);
            case EditType::Fold:
              if (ival_after_)
                return format_qstr (QObject::tr ("Fold {} Tracks"), num_tracks_);
              else
                return format_qstr (
                  QObject::tr ("Unfold {} Tracks"), num_tracks_);
            case EditType::Color:
              return QObject::tr ("Change color");
            case EditType::MidiFaderMode:
              return QObject::tr ("Change MIDI fader mode");
            case EditType::DirectOut:
              return QObject::tr ("Change direct out");
            case EditType::SoloLane:
              return QObject::tr ("Solo lanes");
            case EditType::MuteLane:
              return QObject::tr ("Mute lanes");
            default:
              return QObject::tr ("Edit tracks");
            }
        }
    case Type::Move:
    case Type::MoveInside:
      if (tls_before_->tracks_.size () == 1)
        {
          if (tracklist_selections_action_type_ == Type::MoveInside)
            {
              return QObject::tr ("Move Track inside");
            }
          else
            {
              return QObject::tr ("Move Track");
            }
        }
      else
        {
          if (tracklist_selections_action_type_ == Type::MoveInside)
            {
              return format_qstr (
                QObject::tr ("Move {} Tracks inside"),
                tls_before_->tracks_.size ());
            }
          else
            {
              return format_qstr (
                QObject::tr ("Move {} Tracks"), tls_before_->tracks_.size ());
            }
        }
    case Type::Pin:
      if (tls_before_->tracks_.size () == 1)
        {
          return QObject::tr ("Pin Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Pin {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Unpin:
      if (tls_before_->tracks_.size () == 1)
        {
          return QObject::tr ("Unpin Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Unpin {} Tracks"), tls_before_->tracks_.size ());
        }
    }

  z_return_val_if_reached ({});
}