// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/midi_event.h"
#include "engine/session/router.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/track_processor.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"
#include "utils/string.h"

namespace zrythm::structure::tracks
{
Track::~Track () = default;

Track::Track (
  Type                  type,
  PortType              in_signal_type,
  PortType              out_signal_type,
  BaseTrackDependencies dependencies)
    : base_dependencies_ (dependencies), type_ (type),
      in_signal_type_ (in_signal_type), out_signal_type_ (out_signal_type)
{
  z_debug ("creating {} track", type);
}

Tracklist *
Track::get_tracklist () const
{
  if (tracklist_ != nullptr)
    return tracklist_;

  if (is_auditioner ())
    {
      return SAMPLE_PROCESSOR->tracklist_.get ();
    }
  else
    {
      return TRACKLIST;
    }
}

dsp::PortConnectionsManager *
Track::get_port_connections_manager () const
{
  auto * tracklist = get_tracklist ();
  z_return_val_if_fail (tracklist, nullptr);
  z_return_val_if_fail (tracklist_->port_connections_manager_, nullptr);
  return tracklist->port_connections_manager_.get ();
}

Track *
Track::from_variant (const TrackPtrVariant &variant)
{
  return std::visit ([&] (auto &&t) -> Track * { return t; }, variant);
}

#if 0
TrackUniquePtrVariant
Track::create_track (Track::Type type, const utils::Utf8String &name, int pos)
{
  auto track_var = [&] () -> TrackUniquePtrVariant {
    switch (type)
      {
      case Track::Type::Instrument:
        return InstrumentTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::Audio:
        return AudioTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::AudioBus:
        return AudioBusTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::AudioGroup:
        return AudioGroupTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::Midi:
        return MidiTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::MidiBus:
        return MidiBusTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::MidiGroup:
        return MidiGroupTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);

        break;
      case Track::Type::Folder:
        return FolderTrack::create_unique (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);
        break;
      case Track::Type::Master:
      case Track::Type::Chord:
      case Track::Type::Marker:
      case Track::Type::Modulator:
      default:
        throw std::runtime_error ("Track::create_unique: invalid track type");
        break;
      }
  }();
  std::visit ([&] (auto &track) { track->name_ = name; }, track_var);
  return track_var;
}
#endif

void
init_from (Track &obj, const Track &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track::UuidIdentifiableObject &> (obj),
    static_cast<const Track::UuidIdentifiableObject &> (other), clone_type);
  obj.pos_ = other.pos_;
  obj.type_ = other.type_;
  obj.name_ = other.name_;
  obj.icon_name_ = other.icon_name_;
  obj.visible_ = other.visible_;
  obj.filtered_ = other.filtered_;
  obj.main_height_ = other.main_height_;
  obj.enabled_ = other.enabled_;
  obj.color_ = other.color_;
  obj.in_signal_type_ = other.in_signal_type_;
  obj.out_signal_type_ = other.out_signal_type_;
  obj.comment_ = other.comment_;
  obj.bounce_ = other.bounce_;
  obj.bounce_to_master_ = other.bounce_to_master_;
  obj.frozen_clip_id_ = other.frozen_clip_id_;
  obj.disconnecting_ = other.disconnecting_;
}

utils::Utf8String
Track::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", get_name (), port.get_label ()));
}

bool
Track::is_auditioner () const
{
  return (tracklist_ != nullptr) && tracklist_->is_auditioner ();
}

Track::Type
Track::type_get_from_plugin_descriptor (
  const zrythm::plugins::PluginDescriptor &descr)
{
  if (descr.is_instrument ())
    return Track::Type::Instrument;
  else if (descr.is_midi_modifier ())
    return Track::Type::Midi;
  else
    return Track::Type::AudioBus;
}

void
Track::add_folder_parents (std::vector<FoldableTrack *> &parents, bool prepend)
  const
{
  for (
    const auto &cur_track_var :
    tracklist_->get_track_span ()
      | std::views::filter (
        TrackSpan::derived_from_type_projection<FoldableTrack>))
    {
      std::visit (
        [&] (const auto &cur_track) {
          using TrackT = base_type<decltype (cur_track)>;
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              /* last position covered by the foldable track cur_track */
              int last_covered_pos = cur_track->pos_ + (cur_track->size_ - 1);

              if (cur_track->pos_ < pos_ && pos_ <= last_covered_pos)
                {
                  if (prepend)
                    {
                      parents.insert (parents.begin (), cur_track);
                    }
                  else
                    {
                      parents.push_back (cur_track);
                    }
                }
            }
        },
        cur_track_var);
    }
}

void
Track::remove_from_folder_parents ()
{
  std::vector<FoldableTrack *> parents;
  add_folder_parents (parents, false);
  for (auto parent : parents)
    {
      parent->size_--;
    }
}

bool
Track::should_be_visible () const
{
  if (!visible_ || filtered_)
    return false;

  std::vector<FoldableTrack *> parents;
  add_folder_parents (parents, false);
  return std::ranges::all_of (parents, [] (const auto &parent) {
    return parent->visible_ && !parent->folded_;
  });
}

#if 0
static void
freeze_progress_close_cb (ExportData * data)
{
  g_thread_join (data->thread);

  Track * self = (Track *) g_ptr_array_index (data->tracks, 0);

  exporter_post_export (data->info, data->conns, data->state);

  /* assert exporting is finished */
  z_return_if_fail (!AUDIO_ENGINE->exporting);

  if (
    data->info->progress_info->get_completion_type ()
    == ProgressInfo::CompletionType::SUCCESS)
    {
      /* move the temporary file to the pool */
      GError *    err = NULL;
      FileAudioSource * clip = audio_clip_new_from_file (data->info->file_uri, &err);
      if (!clip)
        {
          HANDLE_ERROR (
            err, QObject::tr ("Failed creating audio clip from file at {}"),
            data->info->file_uri);
          return;
        }
      audio_pool_add_clip (AUDIO_POOL, clip);
      err = NULL;
      bool success =
        audio_clip_write_to_pool (clip, F_NO_PARTS, F_NOT_BACKUP, &err);
      if (!success)
        {
          HANDLE_ERROR (
            err, "Failed to write frozen audio for track '{}' to pool",
            self->name);
          return;
        }
      self->pool_id_ = clip->pool_id_;
    }

  if (g_file_test (data->info->file_uri, G_FILE_TEST_IS_REGULAR))
    {
      io_remove (data->info->file_uri);
    }

  self->frozen = true;
  // EVENTS_PUSH (EventType::ET_TRACK_FREEZE_CHANGED, self);
}

bool
track_freeze (Track * self, bool freeze, GError ** error)
{
  z_info ("{}freezing {}...", freeze ? "" : "un", self->name);

  if (freeze)
    {
      ExportSettings * info = export_settings_new ();
      ExportData *     data = export_data_new (nullptr, info);
      data->tracks = g_ptr_array_new ();
      g_ptr_array_add (data->tracks, self);
      self->bounce_to_master = true;
      track_mark_for_bounce (
        self, F_BOUNCE, F_MARK_REGIONS, F_NO_MARK_CHILDREN, F_NO_MARK_PARENTS);
      data->info->mode = Exporter::Mode::EXPORT_MODE_TRACKS;
      export_settings_set_bounce_defaults (
        data->info, Exporter::Format::WAV, "", self->name);

      data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

      /* start exporting in a new thread */
      data->thread = g_thread_new (
        "bounce_thread", (GThreadFunc) exporter_generic_export_thread,
        data->info);
      Exporter exporter (*info);
      data->thread = exporter.begin_generic_thread ();

      /* create a progress dialog and block */
      ExportProgressDialogWidget * progress_dialog =
        export_progress_dialog_widget_new (
          data, true, freeze_progress_close_cb, false, F_CANCELABLE);
      adw_dialog_present (
        ADW_DIALOG (progress_dialog), GTK_WIDGET (MAIN_WINDOW));
      return true;
    }
  else
    {
      /* FIXME */
      /*audio_pool_remove_clip (*/
      /*AUDIO_POOL, self->pool_id_, true);*/

      self->frozen = false;
      // EVENTS_PUSH (EventType::ET_TRACK_FREEZE_CHANGED, self);
    }

  return true;
}
#endif

void
Track::unselect_all ()
{
  if (is_auditioner ())
    return;

  std::vector<ArrangerObjectPtrVariant> objs;
  append_objects (objs);
  for (auto obj_var : objs)
    {
      std::visit (
        [&] (auto &&obj) {
          auto selection_mgr =
            arrangement::ArrangerObjectFactory::get_instance ()
              ->get_selection_manager_for_object (*obj);
          selection_mgr.remove_from_selection (obj->get_uuid ());
        },
        obj_var);
    }
}

void
Track::append_objects (std::vector<ArrangerObjectPtrVariant> &objs) const
{
  std::visit (
    [&] (auto &&self) {
      using TrackT = base_type<decltype (self)>;

      if constexpr (std::derived_from<TrackT, LanedTrack>)
        {
          for (auto &lane_var : self->lanes_)
            {
              using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
              auto lane = std::get<TrackLaneT *> (lane_var);
              std::ranges::copy (
                lane->get_children_view (), std::back_inserter (objs));
            }
        }

      if constexpr (std::is_same_v<TrackT, ChordTrack>)
        {
          std::ranges::copy (
            self->arrangement::template ArrangerObjectOwner<
              arrangement::ChordRegion>::get_children_view (),
            std::back_inserter (objs));
          std::ranges::copy (
            self->arrangement::template ArrangerObjectOwner<
              arrangement::ScaleObject>::get_children_view (),
            std::back_inserter (objs));
        }
      else if constexpr (std::is_same_v<TrackT, MarkerTrack>)
        {
          std::ranges::copy (
            self->get_children_view (), std::back_inserter (objs));
        }
      if constexpr (AutomatableTrack<TrackT>)
        {
          for (auto * at : self->automationTracklist ()->automation_tracks ())
            {
              std::ranges::copy (
                at->get_children_view (), std::back_inserter (objs));
            }
        }
    },
    convert_to_variant<TrackPtrVariant> (const_cast<Track *> (this)));
}

bool
Track::set_name_with_action_full (const utils::Utf8String &name)
{
  try
    {
      UNDO_MANAGER->perform (new gui::actions::RenameTrackAction (
        convert_to_variant<TrackPtrVariant> (this), *PORT_CONNECTIONS_MGR,
        name));
      return true;
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (QObject::tr ("Failed to rename track"));
      return false;
    }
}

void
Track::set_name_with_action (const utils::Utf8String &name)
{
  set_name_with_action_full (name);
}

void
Track::add_region_if_in_range (
  std::optional<signed_frame_t>                          p1,
  std::optional<signed_frame_t>                          p2,
  std::vector<arrangement::ArrangerObjectUuidReference> &regions,
  arrangement::ArrangerObjectUuidReference               region)
{
  if (!p1 && !p2)
    {
      regions.push_back (region);
      return;
    }

  std::visit (
    [&] (auto &&r) {
      if constexpr (arrangement::RegionObject<base_type<decltype (r)>>)
        {
          if (r->regionMixin ()->bounds ()->is_hit_by_range ({ *p1, *p2 }))
            {
              regions.push_back (region);
            }
        }
    },
    region.get_object ());
}

utils::Utf8String
Track::get_unique_name (const Tracklist &tracklist, const utils::Utf8String &name)
{
  auto new_name = name;
  while (!tracklist.track_name_is_unique (new_name, get_uuid ()))
    {
      auto [ending_num, name_without_num] = new_name.get_int_after_last_space ();
      if (ending_num == -1)
        {
          new_name += u8" 1";
        }
      else
        {
          new_name = utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("{} {}", name_without_num, ending_num + 1));
        }
    }
  return new_name;
}

void
Track::set_name (
  const Tracklist         &tracklist,
  const utils::Utf8String &name,
  bool                     pub_events)
{
  auto new_name = get_unique_name (tracklist, name);
  name_ = new_name;

  if (pub_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_NAME_CHANGED, this);
    }
}

void
Track::set_comment (const utils::Utf8String &comment, bool undoable)
{
  if (undoable)
    {
      tracklist_->get_selection_manager ().select_unique (get_uuid ());

      try
        {
          UNDO_MANAGER->perform (new gui::actions::EditTrackCommentAction (
            convert_to_variant<TrackPtrVariant> (this), comment));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to set track comment"));
          return;
        }
    }
  else
    {
      comment_ = comment;
    }
}

void
Track::set_color (const Color &color, bool undoable, bool fire_events)
{
  if (undoable)
    {
      tracklist_->get_selection_manager ().select_unique (get_uuid ());

      try
        {
          UNDO_MANAGER->perform (new gui::actions::EditTrackColorAction (
            convert_to_variant<TrackPtrVariant> (this), color));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to set track color"));
          return;
        }
    }
  else
    {
      color_ = color;

      if (fire_events)
        {
          // EVENTS_PUSH (EventType::ET_TRACK_COLOR_CHANGED, this);
        }
    }
}

void
Track::
  set_icon (const utils::Utf8String &icon_name, bool undoable, bool fire_events)
{
  if (undoable)
    {
      tracklist_->get_selection_manager ().select_unique (get_uuid ());

      try
        {
          UNDO_MANAGER->perform (new gui::actions::EditTrackIconAction (
            convert_to_variant<TrackPtrVariant> (this), icon_name));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Cannot set track icon"));
          return;
        }
    }
  else
    {
      icon_name_ = icon_name;

      if (fire_events)
        {
          // EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, this);
        }
    }
}

void
Track::set_enabled (
  bool enabled,
  bool trigger_undo,
  bool auto_select,
  bool fire_events)
{
  if (enabled_ == enabled)
    return;

  enabled_ = enabled;
  z_debug ("Setting track {} {}", name_, enabled_ ? "enabled" : "disabled");

  if (auto_select)
    {
      tracklist_->get_selection_manager ().select_unique (get_uuid ());
    }

  if (trigger_undo)
    {
      try
        {
          UNDO_MANAGER->perform (new gui::actions::EnableTrackAction (
            convert_to_variant<TrackPtrVariant> (this), enabled_));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Cannot set track enabled status"));
          return;
        }
    }
  else
    {
      enabled_ = enabled;

      if (fire_events)
        {
          std::visit (
            [this] (auto &&track) { Q_EMIT track->enabledChanged (enabled_); },
            convert_to_variant<TrackPtrVariant> (this));
        }
    }
}

int
Track::get_total_bars (
  const engine::session::Transport &transport,
  int                               total_bars) const
{
  const auto &tempo_map = PROJECT->get_tempo_map ();
  double      pos_ticks = static_cast<double> (
    tempo_map.musical_position_to_tick ({ .bar = total_bars }));

  std::vector<ArrangerObjectPtrVariant> objs;
  append_objects (objs);

  for (auto obj_var : objs)
    {
      auto end_pos_ticks = arrangement::ArrangerObjectSpan::
        end_position_ticks_with_start_position_fallback_projection (obj_var);
      pos_ticks = std::max (end_pos_ticks, pos_ticks);
    }

  int new_total_bars =
    tempo_map.tick_to_musical_position (static_cast<int64_t> (pos_ticks)).bar;
  return std::max (new_total_bars, total_bars);
}

void
Track::create_with_action (
  Type                                         type,
  const zrythm::plugins::PluginConfiguration * pl_setting,
  const FileDescriptor *                       file_descr,
  const Position *                             pos,
  int                                          index,
  int                                          num_tracks,
  int                                          disable_track_idx,
  TracksReadyCallback                          ready_cb)
{
  z_return_if_fail (num_tracks > 0);

  /* only support 1 track when using files */
  z_return_if_fail (file_descr == nullptr || num_tracks == 1);

  if (file_descr != nullptr)
    {
      TRACKLIST->import_files (
        std::nullopt, file_descr, nullptr, nullptr, index, pos, ready_cb);
    }
  else
    {
      UNDO_MANAGER->perform (new gui::actions::CreateTracksAction (
        type, pl_setting, file_descr, index, pos, num_tracks,
        disable_track_idx));
    }
}

Track *
Track::create_empty_at_idx_with_action (Type type, int index)
{
  return create_without_file_with_action (type, nullptr, index);
}

Track *
Track::create_empty_with_action (Type type)
{
  return create_empty_at_idx_with_action (type, TRACKLIST->track_count ());
}

Track *
Track::create_for_plugin_at_idx_w_action (
  Type                                         type,
  const zrythm::plugins::PluginConfiguration * pl_setting,
  int                                          index)
{
  return create_without_file_with_action (type, pl_setting, index);
}

Track *
Track::create_without_file_with_action (
  Type                                         type,
  const zrythm::plugins::PluginConfiguration * pl_setting,
  int                                          index)
{
  /* this may throw, and if it does we don't care - caller is expected to catch
   * it */
  create_with_action (type, pl_setting, nullptr, nullptr, index, 1, -1, nullptr);

  auto track = TRACKLIST->get_track_at_index (index);
  return Track::from_variant (track);
}

void
Track::set_caches (CacheType types)
{
  if (
    ENUM_BITSET_TEST (types, CacheType::PlaybackSnapshots) && !is_auditioner ())
    {
      z_return_if_fail (AUDIO_ENGINE->run_.load () == false);

      set_playback_caches ();
    }

  if (ENUM_BITSET_TEST (types, CacheType::PluginPorts))
    {
      if (auto channel_track = dynamic_cast<ChannelTrack *> (this))
        {
          channel_track->get_channel ()->set_caches ();
        }
    }

  if (
    ENUM_BITSET_TEST (types, CacheType::AutomationLaneRecordModes)
    || ENUM_BITSET_TEST (types, CacheType::AutomationLanePorts))
    {
// TODO
#if 0
      if (auto automatable_track = dynamic_cast<AutomatableTrack *> (this))
        {
          automatable_track->get_automation_tracklist ().set_caches (
            CacheType::AutomationLaneRecordModes
            | CacheType::AutomationLanePorts);
        }
#endif
    }
}

#if 0
GMenu *
Track::generate_edit_context_menu (int num_selected)
{
  GMenu *     edit_submenu = g_menu_new ();
  GMenuItem * menuitem;

  if (type_is_copyable (type_))
    {
      char * str;
      /* delete track */
      if (num_selected == 1)
        str = g_strdup (QObject::tr ("_Delete Track"));
      else
        str = g_strdup (QObject::tr ("_Delete Tracks"));
      menuitem = g_menu_item_new (str, "app.delete-selected-tracks");
      g_menu_item_set_attribute (
        menuitem, "verb-icon", "s",
        "gnome-icon-library-user-trash-full-symbolic");
      g_free (str);
      g_menu_append_item (edit_submenu, menuitem);

      /* duplicate track */
      if (num_selected == 1)
        str = g_strdup (QObject::tr ("Duplicate Track"));
      else
        str = g_strdup (QObject::tr ("Duplicate Tracks"));
      menuitem = g_menu_item_new (str, "app.duplicate-selected-tracks");
      g_menu_item_set_attribute (
        menuitem, "verb-icon", "s", "gnome-icon-library-copy-symbolic");
      g_free (str);
      g_menu_append_item (edit_submenu, menuitem);
    }

    /* add regions TODO */
#  if 0
  if (track->type == Track::Type::INSTRUMENT)
    {
      menuitem = g_menu_item_new (
        _("Add Region"), "app.add-region");
      g_menu_item_set_attribute (menuitem, "verb-icon", "s", "add");
      g_menu_append_item (edit_submenu, menuitem);
    }
#  endif

  menuitem = g_menu_item_new (
    num_selected == 1 ? QObject::tr ("Hide Track") : QObject::tr ("Hide Tracks"),
    "app.hide-selected-tracks");
  g_menu_item_set_attribute (
    menuitem, "verb-icon", "s", "gnome-icon-library-eye-not-looking-symbolic");
  g_menu_append_item (edit_submenu, menuitem);

  menuitem = g_menu_item_new (
    num_selected == 1 ? QObject::tr ("Pin/Unpin Track") : QObject::tr ("Pin/Unpin Tracks"),
    "app.pin-selected-tracks");
  g_menu_item_set_attribute (
    menuitem, "verb-icon", "s", "gnome-icon-library-pin-symbolic");
  g_menu_append_item (edit_submenu, menuitem);

  menuitem = g_menu_item_new (QObject::tr ("Change Color..."), "app.change-track-color");
  g_menu_item_set_attribute (
    menuitem, "verb-icon", "s", "gnome-icon-library-color-picker-symbolic");
  g_menu_append_item (edit_submenu, menuitem);

  if (num_selected == 1)
    {
      menuitem = g_menu_item_new (QObject::tr ("Rename..."), "app.rename-track");
      g_menu_item_set_attribute (
        menuitem, "verb-icon", "s", "gnome-icon-library-text-insert-symbolic");
      g_menu_append_item (edit_submenu, menuitem);
    }

  return edit_submenu;
}
#endif

void
from_json (const nlohmann::json &j, Track &track)
{
  from_json (j, static_cast<Track::UuidIdentifiableObject &> (track));
  j.at (Track::kTypeKey).get_to (track.type_);
  j.at (Track::kNameKey).get_to (track.name_);
  j.at (Track::kIconNameKey).get_to (track.icon_name_);
  j.at (Track::kIndexKey).get_to (track.pos_);
  j.at (Track::kVisibleKey).get_to (track.visible_);
  j.at (Track::kMainHeightKey).get_to (track.main_height_);
  j.at (Track::kEnabledKey).get_to (track.enabled_);
  j.at (Track::kColorKey).get_to (track.color_);
  j.at (Track::kInputSignalTypeKey).get_to (track.in_signal_type_);
  j.at (Track::kOutputSignalTypeKey).get_to (track.out_signal_type_);
  j.at (Track::kCommentKey).get_to (track.comment_);
  // TODO
  // j.at (Track::kFrozenClipIdKey).get_to (track.frozen_clip_id_);
}
}
