// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/router.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/file_descriptor.h"
#include "gui/backend/io/midi_file.h"
#include "gui/backend/ui.h"
#include "gui/dsp/plugin.h"
#include "structure/tracks/tracklist.h"
#include "utils/base64.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/io.h"
#include "utils/traits.h"
#include "utils/types.h"
#include "utils/views.h"

namespace zrythm::gui::actions
{
using namespace zrythm::structure::tracks;

void
init_from (
  TracklistSelectionsAction       &obj,
  const TracklistSelectionsAction &other,
  utils::ObjectCloneType           clone_type)
{
  init_from (
    static_cast<UndoableAction &> (obj),
    static_cast<const UndoableAction &> (other), clone_type);
  obj.tracklist_selections_action_type_ =
    other.tracklist_selections_action_type_;
  obj.track_type_ = other.track_type_;
  if (other.pl_setting_)
    obj.pl_setting_ = utils::clone_unique (*other.pl_setting_);
  obj.is_empty_ = other.is_empty_;
  obj.track_pos_ = other.track_pos_;
  obj.lane_pos_ = other.lane_pos_;
  obj.have_pos_ = other.have_pos_;
  obj.pos_ = other.pos_;
  obj.ival_before_ = other.ival_before_;
  obj.colors_before_ = other.colors_before_;
  obj.track_positions_before_ = other.track_positions_before_;
  obj.track_positions_after_ = other.track_positions_after_;
  obj.num_tracks_ = other.num_tracks_;
  obj.ival_after_ = other.ival_after_;
  obj.new_color_ = other.new_color_;
  obj.file_basename_ = other.file_basename_;
  obj.base64_midi_ = other.base64_midi_;
  obj.pool_id_ = other.pool_id_;
#if 0
  if (other.tls_before_)
    obj.tls_before_ = TrackSpan{ *other.tls_before_ }.create_snapshots (
      obj, PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
      PROJECT->get_port_registry (), PROJECT->get_arranger_object_registry ());
  if (other.tls_after_)
    obj.tls_after_ = TrackSpan{ *other.tls_after_ }.create_snapshots (
      obj, PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
      PROJECT->get_port_registry (), PROJECT->get_arranger_object_registry ());
  if (other.foldable_tls_before_)
    obj.foldable_tls_before_ =
      TrackSpan{ *obj.foldable_tls_before_ }.create_snapshots (
        obj, PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
        PROJECT->get_port_registry (), PROJECT->get_arranger_object_registry ());
#endif
  obj.out_track_uuids_ = other.out_track_uuids_;
  // TODO
  // utils::clone_unique_ptr_container (obj.src_sends_, other.src_sends_);
  obj.edit_type_ = other.edit_type_;
  obj.new_txt_ = other.new_txt_;
  obj.val_before_ = other.val_before_;
  obj.val_after_ = other.val_after_;
  obj.num_fold_change_tracks_ = other.num_fold_change_tracks_;
}

void
TracklistSelectionsAction::copy_track_positions_from_selections (
  std::vector<int> &track_positions,
  TrackSpan         selections_var)
{
  num_tracks_ = static_cast<int> (selections_var.size ());
  for (const auto &track_var : selections_var)
    {
      track_positions.push_back (
        std::visit (TrackSpan::position_projection, track_var));
    }
  std::ranges::sort (track_positions);
}

void
TracklistSelectionsAction::reset_foldable_track_sizes ()
{
  for (auto own_tr_var : *foldable_tls_before_)
    {
      std::visit (
        [&] (auto &&own_tr) {
          using TrackT = base_type<decltype (own_tr)>;
          auto prj_tr_var = *TRACKLIST->get_track (own_tr->get_uuid ());
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              std::get<TrackT *> (prj_tr_var)->size_ = own_tr->size_;
            }
        },
        own_tr_var);
    }
}

bool
TracklistSelectionsAction::validate () const
{
  if (tracklist_selections_action_type_ == Type::Delete)
    {
      if (
        !tls_before_ || TrackSpan{ *tls_before_ }.contains_undeletable_track ())
        {
          return false;
        }
    }
  else if (tracklist_selections_action_type_ == Type::MoveInside)
    {
      if (
        !tls_before_
        || std::ranges::contains (
          TrackSpan{ *tls_before_ }, track_pos_, TrackSpan::position_projection))
        return false;
    }

  return true;
}

TracklistSelectionsAction::TracklistSelectionsAction (
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
  bool                           already_edited)
    : UndoableAction (
        UndoableAction::Type::TracklistSelections,
        AUDIO_ENGINE->frames_per_tick_,
        AUDIO_ENGINE->get_sample_rate ()),
      tracklist_selections_action_type_ (type), track_pos_ (track_pos),
      lane_pos_ (lane_pos), track_type_ (track_type),
      pl_setting_ (pl_setting ? utils::clone_unique (*pl_setting) : nullptr),
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

  const bool need_full_selections = type != Type::Edit;
  const bool is_copy_action =
    type == TracklistSelectionsAction::Type::Copy
    || type == TracklistSelectionsAction::Type::CopyInside;

  if (is_copy_action && tls_before_var->contains_uncopyable_track ())
    {
      throw ZrythmException (
        QObject::tr (
          "Cannot copy tracks: selection contains an uncopyable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::Delete
    && tls_before_var->contains_undeletable_track ())
    {
      throw ZrythmException (
        QObject::tr (
          "Cannot delete tracks: selection contains an undeletable track"));
    }

  if (
    tracklist_selections_action_type_ == Type::MoveInside
    || tracklist_selections_action_type_ == Type::CopyInside)
    {
      [[maybe_unused]] auto _foldable_tr =
        TRACKLIST->get_track_at_index (track_pos);
      assert (
        TrackSpan::derived_from_type_projection<FoldableTrack> (_foldable_tr));
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

  if (pl_setting != nullptr)
    {
      pl_setting_->validate ();
    }
  else if (file_descr == nullptr)
    {
      is_empty_ = true;
    }
  if (type == Type::Pin)
    {
      track_pos_ = TRACKLIST->get_pinned_tracks_cutoff_index ();
    }
  else if (type == Type::Unpin)
    {
      track_pos_ = TRACKLIST->track_count () - 1;
    }

  if (pos != nullptr)
    {
      pos_ = *pos;
      have_pos_ = true;
    }

  /* calculate number of tracks */
  if ((file_descr != nullptr) && track_type == Track::Type::Midi)
    {
      MidiFile mf (file_descr->abs_path_);
      num_tracks_ = mf.get_num_tracks (true);
    }
  else
    {
      num_tracks_ = num_tracks;
    }

  /* create the file in the pool or save base64 if MIDI */
  if (file_descr != nullptr)
    {
      /* TODO: use async API */
      if (track_type == Track::Type::Midi)
        {
          try
            {
              auto contents =
                utils::io::read_file_contents (file_descr->abs_path_);
              base64_midi_ = utils::Utf8String::from_qstring (
                utils::base64::encode (contents));
            }
          catch (const std::exception &e)
            {
              throw ZrythmException (
                fmt::format ("Failed to get MIDI file contents: {}", e.what ()));
            }
        }
      else if (track_type == Track::Type::Audio)
        {
          pool_id_ =
            PROJECT->get_file_audio_source_registry ()
              .create_object<dsp::FileAudioSource> (
                file_descr->abs_path_, AUDIO_ENGINE->get_sample_rate (), 140.f);
        }
      else
        {
          z_return_if_reached ();
        }

      file_basename_ = utils::io::path_get_basename (file_descr->abs_path_);
    }

  if (tls_before_var)
    {
      const auto &tls_before = *tls_before_var;
      if (tls_before.empty ())
        {
          throw ZrythmException (QObject::tr ("No tracks selected"));
        }

      if (need_full_selections)
        {
          if (is_copy_action)
            {
// TODO
#if 0
                  tls_before_ = tls_before.create_new_identities (
                    PROJECT->get_track_registry (),
                    PROJECT->get_plugin_registry (),
                    PROJECT->get_port_registry (),
                    PROJECT->get_arranger_object_registry ());
#endif
            }
          else
            {
// TODO
#if 0
              // FIXME: this QObject is not initialized yet
              tls_before_ = tls_before.create_snapshots (
                *this, PROJECT->get_track_registry (),
                PROJECT->get_plugin_registry (), PROJECT->get_port_registry (),
                PROJECT->get_arranger_object_registry ());
#endif
            }
          TrackCollections::sort_by_position (*tls_before_);

          // FIXME: need new identities?
          const auto foldable_tracks = std::ranges::to<std::vector> (
            TRACKLIST->get_track_span ()
            | std::views::filter (TrackSpan::foldable_projection));
#if 0
          foldable_tls_before_ = TrackSpan{ foldable_tracks }.create_snapshots (
            *this, PROJECT->get_track_registry (),
            PROJECT->get_plugin_registry (), PROJECT->get_port_registry (),
            PROJECT->get_arranger_object_registry ());
#endif
        }
      else
        {
          copy_track_positions_from_selections (
            track_positions_before_, tls_before);
        }
    }

  if (tls_after_var.has_value ())
    {
      const auto &tls_after = tls_after_var.value ();
      if (need_full_selections)
        {
          if (is_copy_action)
            {
// TODO
#if 0
                  tls_after_ = tls_after.create_new_identities (
                    PROJECT->get_track_registry (),
                    PROJECT->get_plugin_registry (),
                    PROJECT->get_port_registry (),
                    PROJECT->get_arranger_object_registry ());
#endif
            }
          else
            {
              // FIXME: this QObject is not initialized yet
#if 0
              tls_after_ = tls_after.create_snapshots (
                *this, PROJECT->get_track_registry (),
                PROJECT->get_plugin_registry (), PROJECT->get_port_registry (),
                PROJECT->get_arranger_object_registry ());
#endif
            }

          TrackCollections::sort_by_position (*tls_after_);
        }
      else
        {
          copy_track_positions_from_selections (
            track_positions_after_, tls_after);
        }
    }

  /* if need to clone tls_before */
  if (tls_before_var && need_full_selections)
    {
      /* save the outputs & incoming sends */
      for (const auto &clone_track_var : *tls_before_)
        {
          std::visit (
            [&] (auto &&clone_track) {
              if constexpr (
                std::derived_from<base_type<decltype (clone_track)>, ChannelTrack>)
                {
                  if (clone_track->channel_->has_output ())
                    {
                      out_track_uuids_.push_back (
                        clone_track->channel_->output_track_uuid_);
                    }
                  else
                    {
                      out_track_uuids_.emplace_back (std::nullopt);
                    }
                }
              else
                {
                  out_track_uuids_.emplace_back (std::nullopt);
                }

              for (
                const auto &cur_track :
                TRACKLIST->get_track_span ()
                  | std::views::filter (
                    TrackSpan::derived_from_type_projection<ChannelTrack>)
                  | std::views::transform (
                    TrackSpan::derived_type_transformation<ChannelTrack>))
                {
                  for (
                    const auto &send :
                    cur_track->channel_->sends_
                      | std::views::filter (&ChannelSend::is_enabled))
                    {
                      Track * target_track = send->get_target_track ();
                      z_return_if_fail (target_track);

                      if (
                        target_track->get_index () == clone_track->get_index ())
                        {
                          src_sends_.emplace_back (
                            utils::clone_unique (
                              *send, utils::ObjectCloneType::Snapshot,
                              PROJECT->get_track_registry (),
                              PROJECT->get_port_registry (),
                              PROJECT->get_param_registry ()));
                        }
                    }
                }
            },
            clone_track_var);
        }
    }

  if (tracklist_selections_action_type_ == Type::Edit && track_var.has_value ())
    {
      num_tracks_ = 1;
      std::visit (
        [&] (auto &&track) {
          track_positions_before_.push_back (track->get_index ());
          track_positions_after_.push_back (track->get_index ());
        },
        track_var.value ());
    }

  if (color_new != nullptr)
    {
      new_color_ = *color_new;
    }
  if (new_txt != nullptr)
    {
      new_txt_ = *new_txt;
    }

  ival_before_.resize (num_tracks_, 0);
  colors_before_.resize (num_tracks_);

  if (port_connections_mgr != nullptr)
    {
      port_connections_before_ = utils::clone_unique (*port_connections_mgr);
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
      auto track_id_ref =
        PROJECT->getTrackFactory ()->create_empty_track (track_type_);
      std::visit (
        [&] (auto &&track) {
          track->setName (label);
          TRACKLIST->insert_track (
            track_id_ref, pos, *AUDIO_ENGINE, false, false);
        },
        track_id_ref.get_object ());
    }
  else /* else if track is not empty */
    {
      TrackUniquePtrVariant track;

      /* if creating audio track from file */
      bool              has_plugin = false;
      utils::Utf8String name{};
      if (track_type_ == Track::Type::Audio && pool_id_.has_value ())
        {
          track = AudioTrack::create_unique (
            PROJECT->get_file_audio_source_registry (),
            PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
            PROJECT->get_port_registry (), PROJECT->get_param_registry (),
            PROJECT->get_arranger_object_registry (), true);
          // TODO set samplerate on AudioTrack
          name = utils::Utf8String::from_path (file_basename_);
        }
      /* else if creating MIDI track from file */
      else if (track_type_ == Track::Type::Midi && !base64_midi_.empty ())
        {
          track = MidiTrack::create_unique (
            PROJECT->get_file_audio_source_registry (),
            PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
            PROJECT->get_port_registry (), PROJECT->get_param_registry (),
            PROJECT->get_arranger_object_registry (), true);
          name = utils::Utf8String::from_path (file_basename_);
        }
      /* at this point we can assume it has a plugin */
      else
        {
          const auto &descr = pl_setting_->descr_;

          auto track_id_ref =
            PROJECT->getTrackFactory ()->create_empty_track (track_type_);
          std::visit (
            [&descr] (auto &&track_ptr) {
              track_ptr->setName (descr->getName ());
            },
            track_id_ref.get_object ());
          // TODO
          // track = track_id_ref.get_object ();
          has_plugin = true;
          name = descr->name_;
        }

      std::visit (
        [&] (auto &&unique_track) {
          auto * added_track = unique_track.release ();
          using TrackT = base_type<decltype (added_track)>;
          PROJECT->get_track_registry ().register_object (added_track);
          TrackUuidReference added_track_ref{
            added_track->get_uuid (), PROJECT->get_track_registry ()
          };
          added_track->setName (name.to_qstring ());
          added_track->set_index (pos);

          std::optional<PluginUuidReference> added_plugin_if_has_plugin;
          if (has_plugin)
            {
              added_plugin_if_has_plugin =
                PROJECT->getPluginFactory ()->create_plugin_from_setting (
                  *pl_setting_);
              auto * pl_ptr = std::get<old_dsp::plugins::CarlaNativePlugin *> (
                added_plugin_if_has_plugin->get_object ());
              pl_ptr->instantiate ();
              pl_ptr->activate ();
            }

          TRACKLIST->insert_track (
            added_track_ref, pos, *AUDIO_ENGINE, false, false);

          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              if (has_plugin)
                {
                  added_track->channel_->add_plugin (
                    *added_plugin_if_has_plugin,
                    std::is_same_v<TrackT, InstrumentTrack>
                      ? plugins::PluginSlot (plugins::PluginSlotType::Instrument)
                      : plugins::PluginSlot (
                          plugins::PluginSlotType::Insert,
                          PluginSpan::slot_projection (
                            added_plugin_if_has_plugin->get_object ())
                            .get_slot_with_index ()
                            .second),
                    true, false, true, false, false);
                }
            }

          Position start_pos = have_pos_ ? pos_ : Position ();
          if (track_type_ == Track::Type::Audio)
            {
              if constexpr (std::is_same_v<TrackT, AudioTrack>)
                {
                  /* create an audio region & add to track*/
                  structure::arrangement::ArrangerObjectFactory::get_instance ()
                    ->add_audio_region_with_clip (
                      added_track->get_lane_at (0), *pool_id_, start_pos.ticks_);
                }
            }
          else if (
            track_type_ == Track::Type::Midi && !base64_midi_.empty ()
            && !file_basename_.empty ())
            {
              if constexpr (std::derived_from<TrackT, PianoRollTrack>)
                {
                  /* create a temporary midi file */
                  auto full_path_file = utils::io::make_tmp_file (
                    std::make_optional<utils::Utf8String> (u8"data.MID"));
                  const auto full_path =
                    utils::Utf8String::from_qstring (full_path_file->fileName ())
                      .to_path ();
                  auto data = utils::base64::decode (
                    QByteArray::fromStdString (base64_midi_.str ()));
                  utils::io::set_file_contents (
                    full_path, data.constData (), data.size ());

                  /* create a MIDI region from the MIDI file & add to track */
                  structure::arrangement::ArrangerObjectFactory::get_instance ()
                    ->addMidiRegionFromMidiFile (
                      &added_track->get_lane_at (0),
                      utils::Utf8String::from_path (full_path).to_qstring (),
                      pos_.ticks_, idx);
                }
            }

          if (
            has_plugin && ZRYTHM_HAVE_UI
            && gui::SettingsManager::get_instance ()
                 ->get_openPluginsOnInstantiation ())
            {
              std::visit (
                [&] (auto &&plugin) {
                  plugin->visible_ = true;
                  /* EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                   * added_pl); */
                },
                added_plugin_if_has_plugin->get_object ());
            }
        },
        track);
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
                ival_after_ < static_cast<int> (TRACKLIST->track_count ()));
              auto _tr_to_disable = TRACKLIST->get_track_at_index (ival_after_);
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
          for (auto &own_track_var : *tls_before_)
            {
              std::visit (
                [&] (auto &&own_track) {
                  using TrackT = base_type<decltype (own_track)>;

                  /* clone our own track */
                  auto track_ref = PROJECT->get_track_registry ().clone_object (
                    *own_track, PROJECT->get_file_audio_source_registry (),
                    PROJECT->get_track_registry (),
                    PROJECT->get_plugin_registry (),
                    PROJECT->get_port_registry (),
                    PROJECT->get_param_registry (),
                    PROJECT->get_arranger_object_registry (), true);
                  // TODO...
                  auto track = std::get<TrackT *> (track_ref.get_object ());

                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      /* remove output */
                      track->get_channel ()->output_track_uuid_ = std::nullopt;

                      /* remove the sends (will be added later) */
                      for (auto &send : track->get_channel ()->sends_)
                        {
                          send->get_enabled_param ().setBaseValue (0.f);
                        }
                    }

                  /* insert it to the tracklist at its original pos */
                  TRACKLIST->insert_track (
                    track_ref, track->get_index (), *AUDIO_ENGINE, false, false);

                  /* if group track, readd all children */
                  if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
                    {
                      track->add_children (
                        own_track->children_, false, false, false);
                    }

                  if constexpr (std::is_same_v<TrackT, InstrumentTrack>)
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
                },
                own_track_var);
            }

          for (
            const auto &[own_track_var, out_track_id] :
            std::views::zip (*tls_before_, out_track_uuids_))
            {
              std::visit (
                [&] (auto &&own_track) {
                  using TrackT = base_type<decltype (own_track)>;

                  /* get the project track */
                  auto prj_track = std::get<TrackT *> (
                    TRACKLIST->get_track_at_index (own_track->get_index ()));
                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      /* reconnect output */
                      if (out_track_id.has_value ())
                        {
                          auto out_track =
                            prj_track->get_channel ()->get_output_track ();
                          out_track->remove_child (
                            prj_track->get_uuid (), true, false, false);
                          auto out_track_var =
                            PROJECT->get_track_registry ().find_by_id (
                              out_track_id.value ());
                          if (!out_track_var.has_value ())
                            {
                              throw ZrythmException ("Out track not found");
                            }
                          out_track = std::visit (
                            [&] (auto &&out_track_inner) -> GroupTargetTrack * {
                              using InnerTrackT =
                                base_type<decltype (out_track_inner)>;
                              if constexpr (
                                std::derived_from<InnerTrackT, GroupTargetTrack>)
                                return out_track_inner;
                              return nullptr;
                            },
                            out_track_var->get ());
                          out_track->add_child (
                            prj_track->get_uuid (), true, false, false);
                        }

                      /* reconnect any sends sent from the track */
                      for (
                        const auto &[clone_send, send] : std::views::zip (
                          own_track->get_channel ()->sends_,
                          prj_track->get_channel ()->sends_))
                        {
                          send->copy_values_from (*clone_send);
                        }

                      /* reconnect any custom connections */
// TODO
#if 0
                      std::vector<dsp::Port *> ports;
                      own_track->append_ports (ports, true);
                      for (auto * port : ports)
                        {
                          auto prj_port_var =
                            PROJECT->find_port_by_id (port->get_uuid ());
                          z_return_if_fail (prj_port_var);
                          std::visit (
                            [&] (auto &&prj_port) {
                              prj_port->restore_from_non_project (*port);
                            },
                            *prj_port_var);
                        }
#endif
                    }
                },
                own_track_var);
            }

          /* re-connect any source sends */
          for (auto &clone_send : src_sends_)
            {

              /* get the original send and connect it */
              auto * prj_send = clone_send->find_in_project ();
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
          for (int i : std::views::iota (0, num_tracks_) | std::views::reverse)
            {
              auto tr = TRACKLIST->get_track_at_index (track_pos_ + i);
              std::visit (
                [&] (auto &&track) {
                  TRACKLIST->remove_track (track->get_uuid ());
                },
                tr);
            }

          /* reenable given track, if any (eg when bouncing) */
          if (ival_after_ > -1)
            {
              z_return_if_fail (
                ival_after_ < static_cast<int> (TRACKLIST->track_count ()));
              auto tr_to_enable = TRACKLIST->get_track_at_index (ival_after_);
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

          for (
            const auto &own_track_var :
            TrackSpan{ *tls_before_ } | std::views::reverse)
            {
              std::visit (
                [&] (auto &&own_track) {
                  using TrackT = base_type<decltype (own_track)>;
                  /* get track from pos */
                  auto prj_track = std::get<TrackT *> (
                    TRACKLIST->get_track_at_index (own_track->get_index ()));
                  /* remember any custom connections */
                  std::vector<dsp::Port *> prj_ports;
                  prj_track->append_ports (prj_ports, true);
                  std::vector<dsp::Port *> clone_ports;
                  own_track->append_ports (clone_ports, true);
                  for (auto * prj_port : prj_ports)
                    {
                      dsp::Port * clone_port = nullptr;
                      for (auto * cur_clone_port : clone_ports)
                        {
                          if (
                            cur_clone_port->get_uuid () == prj_port->get_uuid ())
                            {
                              clone_port = cur_clone_port;
                              break;
                            }
                        }
                      z_return_if_fail (clone_port);

                      // TODO
                      // clone_port->copy_metadata_from_project (*prj_port);
                    }

                  /* if group track, remove all children */
                  if (prj_track->can_be_group_target ())
                    {
                      dynamic_cast<GroupTargetTrack &> (*prj_track)
                        .remove_all_children (true, false, false);
                    }

                  /* remove it */
                  TRACKLIST->remove_track (prj_track->get_uuid ());
                },
                own_track_var);
            }
        }

      /* EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr); */
      /* EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, nullptr); */
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  TRACKLIST->get_track_span ().set_caches (ALL_CACHE_TYPES);

  ROUTER->recalc_graph (false);
}

void
TracklistSelectionsAction::
  do_or_undo_move_or_copy (bool _do, bool copy, bool inside)
{
  bool move = !copy;
  bool pin = tracklist_selections_action_type_ == Type::Pin;
  bool unpin = tracklist_selections_action_type_ == Type::Unpin;

  /* if moving, this will be set back */
  auto prev_clip_editor_region_opt = CLIP_EDITOR->get_region_and_track ();

  if (_do)
    {
      FoldableTrack * foldable_tr = nullptr;
      num_fold_change_tracks_ = 0;
      if (inside)
        {
          auto tr = TRACKLIST->get_track_at_index (track_pos_);
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
          for (const auto &track_var : TrackSpan{ *tls_before_ })
            {
              std::visit (
                [&] (auto &&track) {
                  using TrackT = base_type<decltype (track)>;
                  auto prj_track = std::get<TrackT *> (
                    TRACKLIST->get_track (track->get_uuid ()).value ());

                  if (inside)
                    {
                      std::vector<FoldableTrack *> parents;
                      prj_track->add_folder_parents (parents, false);
                      if (
                        std::ranges::find (parents, foldable_tr)
                        == parents.end ())
                        num_fold_change_tracks_++;
                    }
                },
                track_var);
            }

          TRACKLIST->get_selection_manager ().clear_selection ();
          for (const auto &own_track_var : TrackSpan{ *tls_before_ })
            {
              std::visit (
                [&] (auto &&own_track) {
                  using TrackT = base_type<decltype (own_track)>;
                  auto prj_track = std::get<TrackT *> (
                    TRACKLIST->get_track (own_track->get_uuid ()).value ());
                  int target_pos = -1;
                  /* if not first track to be moved */
                  if (prev_track)
                    {
                      /* move to last track's index + 1 */
                      target_pos = prev_track->get_index () + 1;
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
                  own_track->set_index (prj_track->get_index ());

                  std::vector<FoldableTrack *> parents;
                  prj_track->add_folder_parents (parents, false);

                  TRACKLIST->move_track (
                    prj_track->get_uuid (), target_pos, true, std::nullopt);
                  prev_track = prj_track;

                  /* adjust parent sizes */
                  for (auto * parent : parents)
                    {
                      /* if new pos is outside parent */
                      if (
                        prj_track->get_index () < parent->get_index ()
                        || prj_track->get_index ()
                             >= parent->get_index () + parent->size_)
                        {
                          z_debug (
                            "new pos of {} ({}) is outside parent {}: parent--",
                            prj_track->get_name (), prj_track->get_index (),
                            parent->get_name ());
                          --parent->size_;
                        }

                      /* if foldable track is child of parent (size will be
                       * readded later) */
                      if (inside && parent->is_child (*foldable_tr))
                        {
                          z_debug (
                            "foldable track {} is child of parent {}: parent--",
                            foldable_tr->get_name (), parent->get_name ());
                          parent->size_--;
                        }
                    }

                  TRACKLIST->get_selection_manager ().append_to_selection (
                    prj_track->get_uuid ());

                  TRACKLIST->get_track_span ().print_tracks ();
                },
                own_track_var);
            }

          /* EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr); */
        }
      else if (copy)
        {
          unsigned int num_tracks = tls_before_->size ();

          if (inside)
            {
              num_fold_change_tracks_ = static_cast<int> (tls_before_->size ());
            }

          /* get outputs & sends */
          std::vector<GroupTargetTrack *> outputs_in_prj (num_tracks);
          std::vector<
            std::array<std::unique_ptr<ChannelSend>, Channel::STRIP_SIZE>>
            sends (num_tracks);
          std::vector<std::array<
            PortConnectionsManager::ConnectionsVector, Channel::STRIP_SIZE>>
            send_conns (num_tracks);
          for (size_t i = 0; i < num_tracks; i++)
            {
              auto own_track_var = TrackSpan (*tls_before_)[i];
              std::visit (
                [&] (auto &&own_track) {
                  using TrackT = base_type<decltype (own_track)>;
                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      outputs_in_prj.push_back (
                        own_track->get_channel ()->get_output_track ());

                      for (size_t j = 0; j < Channel::STRIP_SIZE; ++j)
                        {
                          auto &send = own_track->get_channel ()->sends_.at (j);
                          sends.at (i).at (j) = utils::clone_unique (
                            *send, utils::ObjectCloneType::Snapshot,
                            PROJECT->get_track_registry (),
                            PROJECT->get_port_registry (),
                            PROJECT->get_param_registry ());

                          send->append_connection (
                            port_connections_before_.get (),
                            send_conns.at (i).at (j));
                        }
                    }
                  else
                    {
                      outputs_in_prj.push_back (nullptr);
                    }
                },
                own_track_var);
            }

          TRACKLIST->get_selection_manager ().clear_selection ();

          /* create new tracks routed to master */
          std::vector<Track *> new_tracks;
          for (
            const auto &[index, own_track_var] :
            utils::views::enumerate (TrackSpan{ *tls_before_ }))
            {
              std::visit (
                [&] (auto &&own_track_ptr) {
                  using TrackT = base_type<decltype (own_track_ptr)>;

                  /* create a new clone to use in the project */
                  auto track_ref = PROJECT->get_track_registry ().clone_object (
                    *own_track_ptr, PROJECT->get_file_audio_source_registry (),
                    PROJECT->get_track_registry (),
                    PROJECT->get_plugin_registry (),
                    PROJECT->get_port_registry (),
                    PROJECT->get_param_registry (),
                    PROJECT->get_arranger_object_registry (), true);
                  // FIXME
                  auto * track = std::get<TrackT *> (track_ref.get_object ());

                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      /* remove output */
                      // track->get_channel ()->has_output_ = false;
                      track->get_channel ()->output_track_uuid_ = std::nullopt;

                      /* remove sends */
                      for (auto &send : track->get_channel ()->sends_)
                        {
                          send->get_enabled_param ().setBaseValue (0.f);
                        }
                    }

                  /* remove children */
                  if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
                    {
                      track->children_.clear ();
                    }

                  auto target_pos = track_pos_ + index;
                  if (inside)
                    target_pos++;

                  /* add to tracklist at given pos */
                  TRACKLIST->insert_track (
                    track_ref, target_pos, *AUDIO_ENGINE, false, false);

                  /* select it */
                  TRACKLIST->get_selection_manager ().append_to_selection (
                    track->get_uuid ());
                  new_tracks.push_back (track);
                },
                own_track_var);
            }

          /* reroute new tracks to correct outputs & sends */
          for (
            const auto &[i, track_base] : utils::views::enumerate (new_tracks))
            {
              std::visit (
                [&] (auto &&track) {
                  using TrackT = base_type<decltype (track)>;
                  if constexpr (std::derived_from<TrackT, ChannelTrack>)
                    {
                      if (outputs_in_prj[i])
                        {
                          auto out_track =
                            track->get_channel ()->get_output_track ();
                          out_track->remove_child (
                            track->get_uuid (), true, false, false);
                          outputs_in_prj[i]->add_child (
                            track->get_uuid (), true, false, false);
                        }

                      for (
                        const auto &[own_send, own_conns, track_send] :
                        std::views::zip (
                          sends.at (i), send_conns.at (i),
                          track->get_channel ()->sends_))
                        {
                          track_send->copy_values_from (*own_send);
                          if (
                            !own_conns.empty ()
                            && track->get_output_signal_type () == PortType::Audio)
                            {
                              PORT_CONNECTIONS_MGR->add_connection (
                                track_send->stereo_out_left_id_->id (),
                                own_conns.at (0)->dest_id_, 1.f, true, true);
                              PORT_CONNECTIONS_MGR->add_connection (
                                track_send->stereo_out_right_id_->id (),
                                own_conns.at (1)->dest_id_, 1.f, true, true);
                            }
                          else if (
                            !own_conns.empty ()
                            && track->get_output_signal_type () == PortType::Event)
                            {
                              PORT_CONNECTIONS_MGR->add_connection (
                                track_send->midi_out_id_->id (),
                                own_conns.front ()->dest_id_, 1.f, true, true);
                            }
                        }
                    }
                },
                convert_to_variant<TrackPtrVariant> (track_base));

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
          auto tls_before_span = TrackSpan{ *tls_before_ };
          for (const auto &own_track_var : tls_before_span | std::views::reverse)
            {
              std::visit (
                [&] (auto &&own_track) {
                  auto _prj_track =
                    TRACKLIST->get_track (own_track->get_uuid ());
                  z_return_if_fail (_prj_track);
                  std::visit (
                    [&] (auto &&prj_track) {
                      int target_pos = own_track->get_index ();
                      TRACKLIST->move_track (
                        prj_track->get_uuid (), target_pos, false, std::nullopt);

                      if (
                        own_track->get_uuid ()
                        == std::visit (
                          TrackSpan::uuid_projection, tls_before_span.front ()))
                        {
                          TRACKLIST->get_selection_manager ().select_unique (
                            prj_track->get_uuid ());
                        }
                      else
                        {
                          TRACKLIST->get_selection_manager ()
                            .append_to_selection (prj_track->get_uuid ());
                        }
                    },
                    *_prj_track);
                },
                own_track_var);
            }

          /* EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr); */
        }
      else if (copy)
        {
          for (
            const auto i :
            std::views::iota (0_zu, tls_before_->size ()) | std::views::reverse)
            {
              /* get the track from the inserted pos */
              auto target_pos = track_pos_ + i;
              if (inside)
                target_pos++;

              /* remove it */
              const auto prj_track_id =
                TRACKLIST->get_track_ref_at_index (target_pos);
              TRACKLIST->remove_track (prj_track_id.id ());
            }
          /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
          /* EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr); */
        }

      if (inside)
        {
          /* update foldable track sizes (incl. parents) */
          auto _foldable_tr = TRACKLIST->get_track_at_index (track_pos_);
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
      TRACKLIST->set_pinned_tracks_cutoff_index (
        TRACKLIST->get_pinned_tracks_cutoff_index () + tls_before_->size ());
    }
  else if ((unpin && _do) || (pin && !_do))
    {
      TRACKLIST->set_pinned_tracks_cutoff_index (
        TRACKLIST->get_pinned_tracks_cutoff_index () - tls_before_->size ());
    }

  if (move && prev_clip_editor_region_opt.has_value ())
    {
// TODO
#if 0
      std::visit (
        [&] (auto &&r) { CLIP_EDITOR->set_region (r->get_uuid ()); },
        *prev_clip_editor_region_opt);
#endif
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  TRACKLIST->get_track_span ().set_caches (ALL_CACHE_TYPES);

  ROUTER->recalc_graph (false);
}

void
TracklistSelectionsAction::do_or_undo_edit (bool _do)
{
// TODO
#if 0
  if (_do && already_edited_)
    {
      already_edited_ = false;
      return;
    }

  bool need_recalc_graph = false;
  bool need_tracklist_cache_update = false;

  for (const auto i : std::views::iota (0, num_tracks_))
    {
      auto track_var =
        TRACKLIST->get_track_at_index (track_positions_before_[i]);
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          z_return_if_fail (track);

          switch (edit_type_)
            {
            case EditType::Solo:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  bool soloed = track->currently_soloed ();
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
                  bool muted = track->currently_muted ();
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
                  bool listened = track->currently_listened ();
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
                    if (track->get_channel ()->has_output ())
                      {
                        auto cur_direct_out_track =
                          track->get_channel ()->get_output_track ();
                        cur_direct_out_pos = cur_direct_out_track->get_index ();
                      }

                    /* disconnect from the current track */
                    if (track->get_channel ()->has_output ())
                      {
                        auto target_track = dynamic_cast<
                          GroupTargetTrack *> (Track::from_variant (
                          TRACKLIST
                            ->get_track (
                              track->get_channel ()->output_track_uuid_.value ())
                            .value ()));
                        target_track->remove_child (
                          track->get_uuid (), true, false, true);
                      }

                    int target_pos = _do ? ival_after_ : ival_before_[i];

                    /* reconnect to the new track */
                    if (target_pos != -1)
                      {
                        z_return_if_fail_cmp (
                          target_pos, !=, track->channel_->track_->get_index ());
                        auto _group_target_track =
                          TRACKLIST->get_track_at_index (target_pos);
                        std::visit (
                          [&] (auto &&group_target_track) {
                            using GroupTrackT =
                              base_type<decltype (group_target_track)>;
                            if constexpr (
                              std::derived_from<GroupTrackT, GroupTargetTrack>)
                              {
                                group_target_track->add_child (
                                  track->get_uuid (), true, false, true);
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
                Color cur_color = track->getColor ();
                track->set_color (
                  _do ? new_color_ : colors_before_[i], false, true);

                /* remember color */
                colors_before_[i] = cur_color;
              }
              break;
            case EditType::Icon:
              {
                auto cur_icon = track->get_icon_name ();
                track->set_icon (new_txt_, false, true);

                new_txt_ = cur_icon;
              }
              break;
            case EditType::Comment:
              {
                auto cur_comment = track->get_comment ();
                track->set_comment (new_txt_, false);

                new_txt_ = cur_comment;
              }
              break;
            }

          /* EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track.get ()); */
        },
        track_var);
    }

  /* restore connections */
  save_or_load_port_connections (_do);

  if (need_tracklist_cache_update)
    {
      TRACKLIST->get_track_span ().set_caches (ALL_CACHE_TYPES);
    }

  if (need_recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }
#endif
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

void
TracklistSelectionsAction::init_loaded_impl ()
{
// TODO
#if 0
  if (tls_before_)
    {
      TrackSpan{ *tls_before_ }.init_loaded (
        PROJECT->get_plugin_registry (), PROJECT->get_port_registry ());
    }
  if (tls_after_)
    {
      TrackSpan{ *tls_after_ }.init_loaded (
        PROJECT->get_plugin_registry (), PROJECT->get_port_registry ());
    }
  if (foldable_tls_before_)
    {
      TrackSpan{ *foldable_tls_before_ }.init_loaded (
        PROJECT->get_plugin_registry (), PROJECT->get_port_registry ());
    }
  for (auto &send : src_sends_)
    {
      send->init_loaded (nullptr);
    }
#endif
}

QString
TracklistSelectionsAction::to_string () const
{
  switch (tracklist_selections_action_type_)
    {
    case Type::Copy:
    case Type::CopyInside:
      if (tls_before_->size () == 1)
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
              QObject::tr ("Copy {} Tracks inside"), tls_before_->size ());
          else
            return format_qstr (
              QObject::tr ("Copy {} Tracks"), tls_before_->size ());
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
      if (tls_before_->size () == 1)
        {
          return QObject::tr ("Delete Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Delete {} Tracks"), tls_before_->size ());
        }
    case Type::Edit:
      if ((tls_before_ && tls_before_->size () == 1) || (num_tracks_ == 1))
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
      if (tls_before_->size () == 1)
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
                QObject::tr ("Move {} Tracks inside"), tls_before_->size ());
            }

          return format_qstr (
            QObject::tr ("Move {} Tracks"), tls_before_->size ());
        }
    case Type::Pin:
      if (tls_before_->size () == 1)
        {
          return QObject::tr ("Pin Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Pin {} Tracks"), tls_before_->size ());
        }
    case Type::Unpin:
      if (tls_before_->size () == 1)
        {
          return QObject::tr ("Unpin Track");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Unpin {} Tracks"), tls_before_->size ());
        }
    }

  z_return_val_if_reached ({});
}
}
