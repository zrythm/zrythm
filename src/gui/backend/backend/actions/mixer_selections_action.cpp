// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/backend/ui.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/plugin_span.h"
#include "gui/dsp/router.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/logger.h"

using namespace zrythm::gui::actions;

MixerSelectionsAction::MixerSelectionsAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::MixerSelections)
{
}

MixerSelectionsAction::MixerSelectionsAction (
  std::optional<PluginSpan>                      plugins,
  const PortConnectionsManager *                 connections_mgr,
  Type                                           type,
  std::optional<Track::TrackUuid>                to_track_id,
  std::optional<dsp::PluginSlot>                 to_slot,
  const PluginSetting *                          setting,
  int                                            num_plugins,
  int                                            new_val,
  zrythm::gui::old_dsp::plugins::CarlaBridgeMode new_bridge_mode,
  QObject *                                      parent)
    : MixerSelectionsAction (parent)

{
  mixer_selections_action_type_ = type;
  to_slot_ = to_slot;
  to_track_uuid_ = to_track_id;
  new_channel_ = !to_track_id.has_value ();
  num_plugins_ = num_plugins;
  new_val_ = new_val;
  new_bridge_mode_ = new_bridge_mode;
  if (setting)
    {
      setting_ = setting->clone_unique ();
      setting_->validate ();
    }

  if (plugins)
    {
      ms_before_ = plugins->create_snapshots (*this);

      /* clone the automation tracks */
      clone_ats (PluginSpan{ *ms_before_ }, false, 0);
    }

  if (connections_mgr)
    port_connections_before_ = connections_mgr->clone_unique ();
}

void
MixerSelectionsAction::init_loaded_impl ()
{
  if (ms_before_)
    {
      PluginSpan{ *ms_before_ }.init_loaded (nullptr);
    }
  if (deleted_ms_)
    {
      PluginSpan{ *deleted_ms_ }.init_loaded (nullptr);
    }

  for (auto &at : ats_)
    {
      at->init_loaded ();
    }
  for (auto &at : deleted_ats_)
    {
      at->init_loaded ();
    }
}

void
MixerSelectionsAction::init_after_cloning (
  const MixerSelectionsAction &other,
  ObjectCloneType              clone_type)
{
  UndoableAction::copy_members_from (other, clone_type);
  mixer_selections_action_type_ = other.mixer_selections_action_type_;
  to_slot_ = other.to_slot_;
  to_track_uuid_ = other.to_track_uuid_;
  new_channel_ = other.new_channel_;
  num_plugins_ = other.num_plugins_;
  new_val_ = other.new_val_;
  new_bridge_mode_ = other.new_bridge_mode_;
  if (other.setting_)
    setting_ = other.setting_->clone_unique ();
  // TODO
#if 0
    if (other.ms_before_)
    ms_before_ = other.ms_before_->clone_unique ();
  if (other.deleted_ms_)
    deleted_ms_ = other.deleted_ms_->clone_unique ();
  clone_unique_ptr_container (deleted_ats_, other.deleted_ats_);
  clone_unique_ptr_container (ats_, other.ats_);
#endif
}

void
MixerSelectionsAction::clone_ats (PluginSpan plugins, bool deleted, int start_slot)
{
  const auto &plugin_span = plugins;
  auto        first_pl_var = plugin_span.front ();
  auto        track_var = std::visit (
    [&] (auto &&first_pl) { return first_pl->get_track (); }, first_pl_var);
  z_return_if_fail (track_var);
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, AutomatableTrack>)
        {
          z_debug ("cloning automation tracks for track {}", track->get_name ());
          auto &atl = track->automation_tracklist_;
          int   count = 0;
          int   regions_count = 0;
          for (const auto &pl_var : plugin_span)
            {
              std::visit (
                [&] (auto &&pl) {
                  const auto slot = *pl->get_slot ();
                  for (const auto &at : atl->get_automation_tracks ())
                    {
                      const auto &port_id = at->get_port ().id_;
                      if (
                        port_id->owner_type_
                        != dsp::PortIdentifier::OwnerType::Plugin)
                        continue;

                      auto plugin_uuid = port_id->get_plugin_id ();
                      z_return_if_fail (
                        plugin_uuid.has_value ()) auto plugin_var =
                        PROJECT->find_plugin_by_id (plugin_uuid.value ());
                      z_return_if_fail (plugin_var.has_value ());
                      const auto pl_slot = std::visit (
                        [] (auto &&p) { return *p->get_slot (); },
                        plugin_var.value ());
                      if (pl_slot != slot)
                        continue;

// TODO
#if 0
                  if (deleted)
                    {
                      deleted_ats_.emplace_back (at->clone_unique ());
                    }
                  else
                    {
                      ats_.emplace_back (at->clone_unique ());
                    }
#endif
                      count++;
                      regions_count += at->get_children_vector ().size ();
                    }
                },
                pl_var);
            }
          z_debug (
            "cloned {} automation tracks for track {}, total regions {}", count,
            track->get_name (), regions_count);
        }
    },
    *track_var);
}

void
MixerSelectionsAction::copy_at_regions (
  AutomationTrack       &dest,
  const AutomationTrack &src)
{
  // TODO
#if 0
  dest.region_list_->regions_.clear ();
  dest.region_list_->regions_.reserve (src.region_list_->regions_.size ());

  src.foreach_region ([&] (auto &src_r) {
    auto dest_region = src_r.clone_raw_ptr ();
    dest_region->set_automation_track (dest);
    dest_region->setParent (&dest);
    dest.region_list_->regions_.push_back (dest_region);
  });

  if (!dest.region_list_->regions_.empty ())
  {
    z_debug (
      "reverted {} regions for automation track {}:",
      dest.region_list_->regions_.size (), dest.index_);
    }
#endif
}

void
MixerSelectionsAction::revert_automation (
  AutomatableTrack &track,
  dsp::PluginSlot   slot,
  bool              deleted)
{
  z_debug ("reverting automation for {}#{}", track.get_name (), slot);

  auto &atl = track.automation_tracklist_;
  auto &ats = deleted ? deleted_ats_ : ats_;
  int   num_reverted_ats = 0;
  int   num_reverted_regions = 0;
  for (auto &cloned_at : ats)
    {
      auto port_var = PROJECT->find_port_by_id (cloned_at->port_id_);
      z_return_if_fail (port_var.has_value ());
      const auto &port_id =
        std::visit ([] (auto &&p) { return *p->id_; }, port_var.value ());
      auto plugin_uuid = port_id.get_plugin_id ();
      z_return_if_fail (plugin_uuid.has_value ());
      auto plugin_var = PROJECT->find_plugin_by_id (plugin_uuid.value ());
      z_return_if_fail (plugin_var.has_value ());
      const auto plugin_slot = PluginSpan::slot_projection (*plugin_var);
      ;
      if (plugin_slot != slot)
        {
          continue;
        }

      /* find corresponding automation track in track and copy regions */
      auto actual_at =
        atl->get_plugin_at (slot, port_id.port_index_, port_id.get_symbol ());

      copy_at_regions (*actual_at, *cloned_at);
      num_reverted_regions += actual_at->get_children_vector ().size ();
      num_reverted_ats++;
    }

  z_debug (
    "reverted %d automation tracks and %d regions", num_reverted_ats,
    num_reverted_regions);
}

void
MixerSelectionsAction::save_existing_plugin (
  std::vector<PluginPtrVariant> tmp_plugins,
  Track *                       from_tr,
  dsp::PluginSlot               from_slot,
  Track *                       to_tr,
  dsp::PluginSlot               to_slot)
{
  auto to_track_var = convert_to_variant<TrackPtrVariant> (to_tr);
  std::visit (
    [&] (auto &&to_track) {
      using ToTrackT = base_type<decltype (to_track)>;
      if constexpr (std::derived_from<ToTrackT, AutomatableTrack>)
        {
          auto existing_pl = to_track->get_plugin_at_slot (to_slot);
          z_debug (
            "existing plugin at ({}:{} => {}:{}): {}",
            from_tr ? from_tr->get_name () : u8"(none)", from_slot,
            to_tr ? to_tr->get_name () : u8"(none)", to_slot,
            existing_pl
              ? old_dsp::plugins::Plugin::name_projection (existing_pl.value ())
              : u8"(none)");
          if (existing_pl && (from_tr != to_tr || from_slot != to_slot))
            {
              tmp_plugins.push_back (existing_pl.value ());
              clone_ats (
                PluginSpan{ tmp_plugins }, true, tmp_plugins.size () - 1);
            }
          else
            {
              z_info (
                "skipping saving slot and cloning "
                "automation tracks - same slot");
            }
        }
    },
    to_track_var);
}

void
MixerSelectionsAction::revert_deleted_plugin (
  Track          &to_tr_base,
  dsp::PluginSlot to_slot)
{
  std::visit (
    [&] (auto &&to_tr) {
      using TrackT = base_type<decltype (to_tr)>;
      if (!deleted_ms_)
        {
          z_debug (
            "No deleted plugin to revert at {}#{}", to_tr->get_name (), to_slot);
          return;
        }

      z_debug ("reverting deleted plugin at {}#{}", to_tr->get_name (), to_slot);

      auto deleted_type =
        PluginSpan{ *deleted_ms_ }.get_slot_type_of_first_plugin ();
      if (deleted_type == dsp::PluginSlotType::Modulator)
        {
          /* modulators are never replaced */
          return;
        }

      for (const auto &deleted_pl_var : PluginSpan{ *deleted_ms_ })
        {
          std::visit (
            [&] (auto &&deleted_pl) {
              if constexpr (std::derived_from<TrackT, AutomationTrack>)
                {
                  const auto slot_to_revert = deleted_pl->get_slot ();
                  if (slot_to_revert != to_slot)
                    {
                      return;
                    }

                  z_debug (
                    "reverting plugin {} in slot {}", deleted_pl->get_name (),
                    slot_to_revert);

                  /* add to channel - note: cloning deleted_pl also instantiates
                   * the clone */
                  auto added_pl = to_tr->insert_plugin (
                    deleted_pl->get_uuid (), slot_to_revert, true, true, false,
                    false, true, false, false);

                  /* bring back automation */
                  revert_automation (*to_tr, *deleted_ms_, slot_to_revert, true);

                  /* activate and set visibility */
                  added_pl->activate (true);

                  /* show if was visible before */
                  if (ZRYTHM_HAVE_UI && deleted_pl->visible_)
                    {
                      added_pl->visible_ = true;
                      /* EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                       * added_pl); */
                    }
                }
            },
            deleted_pl_var);
        }
    },
    convert_to_variant<TrackPtrVariant> (&to_tr_base));
}

void
MixerSelectionsAction::do_or_undo_create_or_delete (bool do_it, bool create)
{
  auto track_var = *TRACKLIST->get_track (
    create
      ? to_track_uuid_.value ()
      : PluginSpan::track_id_projection (PluginSpan{ *ms_before_ }.front ()));

  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, AutomatableTrack>)
        {
#if 0
          auto * ch =
            track->has_channel ()
              ? dynamic_cast<ChannelTrack *> (track)->channel_
              : nullptr;
#endif
          auto own_ms = PluginSpan{ *ms_before_ };
          auto slot_type =
            create
              ? (to_slot_->has_slot_index ()
                   ? to_slot_->get_slot_with_index ().first
                   : to_slot_->get_slot_type_only ())
              : own_ms.get_slot_type_of_first_plugin ();
          int loop_times =
            create && mixer_selections_action_type_ != Type::Paste
              ? num_plugins_
              : own_ms.size ();
          bool delete_ = !create;

          auto get_slot = [&] (const auto i) {
            if (create)
              {
                int to_slot_i =
                  to_slot_->has_slot_index ()
                    ? to_slot_->get_slot_with_index ().second
                    : 0;
                return dsp::PluginSlot (slot_type, to_slot_i + i);
              }

            return PluginSpan::slot_projection (own_ms[i]);
          };

          /* if creating plugins (create do or delete undo) */
          if ((create && do_it) || (delete_ && !do_it))
            {
              /* clear deleted caches */
              deleted_ats_.clear ();
              deleted_ms_.emplace (std::vector<PluginPtrVariant> ());

              for (const auto i : std::views::iota (0, loop_times))
                {
                  auto slot = get_slot (i);
                  auto own_pl_var = own_ms[i];

                  /* create new plugin */
                  auto pl_ref_id = [&] () {
                    if (create)
                      {
                        std::optional<PluginUuidReference> pl_ref;
                        if (mixer_selections_action_type_ == Type::Paste)
                          {
                            pl_ref = std::visit (
                              [&] (auto &&own_pl) {
                                return PROJECT->get_plugin_registry ()
                                  .clone_object (*own_pl);
                              },
                              own_pl_var);
                          }
                        else
                          {
                            pl_ref =
                              PROJECT->getPluginFactory ()
                                ->create_plugin_from_setting (*setting_);
                          }

                        /* instantiate so that ports are created */
                        std::visit (
                          [&] (auto &&pl) { pl->instantiate (); },
                          pl_ref->get_object ());
                        return *pl_ref;
                      }
                    if (delete_)
                      {
                        /* note: this also instantiates the plugin */
                        return std::visit (
                          [&] (auto &&own_pl) {
                            return PROJECT->get_plugin_registry ()
                              .clone_object (*own_pl);
                          },
                          own_pl_var);
                      }
                    throw std::runtime_error ("should not be reached");
                  }();

                  std::visit (
                    [&] (auto &&pl) {
                      /* validate */
                      if (delete_)
                        {
                          assert (
                            slot == PluginSpan::slot_projection (own_pl_var));
                        }

                      /* set track */
                      // pl->track_ = track;
                      pl->set_track (track->get_uuid ());

                      /* save any plugin about to be deleted */
                      // FIXME: what is the point of from_slot?
                      save_existing_plugin (
                        *deleted_ms_, nullptr, dsp::PluginSlot (slot_type, -1),
                        slot_type == zrythm::dsp::PluginSlotType::Modulator
                          ? (Track *) P_MODULATOR_TRACK
                          : track,
                        slot);

                      /* add to destination */
                      // FIXME: danging pl
                      track->insert_plugin (
                        pl->get_uuid (), slot, true, false, false, false, true,
                        false, false);

                      /* select the plugin */
                      pl->set_selected (true);

                      /* set visibility */
                      if (create)
                        {
                          /* set visible from settings */
                          pl->visible_ =
                            ZRYTHM_HAVE_UI
                            && gui::SettingsManager::openPluginsOnInstantiation ();
                        }
                      else if (delete_)
                        {
                          /* set visible if plugin was visible before deletion */
                          pl->visible_ =
                            ZRYTHM_HAVE_UI
                            && PluginSpan::visible_projection (own_pl_var);
                        }
                      /* EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                       * added_pl); */

                      /* activate */
                      pl->activate (true);
                    },
                    pl_ref_id.get_object ());
                }

              /* if undoing deletion */
              if (delete_)
                {
                  for (
                    const auto &pl_var : own_ms | std::views::take (loop_times))
                    {
                      std::visit (
                        [&] (auto &&pl) {
                          /* restore port connections */
                          z_debug (
                            "restoring custom connections for plugin '{}'",
                            pl->get_name ());
                          std::vector<Port *> ports;
                          pl->append_ports (ports);
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

                          /* copy automation from before deletion */
                          if constexpr (
                            std::derived_from<TrackT, AutomatableTrack>)
                            {
                              revert_automation (
                                *track, *pl->get_slot (), false);
                            }
                        },
                        pl_var);
                    }
                }

              track->validate ();

              /* EVENTS_PUSH (EventType::ET_PLUGINS_ADDED, track); */
            }
          /* else if deleting plugins (create undo or delete do) */
          else
            {
              for (const auto i : std::views::iota (0, loop_times))
                {
                  auto slot = get_slot (i);

                  /* if doing deletion, remember port metadata */
                  if (do_it)
                    {
                      auto own_pl_var = own_ms[i];
                      auto prj_pl_var = track->get_plugin_at_slot (slot);

                      /* remember any custom connections */
                      std::visit (
                        [&] (auto &&own_pl, auto &&prj_pl) {
                          z_debug (
                            "remembering custom connections for plugin '{}'",
                            own_pl->get_name ());
                          std::vector<Port *> ports, own_ports;
                          prj_pl->append_ports (ports);
                          own_pl->append_ports (own_ports);
                          for (auto * prj_port : ports)
                            {
                              auto it = std::find_if (
                                own_ports.begin (), own_ports.end (),
                                [&prj_port] (auto own_port) {
                                  return own_port->id_ == prj_port->id_;
                                });
                              z_return_if_fail (it != own_ports.end ());

                              (*it)->copy_metadata_from_project (*prj_port);
                            }
                        },
                        own_pl_var, *prj_pl_var);
                    }

                  /* remove the plugin at given slot */
                  if constexpr (TrackWithPlugins<TrackT>)
                    {
                      track->remove_plugin (slot);
                    }

                  /* if there was a plugin at the slot before, bring it back */
                  revert_deleted_plugin (*track, slot);
                }

              /* EVENTS_PUSH (EventType::ET_PLUGINS_REMOVED, nullptr); */
            }

          /* restore connections */
          save_or_load_port_connections (do_it);

          ROUTER->recalc_graph (false);
        }
    },
    track_var);
}

void
MixerSelectionsAction::do_or_undo_change_status (bool do_it)
{
  auto ms = PluginSpan{ *ms_before_ };
  auto track_var =
    TRACKLIST->get_track (PluginSpan::track_id_projection (ms.front ()));
  z_return_if_fail (track_var.has_value ());

  std::visit (
    [&] (auto &&track) {
      for (const auto &own_pl_var : ms)
        {
          std::visit (
            [&] (auto &&own_pl) {
              auto project_pl_var =
                PROJECT->find_plugin_by_id (own_pl->get_uuid ());
              std::visit (
                [&] (auto &&project_pl) {
                  project_pl->set_enabled (
                    do_it ? new_val_ : own_pl->is_enabled (false), true);
                },
                *project_pl_var);
            },
            own_pl_var);
        }
    },
    track_var.value ());
}

void
MixerSelectionsAction::do_or_undo_change_load_behavior (bool do_it)
{
  auto ms = PluginSpan{ *ms_before_ };
  auto track_var = TRACKLIST->get_track (ms.get_track_id_of_first_plugin ());

  std::visit (
    [&] (auto &&track) {
#if 0
      auto ch =
        track->has_channel ()
          ? dynamic_cast<ChannelTrack *> (track)->channel_
          : nullptr;
#endif
      for (const auto &own_pl_var : ms)
        {
          std::visit (
            [&] (auto &&own_pl) {
              auto pl_var = PROJECT->find_plugin_by_id (own_pl->get_uuid ());
              std::visit (
                [&] (auto &&pl) {
                  pl->setting_->bridge_mode_ =
                    do_it ? new_bridge_mode_ : own_pl->setting_->bridge_mode_;

            /* TODO - below is tricky */
#if 0
      carla_set_engine_option (
        pl->carla->host_handle, ENGINE_OPTION_PREFER_UI_BRIDGES, false, nullptr);
      carla_set_engine_option (
        pl->carla->host_handle, ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, false, nullptr);

      switch (pl->setting_->bridge_mode_)
        {
        case zrythm::gui::old_dsp::plugins::CarlaBridgeMode::Full:
          carla_set_engine_option (
            pl->carla->host_handle, ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, true, nullptr);
          break;
        case zrythm::gui::old_dsp::plugins::CarlaBridgeMode::UI:
          carla_set_engine_option (
            pl->carla->host_handle, ENGINE_OPTION_PREFER_UI_BRIDGES, true, nullptr);
          break;
        default:
          break;
        }

      pl->activate (false);
      carla_remove_plugin (
        pl->carla->host_handle, 0);
      carla_native_plugin_add_internal_plugin_from_descr (
        pl->carla, pl->setting_->descr);
      pl->activate (true);
#endif
                },
                *pl_var);
            },
            own_pl_var);
        }

      if (ZRYTHM_HAVE_UI)
        {
// TODO
#if 0
      ui_show_error_message (
        QObject::tr ("Project Reload Needed"),
        QObject::tr (
          "Plugin load behavior changes will only take effect after you save and re-load the project"));
#endif
        }
    },
    track_var.value ());
}

void
MixerSelectionsAction::do_or_undo_move_or_copy (bool do_it, bool copy)
{
  auto       own_ms = PluginSpan{ *ms_before_ };
  const auto to_slot_value = to_slot_.value ();
  auto       from_tr_var =
    TRACKLIST->get_track (own_ms.get_track_id_of_first_plugin ());
  z_return_if_fail (from_tr_var.has_value ());
  const bool move = !copy;

  std::visit (
    [&] (auto &&from_tr) {
      using FromTrackT = base_type<decltype (from_tr)>;
      if constexpr (std::derived_from<FromTrackT, AutomatableTrack>)
        {
          if (do_it)
            {
              OptionalTrackPtrVariant to_tr_var;

              if (new_channel_)
                {
                  /* get the own plugin */
                  auto own_pl_var = own_ms.front ();

                  /* add a new track to the tracklist */
                  std::visit (
                    [&] (auto &&own_pl) {
                      const auto str =
                        utils::Utf8String::from_utf8_encoded_string (
                          fmt::format ("{} (Copy)", own_pl->get_name ()));
                      auto to_tr_unique_var = Track::create_track (
                        Track::Type::AudioBus, str, TRACKLIST->track_count ());
                      std::visit (
                        [&] (auto &&to_tr_unique) {
                          PROJECT->get_track_registry ().register_object (
                            *to_tr_unique);
                          TRACKLIST->append_track (
                            TrackUuidReference{
                              to_tr_unique->get_uuid (),
                              PROJECT->get_track_registry () },
                            *AUDIO_ENGINE, false, false);

                          /* remember to track pos */
                          to_track_uuid_ = to_tr_unique->get_uuid ();

                          to_tr_var = to_tr_unique.release ();
                        },
                        to_tr_unique_var);
                    },
                    own_pl_var);
                }
              /* else if not new track/channel */
              else
                {
                  to_tr_var =
                    TRACKLIST->get_track (to_track_uuid_.value ()).value ();
                }

              std::visit (
                [&] (auto &&to_tr) {
                  using ToTrackT = base_type<decltype (to_tr)>;
                  if constexpr (std::derived_from<ToTrackT, AutomatableTrack>)
                    {
                      TRACKLIST->get_track_span ().deselect_all_plugins ();

                      /* sort own selections */
                      auto sorted_own_ms = std::ranges::to<std::vector> (own_ms);
                      std::ranges::sort (
                        sorted_own_ms, {}, PluginSpan::slot_projection);

                      const bool move_downwards_same_track =
                        Track::from_variant (to_tr_var.value ())->get_uuid ()
                          == from_tr->get_uuid ()
                        && !own_ms.empty ()
                        && to_slot_
                             > PluginSpan::slot_projection (own_ms.front ());

                      /* clear deleted caches */
                      deleted_ats_.clear ();
                      deleted_ms_.emplace (std::vector<PluginPtrVariant> ());

                      /* foreach slot */
                      for (
                        int i =
                          move_downwards_same_track ? own_ms.size () - 1 : 0;
                        move_downwards_same_track
                          ? (i >= 0)
                          : (i < (int) own_ms.size ());
                        move_downwards_same_track ? --i : ++i)
                        {
                          /* get/create the actual plugin */
                          auto from_slot =
                            PluginSpan::slot_projection (sorted_own_ms.at (i));
                          auto pl_var = [&] () {
                            if (move)
                              {
                                return from_tr->get_plugin_at_slot (from_slot)
                                  .value ();
                              }

                            return std::visit (
                              [&] (auto &&own_pl) {
                                // FIXME new plugin gets deleted
                                return PROJECT->get_plugin_registry ()
                                  .clone_object (*own_pl)
                                  .get_object ();
                              },
                              sorted_own_ms.at (i));
                          }();

                          auto to_slot = to_slot_value.get_slot_after_n (i);

                          /* save any plugin about to be deleted */
                          save_existing_plugin (
                            *deleted_ms_, from_tr, from_slot, to_tr, to_slot);

                          /* move or copy the plugin */
                          if (move)
                            {
                              z_debug (
                                "moving plugin from {}:{} to {}:{}",
                                from_tr->get_name (), from_slot,
                                to_tr->get_name (), to_slot);

                              if (
                                from_tr->get_uuid () != to_tr->get_uuid ()
                                || from_slot != to_slot)
                                {
                                  std::visit (
                                    [&] (auto &&pl) {
                                      pl->move (to_tr, to_slot, false, false);
                                    },
                                    pl_var);
                                }
                            }
                          else if (copy)
                            {
                              z_debug (
                                "copying plugin from {}:{} to {}:{}",
                                from_tr->get_name (), from_slot,
                                to_tr->get_name (), to_slot);

                              to_tr->insert_plugin (
                                PluginSpan::uuid_projection (pl_var), to_slot,
                                true, false, false, false, true, false, false);

                              std::visit (
                                [&] (auto &&pl, auto &&own_pl) {
                                  z_return_if_fail (
                                    pl->in_ports_.size ()
                                    == own_pl->in_ports_.size ());

                                  /* copy automation regions from original
                                   * plugin */

                                  const auto &prev_pl = own_pl;
                                  auto       &prev_atl =
                                    from_tr->get_automation_tracklist ();
                                  auto &atl = to_tr->get_automation_tracklist ();
                                  for (
                                    const auto &[prev_port, new_port] :
                                    std::views::zip (
                                      prev_pl->get_input_port_span ()
                                        .template get_elements_by_type<
                                          ControlPort> (),
                                      pl->get_input_port_span ()
                                        .template get_elements_by_type<
                                          ControlPort> ()))
                                    {
                                      auto * prev_at =
                                        prev_atl.get_automation_track_by_port_id (
                                          prev_port->get_uuid ());
                                      auto * new_at =
                                        atl.get_automation_track_by_port_id (
                                          new_port->get_uuid ());
                                      if (prev_at && new_at)
                                        {
                                    // TODO
#if 0
                                          prev_at->foreach_region (
                                            [&] (auto &prev_region) {
                                              to_tr->Track::add_region (
                                                prev_region.clone_raw_ptr (),
                                                new_at, std::nullopt, false);
                                              });
#endif
                                        }
                                    }
                                },
                                pl_var, sorted_own_ms.at (i));
                            }

                          std::visit (
                            [&] (auto &&pl) {
                              pl->set_selected (true);

                              /* if new plugin (copy), instantiate it, activate
                               * it and set visibility */
                              if (copy)
                                {
                                  pl->activate (true);

                                  /* show if was visible before */
                                  if (
                                    ZRYTHM_HAVE_UI
                                    && PluginSpan::visible_projection (
                                      sorted_own_ms.at (i)))
                                    {
                                      pl->visible_ = true;
                                      /* EVENTS_PUSH
                                       * (EventType::ET_PLUGIN_VISIBILITY_CHANGED,
                                       * pl); */
                                    }
                                }
                            },
                            pl_var);
                        }

                      to_tr->validate ();

                      if (new_channel_)
                        {
                          /* EVENTS_PUSH (EventType::ET_TRACKS_ADDED, nullptr); */
                        }

                      /* EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED,
                       * to_ch);
                       */
                    }
                },
                to_tr_var.value ());
            }
          /* else if undoing (deleting/moving plugins back) */
          else
            {
              auto to_tr_var = TRACKLIST->get_track (to_track_uuid_.value ());
              z_return_if_fail (to_tr_var.has_value ());
              std::visit (
                [&] (auto &&to_tr) {
                  using ToTrackT = base_type<decltype (to_tr)>;
                  if constexpr (std::derived_from<ToTrackT, AutomatableTrack>)
                    {
#if 0
                      [[maybe_unused]] auto * to_ch =
                        to_tr->has_channel ()
                          ? dynamic_cast<ChannelTrack *> (to_tr)->channel_
                          : nullptr;
#endif

                      /* clear selections to readd each original plugin */
                      TRACKLIST->get_track_span ().deselect_all_plugins ();

                      /* sort own selections */

                      /* sort own selections */
                      auto sorted_own_ms = std::ranges::to<std::vector> (own_ms);
                      std::ranges::sort (
                        sorted_own_ms, {}, PluginSpan::slot_projection);

                      bool move_downwards_same_track =
                        to_tr->get_uuid () == from_tr->get_uuid ()
                        && !own_ms.empty ()
                        && to_slot_ < PluginSpan::slot_projection (
                             sorted_own_ms.front ());
                      for (
                        int i =
                          move_downwards_same_track ? sorted_own_ms.size () - 1 : 0;
                        move_downwards_same_track
                          ? (i >= 0)
                          : (i < (int) sorted_own_ms.size ());
                        move_downwards_same_track ? i-- : i++)
                        {
                          /* get the actual plugin */
                          auto to_slot = to_slot_value.get_slot_after_n (i);
                          auto pl_var = to_tr->get_plugin_at_slot (to_slot);
                          z_return_if_fail (pl_var.has_value ());

                          auto own_pl_var = sorted_own_ms.at (i);

                          std::visit (
                            [&] (auto &pl, auto &&own_pl) {
                              /* original slot */
                              const auto from_slot = *own_pl->get_slot ();
                              auto       existing_pl_var =
                                from_tr->get_plugin_at_slot (from_slot);

                              /* if moving plugins back */
                              if (move)
                                {
                                  /* move plugin to its original
                                   * slot */
                                  z_debug (
                                    "moving plugin back from {}:{} to {}:{}",
                                    to_tr->get_name (), to_slot,
                                    from_tr->get_name (), from_slot);

                                  if (
                                    from_tr->get_uuid () != to_tr->get_uuid ()
                                    || from_slot != to_slot)
                                    {
                                      {
                                        z_return_if_fail (
                                          !existing_pl_var.has_value ());
                                      }
                                      if constexpr (
                                        std::derived_from<
                                          FromTrackT, AutomatableTrack>)
                                        {
                                          pl->move (
                                            from_tr, from_slot, false, false);
                                        }
                                    }
                                }
                              else if (copy)
                                {
                                  if constexpr (TrackWithPlugins<ToTrackT>)
                                    {
                                      to_tr->remove_plugin (to_slot);
                                    }
                                  pl = nullptr;
                                }

                              /* if there was a plugin at the slot before,
                               * bring it back
                               */
                              revert_deleted_plugin (*to_tr, to_slot);

                              if (copy)
                                {
                                  pl_var =
                                    from_tr->get_plugin_at_slot (from_slot);
                                }
                              z_return_if_fail (pl);
                              z_return_if_fail (pl_var.has_value ());

                              /* add orig plugin to mixer selections */
                              std::visit (
                                [&] (auto &&prev_pl) {
                                  prev_pl->set_selected (true);
                                },
                                existing_pl_var.value ());
                            },
                            pl_var.value (), own_pl_var);
                        }

                      /* if a new track was created delete it */
                      if (new_channel_)
                        {
                          TRACKLIST->remove_track (to_tr->get_uuid ());
                        }

                      from_tr->validate ();

                      /* EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED,
                       * to_ch); */
                    }
                },
                to_tr_var.value ());
            }

          /* restore connections */
          save_or_load_port_connections (do_it);

          ROUTER->recalc_graph (false);
        }
    },
    from_tr_var.value ());
}

void
MixerSelectionsAction::do_or_undo (bool do_it)
{
  switch (mixer_selections_action_type_)
    {
    case Type::Create:
      do_or_undo_create_or_delete (do_it, true);
      break;
    case Type::Delete:
      do_or_undo_create_or_delete (do_it, false);
      break;
    case Type::Move:
      do_or_undo_move_or_copy (do_it, false);
      break;
    case Type::Copy:
      do_or_undo_move_or_copy (do_it, true);
      break;
    case Type::Paste:
      do_or_undo_create_or_delete (do_it, true);
      break;
    case Type::ChangeStatus:
      do_or_undo_change_status (do_it);
      break;
    case Type::ChangeLoadBehavior:
      do_or_undo_change_load_behavior (do_it);
      break;
    default:
      z_warn_if_reached ();
    }

  /* if first do and keeping track of connections, clone the new connections */
  if (do_it && port_connections_before_ && !port_connections_after_)
    port_connections_after_ = PORT_CONNECTIONS_MGR->clone_unique ();
}

void
MixerSelectionsAction::perform_impl ()
{
  do_or_undo (true);
}

void
MixerSelectionsAction::undo_impl ()
{
  do_or_undo (false);
}

QString
MixerSelectionsAction::to_string () const
{
  switch (mixer_selections_action_type_)
    {
    case Type::Create:
      if (num_plugins_ == 1)
        {
          return format_qstr (QObject::tr ("Create {}"), setting_->get_name ());
        }
      else
        {
          return format_qstr (
            QObject::tr ("Create {} {}s"), num_plugins_, setting_->get_name ());
        }
    case Type::Delete:
      if (ms_before_->size () == 1)
        {
          return QObject::tr ("Delete Plugin");
        }
      else
        {
          return format_qstr (
            QObject::tr ("Delete {} Plugins"), ms_before_->size ());
        }
    case Type::Move:
      if (ms_before_->size () == 1)
        {
          return format_qstr (
            QObject::tr ("Move {}"),
            PluginSpan::name_projection (PluginSpan{ *ms_before_ }.front ()));
        }
      else
        {
          return format_qstr (
            QObject::tr ("Move {} Plugins"), ms_before_->size ());
        }
    case Type::Copy:
      if (ms_before_->size () == 1)
        {
          return format_qstr (
            QObject::tr ("Copy {}"),
            PluginSpan::name_projection (PluginSpan{ *ms_before_ }.front ()));
        }
      else
        {
          return format_qstr (
            QObject::tr ("Copy {} Plugins"), ms_before_->size ());
        }
    case Type::Paste:
      if (ms_before_->size () == 1)
        {
          return format_qstr (
            QObject::tr ("Paste {}"),
            PluginSpan::name_projection (PluginSpan{ *ms_before_ }.front ()));
        }
      else
        {
          return format_qstr (
            QObject::tr ("Paste {} Plugins"), ms_before_->size ());
        }
    case Type::ChangeStatus:
      if (ms_before_->size () == 1)
        {
          return format_qstr (
            QObject::tr ("Change Status for {}"),
            PluginSpan::name_projection (PluginSpan{ *ms_before_ }.front ()));
        }
      else
        {
          return format_qstr (
            QObject::tr ("Change Status for {} Plugins"), ms_before_->size ());
        }
    case Type::ChangeLoadBehavior:
      return format_qstr (
        QObject::tr ("Change Load Behavior for {}"),
        PluginSpan::name_projection (PluginSpan{ *ms_before_ }.front ()));
    }

  return {};
}
