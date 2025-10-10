// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_lane.h"
#include "structure/tracks/track_processor.h"
#include "utils/expandable_tick_range.h"

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

public:
  TrackLaneList (
    structure::arrangement::ArrangerObjectRegistry &obj_registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    QObject *                                       parent = nullptr);
  ~TrackLaneList () override = default;
  Z_DISABLE_COPY_MOVE (TrackLaneList)

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

  // ========================================================================

  friend void init_from (
    TrackLaneList         &obj,
    const TrackLaneList   &other,
    utils::ObjectCloneType clone_type);

  [[nodiscard]] size_t size () const noexcept { return lanes_.size (); }

  [[nodiscard]] bool empty () const noexcept { return lanes_.empty (); }

  TrackLane * at (size_t idx) const { return lanes_.at (idx).get (); }

#if 0
  void reserve (size_t size) { lanes_.reserve (size); }

  void push_back (utils::QObjectUniquePtr<TrackLane> &&lane)
  {
    const int idx = static_cast<int> (lanes_.size ());
    beginInsertRows (QModelIndex (), idx, idx);
    lane->setParent (this);
    lanes_.emplace_back (std::move (lane));
    endInsertRows ();
  }
#endif

  /** Removes last lane. */
  utils::QObjectUniquePtr<TrackLane> pop_back ()
  {
    if (!empty ())
      {
        const int idx = static_cast<int> (lanes_.size () - 1);
        beginRemoveRows (QModelIndex (), idx, idx);
        auto lane = std::move (lanes_.back ());
        lanes_.pop_back ();
        endRemoveRows ();
        return lane;
      }
    return {};
  }

  void clear ()
  {
    if (!empty ())
      {
        beginRemoveRows (
          QModelIndex (), 0, static_cast<int> (lanes_.size () - 1));
        lanes_.clear ();
        endRemoveRows ();
      }
  }

  auto &lanes () const { return lanes_; }

  auto lanes_view () const
  {
    return lanes_
           | std::views::transform (&utils::QObjectUniquePtr<TrackLane>::get);
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
   * @brief Creates missing TrackLane's until @p index.
   */
  void create_missing_lanes (size_t index);

  /**
   * @brief Removes the empty last lanes of the Track (except the last one).
   */
  void remove_empty_last_lanes ();

private:
  static constexpr auto             kLanesKey = "lanes"sv;
  static constexpr std::string_view kLanesVisibleKey = "lanesVisible";
  friend void to_json (nlohmann::json &j, const TrackLaneList &p)
  {
    j[kLanesKey] = p.lanes_;
    j[kLanesVisibleKey] = p.lanes_visible_;
  }
  friend void from_json (const nlohmann::json &j, TrackLaneList &p)
  {
    p.lanes_.clear ();
    for (const auto &lane_json : j.at (kLanesKey))
      {
        p.lanes_.emplace_back (
          utils::make_qobject_unique<TrackLane> (p.dependencies_, &p));
        lane_json.get_to (*p.lanes_.back ());
      }
    j.at (kLanesVisibleKey).get_to (p.lanes_visible_);
  }

  void erase (size_t pos);

  void update_default_lane_names ();

private:
  TrackLane::TrackLaneDependencies                dependencies_;
  std::vector<utils::QObjectUniquePtr<TrackLane>> lanes_;

  /** Flag to set lanes visible or not. */
  bool lanes_visible_ = false;

  BOOST_DESCRIBE_CLASS (TrackLaneList, (), (), (), (lanes_, lanes_visible_))
};
}
