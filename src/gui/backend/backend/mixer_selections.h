// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_MIXER_SELECTIONS_H__
#define __GUI_BACKEND_MIXER_SELECTIONS_H__

#include "common/dsp/channel.h"
#include "common/dsp/region.h"
#include "common/plugins/carla_native_plugin.h"

class FullMixerSelections;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define MIXER_SELECTIONS (PROJECT->mixer_selections_)

constexpr size_t MIXER_SELECTIONS_MAX_SLOTS = 60;

/**
 * @brief Selections to be used for tracking selected plugins in the currently
 * opened project.
 */
class MixerSelections : public ISerializable<MixerSelections>
{
public:
  ~MixerSelections () override = default;

  virtual void sort () { std::sort (slots_.begin (), slots_.end ()); }

  bool has_any () const { return has_any_; }

  /**
   * Adds a slot to the selections.
   *
   * The selections can only be from one channel.
   *
   * @param track The track (ChannelTrack or ModulatorTrack).
   * @param slot The slot to add to the selections.
   */
  void add_slot (
    const Track                    &track,
    zrythm::plugins::PluginSlotType type,
    int                             slot,
    const bool                      fire_events);

  /**
   * @brief Generates a FullMixerSelections based on this MixerSelections.
   *
   * @return std::unique_ptr<FullMixerSelections>
   * @throw ZrythmException on plugin clone error.
   */
  std::unique_ptr<FullMixerSelections> gen_full_from_this () const;

  /**
   * Returns whether the selections can be pasted to the given slot.
   */
  bool can_be_pasted (
    const Channel                  &ch,
    zrythm::plugins::PluginSlotType type,
    int                             slot) const;

  /**
   * Paste the selections starting at the slot in the given channel.
   *
   * This calls gen_full_from_this() internally to generate FullMixerSelections
   * with cloned plugins (calling init_loaded() on each), which are then pasted.
   */
  void
  paste_to_slot (Channel &ch, zrythm::plugins::PluginSlotType type, int slot);

  /**
   * Returns the first selected plugin if any is selected, otherwise NULL.
   */
  zrythm::plugins::Plugin * get_first_plugin () const;

  /**
   * Returns if the slot is selected or not.
   */
  bool contains_slot (zrythm::plugins::PluginSlotType type, int slot) const
  {
    return type == type_
           && (type == zrythm::plugins::PluginSlotType::Instrument ? has_any_ : std::ranges::any_of (slots_, [slot] (auto i) {
                return i == slot;
              }));
  }

  bool contains_uninstantiated_plugin () const;

  virtual void
  get_plugins (std::vector<zrythm::plugins::Plugin *> &plugins) const;

  /**
   * Removes a slot from the selections.
   *
   * Assumes that the channel is the one already selected.
   */
  void remove_slot (
    int                             slot,
    zrythm::plugins::PluginSlotType type,
    bool                            publish_events);

  void clear (bool fire_events = false);

  bool validate () const;

  /**
   * Get current Track whose plugins are selected from the active project.
   */
  Track * get_track () const;

  /**
   * Returns if the plugin is selected or not.
   */
  bool contains_plugin (const zrythm::plugins::Plugin &pl) const;

  DECLARE_DEFINE_FIELDS_METHOD ();

protected:
  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  /**
   * Gets highest slot in the selections.
   */
  int get_highest_slot () const
  {
    if (slots_.empty ())
      return -1;

    return *std::ranges::min_element (slots_);
  }

  /**
   * Gets lowest slot in the selections.
   */
  int get_lowest_slot () const
  {
    if (slots_.empty ())
      return -1;

    return *std::ranges::max_element (slots_);
  }

public:
  zrythm::plugins::PluginSlotType type_ =
    zrythm::plugins::PluginSlotType::Invalid;

  /** Slots selected. */
  std::vector<int> slots_;

  /** Channel selected. */
  unsigned int track_name_hash_ = 0;

  /** Whether any slot is selected. */
  bool has_any_ = false;
};

using ProjectMixerSelections = MixerSelections;

/**
 * Selections to be used for copying, undoing, etc.
 */
class FullMixerSelections final
    : public MixerSelections,
      public ICloneable<FullMixerSelections>,
      public ISerializable<FullMixerSelections>
{
public:
  void sort () override
  {
    MixerSelections::sort ();
    std::sort (plugins_.begin (), plugins_.end ());
  }

  void init_loaded ()
  {
    for (auto &plugin : plugins_)
      {
        plugin->init_loaded (nullptr, this);
      }
    sort ();
  }

  /**
   * @brief Clones the plugin at the given slot and adds it to the selections.
   *
   * @see MixerSelections.add_slot().
   * @note calls MixerSelections.add_slot() internally.
   * @throw ZrythmException on error.
   */
  void
  add_plugin (const Track &track, zrythm::plugins::PluginSlotType type, int slot);

  void init_after_cloning (const FullMixerSelections &other) override
  {
    type_ = other.type_;
    slots_ = other.slots_;
    track_name_hash_ = other.track_name_hash_;
    has_any_ = other.has_any_;
    for (auto &pl : other.plugins_)
      {
        plugins_.push_back (
          clone_unique_with_variant<zrythm::plugins::PluginVariant> (pl.get ()));
      }
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

  void
  get_plugins (std::vector<zrythm::plugins::Plugin *> &plugins) const override;

  void clear ()
  {
    MixerSelections::clear (false);
    plugins_.clear ();
  }

public:
  /** Plugin clones. */
  std::vector<std::unique_ptr<zrythm::plugins::Plugin>> plugins_;
};

/**
 * @}
 */

#endif
