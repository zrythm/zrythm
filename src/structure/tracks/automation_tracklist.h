// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/processor_base.h"
#include "structure/tracks/automation_track.h"

#include <QAbstractListModel>

namespace zrythm::structure::tracks
{
static constexpr int DEFAULT_AUTOMATION_TRACK_HEIGHT = 48;
static constexpr int MIN_AUTOMATION_TRACK_HEIGHT = 26;

class AutomationTracklist;

/**
 * @brief Holder of an automation track and some metadata about it.
 */
class AutomationTrackHolder : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (double height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY (bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY (AutomationTrack * automationTrack READ automationTrack CONSTANT)

public:
  struct Dependencies
  {
    const dsp::TempoMap                            &tempo_map_;
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry_;
    dsp::PortRegistry                              &port_registry_;
    dsp::ProcessorParameterRegistry                &param_registry_;
    structure::arrangement::ArrangerObjectRegistry &object_registry_;
  };

  AutomationTrackHolder (Dependencies dependencies, QObject * parent = nullptr)
      : QObject (parent), dependencies_ (dependencies)
  {
  }
  AutomationTrackHolder (
    Dependencies                               dependencies,
    utils::QObjectUniquePtr<AutomationTrack> &&at,
    QObject *                                  parent = nullptr)
      : AutomationTrackHolder (dependencies, parent)
  {
    automation_track_ = std::move (at);
    automation_track_->setParent (this);
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  double height () const { return height_; }
  void   setHeight (double height)
  {
    height =
      std::max (height, static_cast<double> (MIN_AUTOMATION_TRACK_HEIGHT));
    if (qFuzzyCompare (height, height_))
      return;

    height_ = height;
    Q_EMIT heightChanged (height);
  }
  Q_SIGNAL void heightChanged (double height);

  bool visible () const { return visible_; }
  void setVisible (bool visible)
  {
    assert (created_by_user_);
    if (visible == visible_)
      return;

    visible_ = visible;
    Q_EMIT visibleChanged (visible);
  }
  Q_SIGNAL void visibleChanged (bool visible);

  AutomationTrack * automationTrack () const
  {
    return automation_track_.get ();
  }

  // ========================================================================

public:
  /**
   * @brief Whether this automation track has been created by the user in
   * the UI.
   */
  bool created_by_user_{};

private:
  Dependencies dependencies_;

  utils::QObjectUniquePtr<AutomationTrack> automation_track_;

  /**
   * Whether this automation track is visible.
   *
   * Being created is a precondition for this.
   */
  bool visible_{};

  /** Position of multipane handle. */
  double height_{ DEFAULT_AUTOMATION_TRACK_HEIGHT };

private:
  static constexpr auto kAutomationTrackKey = "automationTrack"sv;
  static constexpr auto kCreatedByUserKey = "createdByUser"sv;
  static constexpr auto kVisible = "visible"sv;
  static constexpr auto kHeightKey = "height"sv;
  friend void to_json (nlohmann::json &j, const AutomationTrackHolder &nfo)
  {
    j[kAutomationTrackKey] = nfo.automation_track_;
    j[kCreatedByUserKey] = nfo.created_by_user_;
    j[kVisible] = nfo.visible_;
    j[kHeightKey] = nfo.height_;
  }
  friend void from_json (const nlohmann::json &j, AutomationTrackHolder &nfo);
};

/**
 * @brief A container that manages a list of automation tracks.
 */
class AutomationTracklist final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    bool automationVisible READ automationVisible WRITE setAutomationVisible
      NOTIFY automationVisibleChanged)
  QML_UNCREATABLE ("")

  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;

public:
  AutomationTracklist (
    AutomationTrackHolder::Dependencies dependencies,
    QObject *                           parent = nullptr);

public:
  enum Roles
  {
    AutomationTrackHolderRole = Qt::UserRole + 1,
    AutomationTrackRole,
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

  bool          automationVisible () const { return automation_visible_; }
  void          setAutomationVisible (bool visible);
  Q_SIGNAL void automationVisibleChanged (bool visible);

  // ========================================================================

  friend void init_from (
    AutomationTracklist       &obj,
    const AutomationTracklist &other,
    utils::ObjectCloneType     clone_type);

  /**
   * @brief Adds the given automation track.
   */
  AutomationTrack *
  add_automation_track (utils::QObjectUniquePtr<AutomationTrack> &&at);

  AutomationTrack * automation_track_at (size_t index) const
  {
    return automation_tracks_.at (index)->automationTrack ();
  }

  /**
   * Returns the AutomationTrack after delta visible AutomationTrack's.
   *
   * Negative delta searches backwards.
   */
  AutomationTrack *
  get_visible_automation_track_after_delta (const AutomationTrack &at, int delta)
    const;

  int get_visible_automation_track_count_between (
    const AutomationTrack &src,
    const AutomationTrack &dest) const;

  /**
   * Removes the AutomationTrack from the AutomationTracklist, optionally
   * freeing it.
   *
   * @return The removed automation track (in case we want to move it). Can be
   * ignored to let it get free'd when it goes out of scope.
   */
  utils::QObjectUniquePtr<AutomationTrackHolder>
  remove_automation_track (AutomationTrack &at);

  /**
   * Removes all arranger objects recursively.
   */
  void clear_arranger_objects ();

  /**
   * Returns the y pixels from the value based on the allocation of the
   * automation track.
   */
  static int get_y_px_from_normalized_val (
    const double automation_track_height,
    const float  normalized_val)
  {
    return static_cast<int> (
      automation_track_height - (normalized_val * automation_track_height));
  }

  /**
   * Swaps @p at with the automation track at @p index or pushes the other
   * automation tracks down.
   *
   * A special case is when @p index == size(). In this case, the
   * given automation track is set last and all the other automation tracks
   * are pushed upwards.
   *
   * @param push_down False to swap positions with the current
   * AutomationTrack, or true to push down all the tracks below.
   */
  void
  set_automation_track_index (AutomationTrack &at, int index, bool push_down);

  /**
   * Used when the add button is added and a new automation track is requested
   * to be shown.
   *
   * Marks the first invisible automation track as visible, or marks an
   * uncreated one as created if all invisible ones are visible, and returns
   * it.
   *
   * @return The holder or nullptr.
   */
  AutomationTrackHolder * get_first_invisible_automation_track_holder () const;

  auto &automation_track_holders () const { return automation_tracks_; }

  auto automation_tracks () const
  {
    return std::views::transform (
      automation_track_holders (),
      [] (const auto &th) { return th->automationTrack (); });
  }

  AutomationTrack *
  get_previous_visible_automation_track (const AutomationTrack &at) const;

  AutomationTrack *
  get_next_visible_automation_track (const AutomationTrack &at) const;

private:
  static constexpr auto kAutomationTracksKey = "automationTracks"sv;
  static constexpr auto kAutomationVisibleKey = "automationVisible"sv;
  friend void to_json (nlohmann::json &j, const AutomationTracklist &ats)
  {
    j[kAutomationTracksKey] = ats.automation_tracks_;
    j[kAutomationVisibleKey] = ats.automation_visible_;
  }
  friend void from_json (const nlohmann::json &j, AutomationTracklist &ats);

  auto &get_port_registry () { return dependencies_.port_registry_; }
  auto &get_port_registry () const { return dependencies_.port_registry_; }

  auto &automation_track_holders () { return automation_tracks_; }
  auto  get_iterator_for_automation_track (const AutomationTrack &at) const
  {
    auto it = std::ranges::find (
      automation_track_holders (), &at,
      [&] (const auto &ath) { return ath->automationTrack (); });
    if (it == automation_track_holders ().end ())
      {
        throw std::runtime_error ("Automation track not found");
      }
    return it;
  }
  auto get_iterator_for_automation_track (const AutomationTrack &at)
  {
    auto it = std::ranges::find (
      automation_track_holders (), &at,
      [&] (const auto &ath) { return ath->automationTrack (); });
    if (it == automation_track_holders ().end ())
      {
        throw std::runtime_error ("Automation track not found");
      }
    return it;
  }
  int get_automation_track_index (const AutomationTrack &at) const
  {
    return static_cast<int> (std::distance (
      automation_track_holders ().begin (),
      get_iterator_for_automation_track (at)));
  }

private:
  AutomationTrackHolder::Dependencies dependencies_;

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
  std::vector<utils::QObjectUniquePtr<AutomationTrackHolder>> automation_tracks_;

  /** Flag to set automations visible or not. */
  bool automation_visible_{};

  BOOST_DESCRIBE_CLASS (
    AutomationTracklist,
    (),
    (),
    (),
    (automation_tracks_, automation_visible_))
};

}
