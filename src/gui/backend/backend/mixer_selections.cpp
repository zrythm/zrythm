// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/channel_track.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/mixer_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <glib/gi18n.h>

Track *
MixerSelections::get_track () const
{
  if (!has_any_)
    return nullptr;

  Track * track = TRACKLIST->find_track_by_name_hash (track_name_hash_);
  z_return_val_if_fail (track, nullptr);
  return track;
}

void
MixerSelections::add_slot (
  const Track                    &track,
  zrythm::plugins::PluginSlotType type,
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
  zrythm::plugins::PluginSlotType type,
  int                             slot)
{
  add_slot (track, type, slot, false);

  zrythm::plugins::Plugin * pl = nullptr;
  switch (type)
    {
    case zrythm::plugins::PluginSlotType::MidiFx:
    case zrythm::plugins::PluginSlotType::Insert:
    case zrythm::plugins::PluginSlotType::Instrument:
      pl =
        dynamic_cast<const ChannelTrack &> (track)
          .get_channel ()
          ->get_plugin_at_slot (slot, type);
      break;
    case zrythm::plugins::PluginSlotType::Modulator:
      pl = dynamic_cast<const ModulatorTrack &> (track).modulators_[slot].get ();
      break;
    default:
      break;
    }
  z_return_if_fail (pl);

  try
    {
      auto pl_clone =
        clone_unique_with_variant<zrythm::plugins::PluginVariant> (pl);
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
  zrythm::plugins::PluginSlotType type,
  bool                            publish_events)
{
  z_info ("removing slot {}", slot);
  slots_.erase (
    std::remove (slots_.begin (), slots_.end (), slot), slots_.end ());

  if (slots_.empty () || type_ == zrythm::plugins::PluginSlotType::Instrument)
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
MixerSelections::contains_plugin (const zrythm::plugins::Plugin &pl) const
{
  if (track_name_hash_ != pl.id_.track_name_hash_)
    return false;

  if (type_ == zrythm::plugins::PluginSlotType::Instrument)
    {
      return pl.id_.slot_type_ == zrythm::plugins::PluginSlotType::Instrument
             && pl.id_.track_name_hash_ == track_name_hash_;
    }
  else
    {
      return std::ranges::any_of (slots_, [&pl, this] (auto slot) {
        return slot == pl.id_.slot_ && type_ == pl.id_.slot_type_;
      });
    }
}

bool
MixerSelections::contains_uninstantiated_plugin () const
{
  std::vector<zrythm::plugins::Plugin *> plugins;
  get_plugins (plugins);

  return std::ranges::any_of (plugins, [] (auto pl) {
    return pl->instantiation_failed_;
  });
}

zrythm::plugins::Plugin *
MixerSelections::get_first_plugin () const
{
  if (has_any_)
    {
      ChannelTrack * track = dynamic_cast<ChannelTrack *> (get_track ());
      z_return_val_if_fail (track, nullptr);
      switch (type_)
        {
        case zrythm::plugins::PluginSlotType::Instrument:
          return track->channel_->instrument_.get ();
        case zrythm::plugins::PluginSlotType::Insert:
          return track->channel_->inserts_[slots_[0]].get ();
        case zrythm::plugins::PluginSlotType::MidiFx:
          return track->channel_->midi_fx_[slots_[0]].get ();
        case zrythm::plugins::PluginSlotType::Modulator:
          return dynamic_cast<ModulatorTrack *> (track)
            ->modulators_[slots_[0]]
            .get ();
        default:
          z_return_val_if_reached (nullptr);
          break;
        }
    }

  return nullptr;
}

void
MixerSelections::get_plugins (
  std::vector<zrythm::plugins::Plugin *> &plugins) const
{
  Track * track = get_track ();
  z_return_if_fail (IS_TRACK_AND_NONNULL (track));

  for (int slot : slots_)
    {
      zrythm::plugins::Plugin * pl = nullptr;
      switch (type_)
        {
        case zrythm::plugins::PluginSlotType::Instrument:
        case zrythm::plugins::PluginSlotType::Insert:
        case zrythm::plugins::PluginSlotType::MidiFx:
          {
            auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
            pl = channel_track.get_channel ()->get_plugin_at_slot (slot, type_);
          }
          break;
        case zrythm::plugins::PluginSlotType::Modulator:
          pl = dynamic_cast<ModulatorTrack *> (track)->modulators_[slot].get ();
          break;
        default:
          z_return_if_reached ();
        }

      z_return_if_fail (pl);

      plugins.push_back (pl);
    }
}

void
FullMixerSelections::get_plugins (
  std::vector<zrythm::plugins::Plugin *> &plugins) const
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

  Track * track = get_track ();
  z_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

  for (int slot : slots_)
    {
      zrythm::plugins::Plugin * pl = nullptr;
      switch (type_)
        {
        case zrythm::plugins::PluginSlotType::Instrument:
        case zrythm::plugins::PluginSlotType::Insert:
        case zrythm::plugins::PluginSlotType::MidiFx:
          {
            auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
            pl = channel_track.get_channel ()->get_plugin_at_slot (slot, type_);
          }
          break;
        case zrythm::plugins::PluginSlotType::Modulator:
          pl = dynamic_cast<ModulatorTrack *> (track)->modulators_[slot].get ();
          break;
        default:
          z_return_val_if_reached (false);
          break;
        }

      z_return_val_if_fail (pl, false);
    }

  return true;
}

void
MixerSelections::clear (bool fire_events)
{
  slots_.clear ();
  has_any_ = false;
  track_name_hash_ = 0;
  type_ = zrythm::plugins::PluginSlotType::Invalid;
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

  Track * track = TRACKLIST->find_track_by_name_hash (track_name_hash_);
  z_return_val_if_fail (IS_TRACK_AND_NONNULL (track), nullptr);

  for (int slot : slots_)
    {
      zrythm::plugins::Plugin * pl = nullptr;
      switch (type_)
        {
        case zrythm::plugins::PluginSlotType::Instrument:
        case zrythm::plugins::PluginSlotType::Insert:
        case zrythm::plugins::PluginSlotType::MidiFx:
          {
            auto &channel_track = dynamic_cast<ChannelTrack &> (*track);
            pl = channel_track.get_channel ()->get_plugin_at_slot (slot, type_);
          }
          break;
        case zrythm::plugins::PluginSlotType::Modulator:
          pl = dynamic_cast<ModulatorTrack *> (track)->modulators_[slot].get ();
          break;
        default:
          z_return_val_if_reached (nullptr);
        }

      z_return_val_if_fail (pl, nullptr);

      try
        {
          auto pl_clone =
            clone_unique_with_variant<zrythm::plugins::PluginVariant> (pl);
          ret->plugins_.emplace_back (std::move (pl_clone));
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException (fmt::format (
            "Failed to clone plugin {}: {}", pl->get_name (), e.what ()));
          return nullptr;
        }
    }

  return ret;
}

bool
MixerSelections::can_be_pasted (
  const Channel                  &ch,
  zrythm::plugins::PluginSlotType type,
  int                             slot) const
{
  int lowest = get_lowest_slot ();
  int highest = get_highest_slot ();
  int delta = highest - lowest;

  return slot + delta < (int) STRIP_SIZE;
}

void
MixerSelections::
  paste_to_slot (Channel &ch, zrythm::plugins::PluginSlotType type, int slot)
{
  try
    {
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsPasteAction> (
        *gen_full_from_this (), *PORT_CONNECTIONS_MGR, type, ch.get_track (),
        slot));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to paste plugins"));
    }
}