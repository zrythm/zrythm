// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/channel.h"
# include "gui/dsp/channel_track.h"
# include "gui/dsp/modulator_track.h"
# include "gui/dsp/tracklist.h"
#include "utils/rt_thread_id.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/mixer_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

OptionalTrackPtrVariant
MixerSelections::get_track () const
{
  if (!has_any_)
    return std::nullopt;

  auto ret = TRACKLIST->find_track_by_name_hash (track_name_hash_);
  z_return_val_if_fail (ret, std::nullopt);
  return ret;
}

void
MixerSelections::add_slot (
  const Track                    &track,
  zrythm::gui::dsp::plugins::PluginSlotType type,
  int                             slot,
  const bool                      fire_events)
{
  unsigned int name_hash = track.get_name_hash ();

  if (!has_any_ || name_hash != track_name_hash_ || type != type_)
    {
      clear (false);
    }
  track_name_hash_ = name_hash;
  type_ = type;
  has_any_ = true;

  z_debug ("adding slot {}:{}:{}", track.name_, ENUM_NAME (type), slot);

  if (!std::ranges::any_of (slots_, [slot] (auto i) { return i == slot; }))
    {
      slots_.push_back (slot);
      std::sort (slots_.begin (), slots_.end ());
    }

  if (fire_events && track.is_in_active_project ())
    {
      /* EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, nullptr); */
    }
}

void
FullMixerSelections::add_plugin (
  const Track                    &track,
  zrythm::gui::dsp::plugins::PluginSlotType type,
  int                             slot)
{
  add_slot (track, type, slot, false);

  zrythm::gui::dsp::plugins::Plugin * pl = nullptr;
  switch (type)
    {
    case zrythm::gui::dsp::plugins::PluginSlotType::MidiFx:
    case zrythm::gui::dsp::plugins::PluginSlotType::Insert:
    case zrythm::gui::dsp::plugins::PluginSlotType::Instrument:
      pl =
        dynamic_cast<const ChannelTrack &> (track)
          .get_channel ()
          ->get_plugin_at_slot (slot, type);
      break;
    case zrythm::gui::dsp::plugins::PluginSlotType::Modulator:
      pl = dynamic_cast<const ModulatorTrack &> (track).modulators_[slot].get ();
      break;
    default:
      break;
    }
  z_return_if_fail (pl);

  try
    {
      auto pl_clone =
        clone_unique_with_variant<zrythm::gui::dsp::plugins::PluginVariant> (pl);
      plugins_.emplace_back (std::move (pl_clone));
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException ("Failed to clone/add plugin to selections");
    }
  sort ();
}

void
MixerSelections::remove_slot (
  int                             slot,
  zrythm::gui::dsp::plugins::PluginSlotType type,
  bool                            publish_events)
{
  z_info ("removing slot {}", slot);
  slots_.erase (
    std::remove (slots_.begin (), slots_.end (), slot), slots_.end ());

  if (slots_.empty () || type_ == zrythm::gui::dsp::plugins::PluginSlotType::Instrument)
    {
      has_any_ = false;
      track_name_hash_ = 0;
    }

  if (ZRYTHM_HAVE_UI && publish_events)
    {
      /* EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, nullptr); */
    }
}

bool
MixerSelections::contains_plugin (const zrythm::gui::dsp::plugins::Plugin &pl) const
{
  if (track_name_hash_ != pl.id_.track_name_hash_)
    return false;

  if (type_ == zrythm::gui::dsp::plugins::PluginSlotType::Instrument)
    {
      return pl.id_.slot_type_ == zrythm::gui::dsp::plugins::PluginSlotType::Instrument
             && pl.id_.track_name_hash_ == track_name_hash_;
    }

  return std::ranges::any_of (slots_, [&pl, this] (auto slot) {
    return slot == pl.id_.slot_ && type_ == pl.id_.slot_type_;
  });
}

bool
MixerSelections::contains_uninstantiated_plugin () const
{
  std::vector<zrythm::gui::dsp::plugins::Plugin *> plugins;
  get_plugins (plugins);

  return std::ranges::any_of (plugins, [] (auto pl) {
    return pl->instantiation_failed_;
  });
}

zrythm::gui::dsp::plugins::Plugin *
MixerSelections::get_first_plugin () const
{
  if (has_any_)
    {
      auto track_var = get_track ();
      z_return_val_if_fail (track_var, nullptr);
      std::visit (
        [&] (auto &&track) -> zrythm::gui::dsp::plugins::Plugin * {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              if (type_ == zrythm::gui::dsp::plugins::PluginSlotType::Instrument)
                return track->channel_->instrument_.get ();
              if (type_ == zrythm::gui::dsp::plugins::PluginSlotType::Insert)
                return track->channel_->inserts_.at (slots_.at (0)).get ();
              if (type_ == zrythm::gui::dsp::plugins::PluginSlotType::MidiFx)
                return track->channel_->midi_fx_.at (slots_.at (0)).get ();
            }
          else if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
            {
              if (type_ == zrythm::gui::dsp::plugins::PluginSlotType::Modulator)
                return track->modulators_.at (slots_.at (0)).get ();
            }
          z_return_val_if_reached (nullptr);
        },
        *track_var);
    }

  return nullptr;
}

void
MixerSelections::get_plugins (
  std::vector<zrythm::gui::dsp::plugins::Plugin *> &plugins) const
{
  auto track_var = get_track ();
  z_return_if_fail (track_var);

  std::visit (
    [&] (auto &&track) -> void {
      using TrackT = base_type<decltype (track)>;

      for (int slot : slots_)
        {
          zrythm::gui::dsp::plugins::Plugin * pl = nullptr;
          switch (type_)
            {
            case zrythm::gui::dsp::plugins::PluginSlotType::Instrument:
            case zrythm::gui::dsp::plugins::PluginSlotType::Insert:
            case zrythm::gui::dsp::plugins::PluginSlotType::MidiFx:
              if constexpr (std::derived_from<TrackT, ChannelTrack>)
                {
                  auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
                  pl = channel_track.get_channel ()->get_plugin_at_slot (
                    slot, type_);
                }
              break;
            case zrythm::gui::dsp::plugins::PluginSlotType::Modulator:
              if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
                {
                  pl = track->modulators_[slot].get ();
                }
              break;
            default:
              z_return_if_reached ();
            }

          z_return_if_fail (pl);

          plugins.push_back (pl);
        }
    },
    *track_var);
}

void
FullMixerSelections::get_plugins (
  std::vector<zrythm::gui::dsp::plugins::Plugin *> &plugins) const
{
  for (auto &plugin : plugins_)
    {
      plugins.push_back (plugin.get ());
    }
}
bool
MixerSelections::validate () const
{
  if (!has_any_)
    return true;

  auto track_var = get_track ();
  z_return_val_if_fail (track_var, false);

  return std::visit (
    [&] (auto &&track) -> bool {
      for (int slot : slots_)
        {
          zrythm::gui::dsp::plugins::Plugin * pl = nullptr;
          switch (type_)
            {
            case zrythm::gui::dsp::plugins::PluginSlotType::Instrument:
            case zrythm::gui::dsp::plugins::PluginSlotType::Insert:
            case zrythm::gui::dsp::plugins::PluginSlotType::MidiFx:
              {
                auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
                pl = channel_track.get_channel ()->get_plugin_at_slot (
                  slot, type_);
              }
              break;
            case zrythm::gui::dsp::plugins::PluginSlotType::Modulator:
              pl =
                dynamic_cast<ModulatorTrack *> (track)->modulators_[slot].get ();
              break;
            default:
              z_return_val_if_reached (false);
              break;
            }

          z_return_val_if_fail (pl, false);
        }

      return true;
    },
    *track_var);
}

void
MixerSelections::clear (bool fire_events)
{
  slots_.clear ();
  has_any_ = false;
  track_name_hash_ = 0;
  type_ = zrythm::gui::dsp::plugins::PluginSlotType::Invalid;
  // if (fire_events)
  /* EVENTS_PUSH (EventType::ET_MIXER_SELECTIONS_CHANGED, nullptr); */
}

std::unique_ptr<FullMixerSelections>
MixerSelections::gen_full_from_this () const
{
  auto ret = std::make_unique<FullMixerSelections> ();
  ret->type_ = type_;
  ret->slots_ = slots_;
  ret->track_name_hash_ = track_name_hash_;
  ret->has_any_ = has_any_;

  auto track_var = TRACKLIST->find_track_by_name_hash (track_name_hash_);
  z_return_val_if_fail (track_var, nullptr);

  return std::visit (
    [&] (auto &&track) -> std::unique_ptr<FullMixerSelections> {
      for (int slot : slots_)
        {
          zrythm::gui::dsp::plugins::Plugin * pl = nullptr;
          switch (type_)
            {
            case zrythm::gui::dsp::plugins::PluginSlotType::Instrument:
            case zrythm::gui::dsp::plugins::PluginSlotType::Insert:
            case zrythm::gui::dsp::plugins::PluginSlotType::MidiFx:
              {
                auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
                pl = channel_track.get_channel ()->get_plugin_at_slot (
                  slot, type_);
              }
              break;
            case zrythm::gui::dsp::plugins::PluginSlotType::Modulator:
              pl =
                dynamic_cast<ModulatorTrack *> (track)->modulators_[slot].get ();
              break;
            default:
              z_return_val_if_reached (nullptr);
            }

          z_return_val_if_fail (pl, nullptr);

          try
            {
              auto pl_clone =
                clone_unique_with_variant<zrythm::gui::dsp::plugins::PluginVariant> (pl);
              ret->plugins_.emplace_back (std::move (pl_clone));
            }
          catch (const ZrythmException &e)
            {
              throw ZrythmException (fmt::format (
                "Failed to clone plugin {}: {}", pl->get_name (), e.what ()));
              return nullptr;
            }
        }

      return std::move (ret);
    },
    *track_var);
}

bool
MixerSelections::can_be_pasted (
  const Channel                  &ch,
  zrythm::gui::dsp::plugins::PluginSlotType type,
  int                             slot) const
{
  int lowest = get_lowest_slot ();
  int highest = get_highest_slot ();
  int delta = highest - lowest;

  return slot + delta < (int) utils::audio::STRIP_SIZE;
}

void
MixerSelections::
  paste_to_slot (Channel &ch, zrythm::gui::dsp::plugins::PluginSlotType type, int slot)
{
  try
    {
      UNDO_MANAGER->perform (new gui::actions::MixerSelectionsPasteAction (
        *gen_full_from_this (), *PORT_CONNECTIONS_MGR, type, ch.get_track (),
        slot));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to paste plugins"));
    }
}
