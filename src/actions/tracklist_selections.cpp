// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/audio_bus_track.h"
#include "dsp/audio_group_track.h"
#include "dsp/audio_region.h"
#include "dsp/audio_track.h"
#include "dsp/foldable_track.h"
#include "dsp/folder_track.h"
#include "dsp/group_target_track.h"
#include "dsp/instrument_track.h"
#include "dsp/midi_bus_track.h"
#include "dsp/midi_group_track.h"
#include "dsp/midi_track.h"
#include "dsp/router.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "io/file_descriptor.h"
#include "io/midi_file.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/io.h"
#include "utils/types.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <glibmm.h>

void
TracklistSelectionsAction::init_after_cloning (
  const TracklistSelectionsAction &other)
{
  UndoableAction::copy_members_from (other);
  tracklist_selections_action_type_ = other.tracklist_selections_action_type_;
  track_type_ = other.track_type_;
  if (other.pl_setting_)
    pl_setting_ = std::make_unique<PluginSetting> (*other.pl_setting_);
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
      auto prj_tr = Track::find_by_name<FoldableTrack> (own_tr->name_);
      prj_tr->size_ = own_tr->size_;
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
      pl_setting_ (
        pl_setting ? std::make_unique<PluginSetting> (*pl_setting) : nullptr),
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
      throw ZrythmException (
        _ ("Cannot copy tracks: selection contains an uncopyable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::Delete
    && tls_before->contains_undeletable_track ())
    {
      throw ZrythmException (
        _ ("Cannot delete tracks: selection contains an undeletable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::MoveInside
    || tracklist_selections_action_type_ == Type::CopyInside)
    {
      auto &foldable_tr = TRACKLIST->tracks_[track_pos];
      assert (foldable_tr->is_foldable ());
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
              std::string contents =
                Glib::file_get_contents (file_descr->abs_path_);
              base64_midi_ = Glib::Base64::encode (contents);
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

      file_basename_ = Glib::path_get_basename (file_descr->abs_path_);
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
          throw ZrythmException (_ ("No tracks selected"));
        }

      if (need_full_selections)
        {
          tls_before_ = tls_before->clone_unique ();
          tls_before_->sort ();
          foldable_tls_before_ = std::make_unique<TracklistSelections> ();
          for (auto &tr : TRACKLIST->tracks_)
            {
              if (tr->is_foldable ())
                foldable_tls_before_->add_track (
                  clone_unique_with_variant<TrackVariant> (tr.get ()));
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
      auto label = format_str (_ ("{} Track"), track_type_str);
      auto track = Track::create_track (track_type_, label, pos);
      TRACKLIST->insert_track (std::move (track), pos, false, false);
    }
  else /* else if track is not empty */
    {
      std::unique_ptr<Plugin> pl;
      std::unique_ptr<Track>  track;
      Plugin *                added_pl = nullptr;

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
          auto &setting = pl_setting_;
          auto &descr = setting->descr_;

          track = Track::create_track (track_type_, descr.name_, pos);

          pl = setting->create_plugin (track->get_name_hash ());
          pl->instantiate ();
          pl->activate ();
        }

      auto added_track =
        TRACKLIST->insert_track (std::move (track), pos, false, false);

      if (pl && added_track->has_channel ())
        {
          bool is_instrument = added_track->type_ == Track::Type::Instrument;
          auto channel_track = dynamic_cast<ChannelTrack *> (added_track);
          added_pl = channel_track->channel_->add_plugin (
            std::move (pl),
            is_instrument ? PluginSlotType::Instrument : PluginSlotType::Insert,
            is_instrument ? -1 : pl->id_.slot_, true, false, true, false, false);
        }

      Position start_pos = have_pos_ ? pos_ : Position ();
      if (track_type_ == Track::Type::Audio)
        {
          /* create an audio region & add to track*/
          added_track->add_region (
            std::make_shared<AudioRegion> (
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
          std::string dir_str = io_create_tmp_dir ("zrythm_tmp_midi_XXXXXX");
          auto        full_path = Glib::build_filename (dir_str, "data.MID");
          auto        data = Glib::Base64::decode (base64_midi_);
          io_write_file_atomic (full_path, data);

          /* create a MIDI region from the MIDI file & add to track */
          added_track->add_region (
            std::make_shared<MidiRegion> (
              pos_, full_path, added_track->get_name_hash (), 0, 0, idx),
            nullptr, 0, true, false);

          /* remove temporary data */
          io_remove (full_path);
          io_rmdir (dir_str, false);
        }

      if (
        added_pl && ZRYTHM_HAVE_UI
        && g_settings_get_boolean (S_P_PLUGINS_UIS, "open-on-instantiate"))
        {
          added_pl->visible_ = true;
          EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, added_pl);
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
              auto &tr_to_disable = TRACKLIST->tracks_[ival_after_];
              z_return_if_fail (tr_to_disable);
              tr_to_disable->set_enabled (false, false, false, true);
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
                std::move (track), track->pos_, false, false);

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
                  auto &own_instrument_track =
                    dynamic_cast<InstrumentTrack &> (*own_track);
                  auto added_instrument_track =
                    dynamic_cast<InstrumentTrack *> (added_track);
                  if (own_instrument_track.get_channel ()->instrument_->visible_)
                    {
                      EVENTS_PUSH (
                        EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                        added_instrument_track->get_channel ()
                          ->instrument_.get ());
                    }
                }
            }

          for (int i = 0; i < num_tracks; i++)
            {
              auto &own_track = tls_before_->tracks_[i];

              /* get the project track */
              auto &prj_track = TRACKLIST->tracks_[own_track->pos_];
              if (!prj_track->has_channel ())
                continue;

              auto &own_channel_track =
                dynamic_cast<ChannelTrack &> (*own_track);
              auto &prj_channel_track =
                dynamic_cast<ChannelTrack &> (*prj_track);

              /* reconnect output */
              if (out_track_hashes_[i] != 0)
                {
                  auto out_track =
                    prj_channel_track.get_channel ()->get_output_track ();
                  out_track->remove_child (
                    prj_track->get_name_hash (), true, false, false);
                  out_track = dynamic_cast<GroupTargetTrack *> (
                    TRACKLIST->find_track_by_name_hash (out_track_hashes_[i]));
                  out_track->add_child (
                    prj_track->get_name_hash (), true, false, false);
                }

              /* reconnect any sends sent from the track */
              for (size_t j = 0; j < STRIP_SIZE; j++)
                {
                  auto &clone_send = own_channel_track.get_channel ()->sends_[j];
                  auto &send = prj_channel_track.get_channel ()->sends_[j];
                  send->copy_values_from (*clone_send);
                }

              /* reconnect any custom connections */
              std::vector<Port *> ports;
              own_channel_track.append_ports (ports, true);
              for (auto port : ports)
                {
                  Port * prj_port = Port::find_from_identifier (port->id_);
                  prj_port->restore_from_non_project (*port);
                }
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

      EVENTS_PUSH (EventType::ET_TRACKS_ADDED, nullptr);
      EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
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
              auto &track = TRACKLIST->tracks_.at (track_pos_ + i);
              z_return_if_fail (track);
              z_return_if_fail (
                TRACKLIST->get_track_pos (*track) == track->pos_);

              TRACKLIST->remove_track (*track, true, true, false, false);
            }

          /* reenable given track, if any (eg when bouncing) */
          if (ival_after_ > -1)
            {
              z_return_if_fail (
                ival_after_ < static_cast<int> (TRACKLIST->tracks_.size ()));
              auto &tr_to_enable = TRACKLIST->tracks_[ival_after_];
              z_return_if_fail (tr_to_enable);
              tr_to_enable->set_enabled (true, false, false, true);
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
              auto &prj_track = TRACKLIST->tracks_[own_track->pos_];
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
            }
        }

      EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr);
      EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr);
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
  Region * prev_clip_editor_region = CLIP_EDITOR->get_region ();

  if (_do)
    {
      FoldableTrack * foldable_tr = nullptr;
      num_fold_change_tracks_ = 0;
      if (inside)
        {
          foldable_tr = dynamic_cast<FoldableTrack *> (
            TRACKLIST->tracks_[track_pos_].get ());
          z_return_if_fail (foldable_tr);
        }

      Track * prev_track = nullptr;
      if (move)
        {
          /* calculate how many tracks are not already in the folder */
          for (size_t i = 0; i < tls_before_->tracks_.size (); i++)
            {
              Track * prj_track =
                TRACKLIST->find_track_by_name (tls_before_->tracks_[i]->name_);
              z_return_if_fail (prj_track);
              if (inside)
                {
                  std::vector<FoldableTrack *> parents;
                  prj_track->add_folder_parents (parents, false);
                  if (
                    std::find (parents.begin (), parents.end (), foldable_tr)
                    == parents.end ())
                    num_fold_change_tracks_++;
                }
            }

          for (size_t i = 0; i < tls_before_->tracks_.size (); i++)
            {
              Track * prj_track =
                TRACKLIST->find_track_by_name (tls_before_->tracks_[i]->name_);
              z_return_if_fail (prj_track);

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

              TRACKLIST->move_track (*prj_track, target_pos, true, false, false);
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

                  /* if foldable track is child of parent (size will be readded
                   * later) */
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
            }

          EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr);
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
          std::vector<std::array<std::vector<PortConnection>, STRIP_SIZE>>
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

                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      auto &send = own_channel_track->get_channel ()->sends_[j];
                      sends[i][j] = send->clone_unique ();

                      send->append_connection (
                        port_connections_before_.get (), send_conns[i][j]);
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

              /* create a new clone to use in the project */
              auto track =
                clone_unique_with_variant<TrackVariant> (own_track.get ());
              z_return_if_fail (track);

              if (track->has_channel ())
                {
                  auto channel_track =
                    dynamic_cast<ChannelTrack *> (track.get ());
                  /* remove output */
                  channel_track->get_channel ()->has_output_ = false;
                  channel_track->get_channel ()->output_name_hash_ = 0;

                  /* remove sends */
                  for (auto &send : channel_track->get_channel ()->sends_)
                    {
                      send->enabled_->control_ = 0.f;
                    }
                }

              /* remove children */
              if (track->can_be_group_target ())
                {
                  auto group_target_track =
                    dynamic_cast<GroupTargetTrack *> (track.get ());
                  group_target_track->children_.clear ();
                }

              int target_pos = track_pos_ + i;
              if (inside)
                target_pos++;

              /* add to tracklist at given pos */
              auto added_track = TRACKLIST->insert_track (
                std::move (track), target_pos, false, false);

              /* select it */
              added_track->select (true, false, false);
              new_tracks.push_back (added_track);
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
                            track_send->stereo_out_->get_l ().id_,
                            own_conns.at (0).dest_id_, 1.f, true, true);
                          PORT_CONNECTIONS_MGR->ensure_connect (
                            track_send->stereo_out_->get_r ().id_,
                            own_conns.at (1).dest_id_, 1.f, true, true);
                        }
                      else if (
                        !own_conns.empty ()
                        && track->out_signal_type_ == PortType::Event)
                        {
                          PORT_CONNECTIONS_MGR->ensure_connect (
                            track_send->midi_out_->id_,
                            own_conns.front ().dest_id_, 1.f, true, true);
                        }
                    }

                } /* endif track has channel */

            } /* endforeach track */

          EVENTS_PUSH (EventType::ET_TRACK_ADDED, nullptr);
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);

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

              auto prj_track = TRACKLIST->find_track_by_name (own_track->name_);
              z_return_if_fail (prj_track);

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
            }

          EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr);
        }
      else if (copy)
        {
          for (int i = tls_before_->tracks_.size () - 1; i >= 0; i--)
            {
              /* get the track from the inserted pos */
              int target_pos = track_pos_ + i;
              if (inside)
                target_pos++;

              auto &prj_track = TRACKLIST->tracks_[target_pos];
              z_return_if_fail (prj_track);

              /* remove it */
              TRACKLIST->remove_track (*prj_track, true, true, false, false);
            }
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
          EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr);
        }

      if (inside)
        {
          /* update foldable track sizes (incl. parents) */
          auto foldable_tr = TRACKLIST->get_track<FoldableTrack> (track_pos_);
          z_return_if_fail (foldable_tr);

          foldable_tr->add_to_size (-num_fold_change_tracks_);
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
      CLIP_EDITOR->set_region (prev_clip_editor_region, false);
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
      auto &track = TRACKLIST->tracks_[track_positions_before_[i]];
      z_return_if_fail (track);
      auto channel_track = dynamic_cast<ChannelTrack *> (track.get ());
      auto laned_track = dynamic_cast<LanedTrack *> (track.get ());

      switch (edit_type_)
        {
        case EditType::Solo:
          if (track->has_channel ())
            {
              bool soloed = channel_track->get_soloed ();
              channel_track->set_soloed (
                _do ? ival_after_ : ival_before_[i], false, false, false);

              ival_before_[i] = soloed;
            }
          break;
        case EditType::SoloLane:
          {
            std::visit (
              [&] (auto &&casted_track) {
                auto &lane = casted_track->lanes_[lane_pos_];
                bool  soloed = lane->get_soloed ();
                lane->set_soloed (
                  _do ? ival_after_ : ival_before_[i], false, false);

                ival_before_[i] = soloed;
              },
              convert_to_variant<LanedTrackPtrVariant> (laned_track));
          }
          break;
        case EditType::Mute:
          if (track->has_channel ())
            {
              bool muted = channel_track->get_muted ();
              channel_track->set_muted (
                _do ? ival_after_ : ival_before_[i], false, false, false);

              ival_before_[i] = muted;
            }
          break;
        case EditType::MuteLane:
          {
            std::visit (
              [&] (auto &&t) {
                auto &lane = t->lanes_[lane_pos_];
                bool  muted = lane->get_muted ();
                lane->set_muted (
                  _do ? ival_after_ : ival_before_[i], false, false);

                ival_before_[i] = muted;
              },
              convert_to_variant<LanedTrackPtrVariant> (laned_track));
          }
          break;
        case EditType::Listen:
          if (track->has_channel ())
            {
              bool listened = channel_track->get_listened ();
              channel_track->set_listened (
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
            auto foldable_track = dynamic_cast<FoldableTrack *> (track.get ());
            bool folded = foldable_track->folded_;
            foldable_track->set_folded (
              _do ? ival_after_ : ival_before_[i], false, false, true);

            ival_before_[i] = folded;
          }
          break;
        case EditType::Volume:
          z_return_if_fail (channel_track);
          channel_track->channel_->fader_->set_amp (
            _do ? val_after_ : val_before_);
          break;
        case EditType::Pan:
          z_return_if_fail (channel_track);
          channel_track->get_channel ()->set_balance_control (
            _do ? val_after_ : val_before_);
          break;
        case EditType::MidiFaderMode:
          z_return_if_fail (channel_track);
          if (_do)
            {
              ival_before_[i] =
                static_cast<int> (channel_track->channel_->fader_->midi_mode_);
              channel_track->channel_->fader_->set_midi_mode (
                static_cast<Fader::MidiFaderMode> (ival_after_), false, true);
            }
          else
            {
              channel_track->channel_->fader_->set_midi_mode (
                static_cast<Fader::MidiFaderMode> (ival_before_[i]), false,
                true);
            }
          break;
        case EditType::DirectOut:
          {
            z_return_if_fail (channel_track);

            int cur_direct_out_pos = -1;
            if (channel_track->get_channel ()->has_output_)
              {
                auto cur_direct_out_track =
                  channel_track->get_channel ()->get_output_track ();
                cur_direct_out_pos = cur_direct_out_track->pos_;
              }

            /* disconnect from the current track */
            if (channel_track->get_channel ()->has_output_)
              {
                auto target_track = TRACKLIST->find_track_by_name_hash (
                  channel_track->get_channel ()->output_name_hash_);
                dynamic_cast<GroupTargetTrack *> (target_track)
                  ->remove_child (track->get_name_hash (), true, false, true);
              }

            int target_pos = _do ? ival_after_ : ival_before_[i];

            /* reconnect to the new track */
            if (target_pos != -1)
              {
                z_return_if_fail_cmp (
                  target_pos, !=, channel_track->channel_->track_->pos_);
                auto group_target_track =
                  TRACKLIST->get_track<GroupTargetTrack> (target_pos);
                z_return_if_fail (group_target_track);
                group_target_track->add_child (
                  track->get_name_hash (), true, false, true);
              }

            /* remember previous pos */
            ival_before_[i] = cur_direct_out_pos;

            need_recalc_graph = true;
          }
          break;
        case EditType::Rename:
          {
            const auto &cur_name = track->get_name ();
            track->set_name (new_txt_, false);

            /* remember the new name */
            new_txt_ = cur_name;

            need_tracklist_cache_update = true;
            need_recalc_graph = true;
          }
          break;
        case EditType::RenameLane:
          {
            std::visit (
              [&] (auto &&t) {
                auto       &lane = t->lanes_[lane_pos_];
                const auto &cur_name = lane->name_;
                lane->rename (new_txt_, false);

                /* remember the new name */
                new_txt_ = cur_name;
              },
              convert_to_variant<LanedTrackPtrVariant> (laned_track));
          }
          break;
        case EditType::Color:
          {
            auto cur_color = track->color_;
            track->set_color (_do ? new_color_ : colors_before_[i], false, true);

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

      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track.get ());
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

std::string
TracklistSelectionsAction::to_string () const
{
  switch (tracklist_selections_action_type_)
    {
    case Type::Copy:
    case Type::CopyInside:
      if (tls_before_->tracks_.size () == 1)
        {
          if (tracklist_selections_action_type_ == Type::CopyInside)
            return _ ("Copy Track inside");
          else
            return _ ("Copy Track");
        }
      else
        {
          if (tracklist_selections_action_type_ == Type::CopyInside)
            return format_str (
              _ ("Copy {} Tracks inside"), tls_before_->tracks_.size ());
          else
            return format_str (
              _ ("Copy {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Create:
      {
        auto type = Track_Type_to_string (track_type_);
        if (num_tracks_ == 1)
          {
            return format_str (_ ("Create {} Track"), type);
          }
        else
          {
            return format_str (_ ("Create {} {} Tracks"), num_tracks_, type);
          }
      }
    case Type::Delete:
      if (tls_before_->tracks_.size () == 1)
        {
          return _ ("Delete Track");
        }
      else
        {
          return format_str (
            _ ("Delete {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Edit:
      if (
        (tls_before_ && tls_before_->tracks_.size () == 1) || (num_tracks_ == 1))
        {
          switch (edit_type_)
            {
            case EditType::Solo:
              if (ival_after_)
                return _ ("Solo Track");
              else
                return _ ("Unsolo Track");
            case EditType::SoloLane:
              if (ival_after_)
                return _ ("Solo Lane");
              else
                return _ ("Unsolo Lane");
            case EditType::Mute:
              if (ival_after_)
                return _ ("Mute Track");
              else
                return _ ("Unmute Track");
            case EditType::MuteLane:
              if (ival_after_)
                return _ ("Mute Lane");
              else
                return _ ("Unmute Lane");
            case EditType::Listen:
              if (ival_after_)
                return _ ("Listen Track");
              else
                return _ ("Unlisten Track");
            case EditType::Enable:
              if (ival_after_)
                return _ ("Enable Track");
              else
                return _ ("Disable Track");
            case EditType::Fold:
              if (ival_after_)
                return _ ("Fold Track");
              else
                return _ ("Unfold Track");
            case EditType::Volume:
              return format_str (
                _ ("Change Fader from {:.1f} to {:.1f}"), val_before_,
                val_after_);
            case EditType::Pan:
              return format_str (
                _ ("Change Pan from {:.1f} to {:.1f}"), val_before_, val_after_);
            case EditType::DirectOut:
              return _ ("Change direct out");
            case EditType::Rename:
              return _ ("Rename track");
            case EditType::RenameLane:
              return _ ("Rename lane");
            case EditType::Color:
              return _ ("Change color");
            case EditType::Icon:
              return _ ("Change icon");
            case EditType::Comment:
              return _ ("Change comment");
            case EditType::MidiFaderMode:
              return _ ("Change MIDI fader mode");
            }
        }
      else
        {
          switch (edit_type_)
            {
            case EditType::Solo:
              if (ival_after_)
                return format_str (_ ("Solo {} Tracks"), num_tracks_);
              else
                return format_str (_ ("Unsolo {} Tracks"), num_tracks_);
            case EditType::Mute:
              if (ival_after_)
                return format_str (_ ("Mute {} Tracks"), num_tracks_);
              else
                return format_str (_ ("Unmute {} Tracks"), num_tracks_);
            case EditType::Listen:
              if (ival_after_)
                return format_str (_ ("Listen {} Tracks"), num_tracks_);
              else
                return format_str (_ ("Unlisten {} Tracks"), num_tracks_);
            case EditType::Enable:
              if (ival_after_)
                return format_str (_ ("Enable {} Tracks"), num_tracks_);
              else
                return format_str (_ ("Disable {} Tracks"), num_tracks_);
            case EditType::Fold:
              if (ival_after_)
                return format_str (_ ("Fold {} Tracks"), num_tracks_);
              else
                return format_str (_ ("Unfold {} Tracks"), num_tracks_);
            case EditType::Color:
              return _ ("Change color");
            case EditType::MidiFaderMode:
              return _ ("Change MIDI fader mode");
            case EditType::DirectOut:
              return _ ("Change direct out");
            case EditType::SoloLane:
              return _ ("Solo lanes");
            case EditType::MuteLane:
              return _ ("Mute lanes");
            default:
              return _ ("Edit tracks");
            }
        }
    case Type::Move:
    case Type::MoveInside:
      if (tls_before_->tracks_.size () == 1)
        {
          if (tracklist_selections_action_type_ == Type::MoveInside)
            {
              return _ ("Move Track inside");
            }
          else
            {
              return _ ("Move Track");
            }
        }
      else
        {
          if (tracklist_selections_action_type_ == Type::MoveInside)
            {
              return format_str (
                _ ("Move {} Tracks inside"), tls_before_->tracks_.size ());
            }
          else
            {
              return format_str (
                _ ("Move {} Tracks"), tls_before_->tracks_.size ());
            }
        }
    case Type::Pin:
      if (tls_before_->tracks_.size () == 1)
        {
          return _ ("Pin Track");
        }
      else
        {
          return format_str (_ ("Pin {} Tracks"), tls_before_->tracks_.size ());
        }
    case Type::Unpin:
      if (tls_before_->tracks_.size () == 1)
        {
          return _ ("Unpin Track");
        }
      else
        {
          return format_str (
            _ ("Unpin {} Tracks"), tls_before_->tracks_.size ());
        }
    }

  z_return_val_if_reached ("");
}