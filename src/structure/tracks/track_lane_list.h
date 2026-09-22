// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <optional>

#include "structure/tracks/track_lane.h"
#include "utils/expandable_tick_range.h"
#include "utils/qt.h"

namespace zrythm::structure::tracks
{
class TrackLaneList : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    bool lanesVisible READ lanesVisible WRITE setLanesVisible NOTIFY
      lanesVisibleChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum Roles
  {
    TrackLanePtrRole = Qt::UserRole + 1,
  };
  Q_ENUM (Roles)

public:
  TrackLaneList (
    utils::IObjectRegistry &registry,
    dsp::TimebaseProvider * timebase_provider,
    QObject *               parent = nullptr);
  ~TrackLaneList () override;
  Q_DISABLE_COPY_MOVE (TrackLaneList)

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_INVOKABLE TrackLane * getFirstLane () const
  {
    return lanes_.front ().get ();
  }

  bool lanesVisible () const { return lanes_visible_; }
  void setLanesVisible (bool visible)
  {
    if (lanes_visible_ == visible)
      return;

    lanes_visible_ = visible;
    Q_EMIT lanesVisibleChanged (visible);
  }
  Q_SIGNAL void lanesVisibleChanged (bool visible);

  /**
   * @brief Adds a lane to the end of the list with default settings.
   */
  Q_INVOKABLE TrackLane * addLane () { return insertLane (rowCount ()); }
  Q_INVOKABLE TrackLane * insertLane (size_t index);
  Q_INVOKABLE void        removeLane (size_t index);

  Q_INVOKABLE void moveLane (size_t from_index, size_t to_index);

  Q_SIGNAL void
  laneObjectsNeedRecache (utils::ExpandableTickRange affectedRange);

  Q_SIGNAL void totalHeightChanged ();

  // ========================================================================

  [[nodiscard]] size_t size () const noexcept { return lanes_.size (); }

  [[nodiscard]] bool empty () const noexcept { return lanes_.empty (); }

  TrackLane * at (size_t idx) const { return lanes_.at (idx).get (); }

  void clear ();

  auto &lanes () const { return lanes_; }

  auto lanes_view () const
  {
    return lanes_ | std::views::transform (&TrackLaneUuidReference::get);
  }

  /**
   * @brief Returns the index of @p lane in the list, or std::nullopt if it
   * is not one of this list's lanes.
   */
  std::optional<size_t> indexOfLane (const TrackLane * lane) const
  {
    const auto view = lanes_view ();
    const auto it = std::ranges::find (view, lane);
    if (it == view.end ())
      return std::nullopt;
    return static_cast<size_t> (std::ranges::distance (view.begin (), it));
  }

  /**
   * @brief Returns whether any lane in the list is soloed.
   */
  [[nodiscard]] bool any_lane_soloed () const
  {
    return std::ranges::any_of (lanes_view (), &TrackLane::soloed);
  }

  /**
   * @brief Gets the total height of all visible lanes (if any).
   */
  double get_visible_lane_heights () const
  {
    if (!lanes_visible_)
      return 0;

    return std::ranges::fold_left (
      lanes_view () | std::views::transform (&TrackLane::height), 0,
      std::plus{});
  }

  /**
   * @brief Attaches an already-registered lane at @p index.
   *
   * Used to reattach a lane kept alive by a reference while it was
   * detached from the list.
   *
   * @throw std::out_of_range if @p index is greater than the list size.
   * @throw std::invalid_argument if the lane is already in the list or
   * still attached to a list.
   */
  void reinsert_lane (size_t index, TrackLaneUuidReference lane_ref);

  /**
   * @brief Creates missing TrackLane's until @p index.
   */
  void create_missing_lanes (size_t index);

  /**
   * @brief Appends an empty lane when the last lane has content, so the
   * list always ends with an empty lane.
   */
  void ensure_trailing_empty_lane ();

  /**
   * @brief Removes trailing empty lanes until a single trailing empty
   * lane remains.
   *
   * Always keeps at least one lane.
   */
  void trim_trailing_empty_lanes ();

  /**
   * @brief Removes the empty last lanes of the Track (except the last one).
   */
  void remove_empty_last_lanes ();

private:
  static constexpr auto             kLaneIdsKey = "laneIds"sv;
  static constexpr std::string_view kLanesVisibleKey = "lanesVisible";
  friend void to_json (nlohmann::json &j, const TrackLaneList &p);
  friend void from_json (const nlohmann::json &j, TrackLaneList &p);

  void erase (size_t pos);

  /**
   * @brief Removes @p count lanes starting at @p first_row.
   *
   * Clears the removed lanes' owner-list pointers, drops their
   * references and updates the model rows.
   */
  void remove_lanes (size_t first_row, size_t count);

  void update_default_lane_names ();

private:
  TrackLane::TrackLaneDependencies    dependencies_;
  std::vector<TrackLaneUuidReference> lanes_;

  /** Flag to set lanes visible or not. */
  bool lanes_visible_ = false;

  BOOST_DESCRIBE_CLASS (TrackLaneList, (), (), (), (lanes_, lanes_visible_))
};

/**
 * @brief Trims the trailing empty lanes of the list owning @p owner.
 *
 * No effect when @p owner is not a track lane or is detached from its
 * list.
 */
template <arrangement::FinalArrangerObjectSubclass ChildT>
void
trim_trailing_empty_lanes_if_lane (
  arrangement::ArrangerObjectOwner<ChildT> * owner)
{
  if (const auto * lane = dynamic_cast<TrackLane *> (owner); lane != nullptr)
    {
      if (auto * list = lane->owner_list (); list != nullptr)
        {
          list->trim_trailing_empty_lanes ();
        }
    }
}
}
