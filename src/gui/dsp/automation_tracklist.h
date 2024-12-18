// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation tracklist containing automation
 * points and curves.
 */

#ifndef __AUDIO_AUTOMATION_TRACKLIST_H__
#define __AUDIO_AUTOMATION_TRACKLIST_H__

#include "gui/dsp/automation_track.h"

#include <QAbstractListModel>

class AutomationTrack;
class AutomatableTrack;

using namespace zrythm;

namespace zrythm::gui
{
class Channel;
}

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Each track has an automation tracklist with automation tracks to be generated
 * at runtime, and filled in with automation points/curves when loading projects.
 */
class AutomationTracklist final
    : public QAbstractListModel,
      public ICloneable<AutomationTracklist>,
      public zrythm::utils::serialization::ISerializable<AutomationTracklist>
{
  Q_OBJECT
  QML_ELEMENT

public:
  AutomationTracklist (QObject * parent = nullptr);

public:
  enum Roles
  {
    AutomationTrackPtrRole = Qt::UserRole + 1,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_INVOKABLE void
  showNextAvailableAutomationTrack (AutomationTrack * current_automation_track);
  Q_INVOKABLE void
  hideAutomationTrack (AutomationTrack * current_automation_track);

  // ========================================================================

  void init_after_cloning (const AutomationTracklist &other) override;

  /**
   * Inits a loaded AutomationTracklist.
   */
  ATTR_COLD void init_loaded (AutomatableTrack * track);

  AutomatableTrack * get_track () const;

  /**
   * @brief Adds the given automation track.
   *
   * This takes (QObject) ownership of the AutomationTrack.
   */
  AutomationTrack * add_at (AutomationTrack &at);

  /**
   * Prints info about all the automation tracks.
   *
   * Used for debugging.
   */
  void print_ats () const;

  /**
   * Updates the frames of each position in each child of the automation
   * tracklist recursively.
   *
   * @param from_ticks Whether to update the positions based on ticks (true)
   * or frames (false).
   */
  void update_positions (bool from_ticks, bool bpm_change);

  AutomationTrack * get_prev_visible_at (const AutomationTrack &at) const;

  AutomationTrack * get_next_visible_at (const AutomationTrack &at) const;

  void set_at_visible (AutomationTrack &at, bool visible);

  /**
   * Returns the AutomationTrack after delta visible AutomationTrack's.
   *
   * Negative delta searches backwards.
   *
   * This function searches tracks only in the same Tracklist as the given one
   * (ie, pinned or not).
   */
  AutomationTrack *
  get_visible_at_after_delta (const AutomationTrack &at, int delta) const;

  int
  get_visible_at_diff (const AutomationTrack &src, const AutomationTrack &dest)
    const;

  /**
   * Updates the Track position of the Automatable's and AutomationTrack's.
   *
   * @param track The Track to update to.
   */
  void update_track_name_hash (AutomatableTrack &track);

  /**
   * Removes the AutomationTrack from the AutomationTracklist, optionally
   * freeing it.
   *
   * @return The removed automation track (in case we want to move it). Can be
   * ignored to let it get free'd when it goes out of scope.
   */
  AutomationTrack *
  remove_at (AutomationTrack &at, bool free, bool fire_events);

  /**
   * Removes the AutomationTrack's associated with this channel from the
   * AutomationTracklist in the corresponding Track.
   */
  void remove_channel_ats (gui::Channel * ch);

  /**
   * Returns the AutomationTrack corresponding to the given Port.
   */
  AutomationTrack * get_at_from_port (const Port &port) const;

  /**
   * Unselects all arranger objects.
   */
  void unselect_all ();

  /**
   * Removes all objects recursively.
   */
  void clear_objects ();

  /**
   * Sets the index of the AutomationTrack and swaps it with the
   * AutomationTrack at that index or pushes the other AutomationTrack's down.
   *
   * A special case is when @p index == @ref ats_.size(). In this case, the
   * given automation track is set last and all the other automation tracks
   * are pushed upwards.
   *
   * @param push_down False to swap positions with the current
   * AutomationTrack, or true to push down all the tracks below.
   */
  void set_at_index (AutomationTrack &at, int index, bool push_down);

  /**
   * Gets the automation track matching the given arguments.
   *
   * Currently only used in mixer selections action.
   */
  AutomationTrack * get_plugin_at (
    zrythm::dsp::PluginSlotType slot_type,
    int                         plugin_slot,
    int                         port_index,
    const std::string          &symbol);

  /**
   * Used when the add button is added and a new automation track is requested
   * to be shown.
   *
   * Marks the first invisible automation track as visible, or marks an
   * uncreated one as created if all invisible ones are visible, and returns
   * it.
   */
  AutomationTrack * get_first_invisible_at () const;

  void append_objects (std::vector<ArrangerObject *> objects) const;

  /**
   * Returns the number of visible AutomationTrack's.
   */
  int get_num_visible () const;

  /**
   * Verifies the identifiers on a live automation tracklist (in the project,
   * not a clone).
   *
   * @return True if pass.
   */
  bool validate () const;

  /**
   * Counts the total number of regions in the automation tracklist.
   */
  int get_num_regions () const;

  void print_regions () const;

  void set_caches (CacheType types);

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * @brief Automation tracks in this automation tracklist.
   *
   * These should be updated with ALL of the automatables available in the
   * channel and its plugins every time there is an update.
   *
   * Active automation lanes that are shown in the UI, including hidden ones,
   * can be found using @ref AutomationTrack.created_ and @ref
   * AutomationTrack.visible_.
   *
   * Automation tracks become active automation lanes when they have
   * automation or are selected.
   */
  std::vector<AutomationTrack *> ats_;

  /**
   * Cache of automation tracks in record mode, used in recording manager to
   * avoid looping over all automation tracks.
   */
  std::vector<AutomationTrack *> ats_in_record_mode_;

  /**
   * Cache of visible automation tracks.
   */
  std::vector<AutomationTrack *> visible_ats_;

  /**
   * Pointer back to the track.
   *
   * This should be set during initialization.
   */
  AutomatableTrack * track_ = nullptr;
};

/**
 * @}
 */

#endif
