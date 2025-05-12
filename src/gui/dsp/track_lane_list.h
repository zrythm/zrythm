// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef DSP_TRACK_LANE_LIST_H
#define DSP_TRACK_LANE_LIST_H

#include "gui/dsp/audio_lane.h"
#include "gui/dsp/midi_lane.h"
#include "gui/dsp/track_lane.h"

class TrackLaneList final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum Roles
  {
    TrackLanePtrRole = Qt::UserRole + 1,
  };

public:
  TrackLaneList (QObject * parent = nullptr);
  ~TrackLaneList () override;

  friend class LanedTrackImpl<AudioLane>;
  friend class LanedTrackImpl<MidiLane>;

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_INVOKABLE QVariant getFirstLane () const
  {
    return data (index (0, 0), TrackLanePtrRole);
  }

  // ========================================================================

  void
  copy_members_from (const TrackLaneList &other, ObjectCloneType clone_type);

  [[nodiscard]] size_t size () const noexcept { return lanes_.size (); }

  [[nodiscard]] bool empty () const noexcept { return lanes_.empty (); }

  [[nodiscard]] auto at (size_t idx) const { return lanes_.at (idx); }

  void reserve (size_t size) { lanes_.reserve (size); }

  void push_back (const TrackLanePtrVariant lane)
  {
    const int idx = static_cast<int> (lanes_.size ());
    beginInsertRows (QModelIndex (), idx, idx);
    lanes_.push_back (lane);
    std::visit ([&] (auto &&l) { l->setParent (this); }, lane);
    endInsertRows ();
  }

  /** Removes last lane. */
  void pop_back ()
  {
    if (!empty ())
      {
        const int idx = static_cast<int> (lanes_.size () - 1);
        beginRemoveRows (QModelIndex (), idx, idx);
        auto lane = lanes_.back ();
        std::visit (
          [&] (auto &&l) {
            l->setParent (nullptr);
            lanes_.pop_back ();
            l->deleteLater ();
          },
          lane);

        endRemoveRows ();
      }
  }

  void clear ()
  {
    if (!empty ())
      {
        beginRemoveRows (
          QModelIndex (), 0, static_cast<int> (lanes_.size () - 1));
        for (auto &lane : lanes_)
          {
            std::visit (
              [&] (auto &&l) {
                l->setParent (nullptr);
                l->deleteLater ();
              },
              lane);
          }
        lanes_.clear ();
        endRemoveRows ();
      }
  }

  /** Iterator access. */
  [[nodiscard]] auto begin () noexcept { return lanes_.begin (); }

  [[nodiscard]] auto end () noexcept { return lanes_.end (); }

  [[nodiscard]] auto begin () const noexcept { return lanes_.begin (); }

  [[nodiscard]] auto end () const noexcept { return lanes_.end (); }

  void erase (size_t pos);

  [[nodiscard]] auto front () noexcept { return lanes_.front (); }

  [[nodiscard]] auto back () noexcept { return lanes_.back (); }

  [[nodiscard]] auto front () const noexcept { return lanes_.front (); }

  [[nodiscard]] auto back () const noexcept { return lanes_.back (); }

private:
  static constexpr std::string_view kLanesKey = "lanes";
  friend void to_json (nlohmann::json &j, const TrackLaneList &p)
  {
    j[kLanesKey] = p.lanes_;
  }
  struct Builder
  {
    template <typename TrackLaneT> std::unique_ptr<TrackLaneT> build () const
    {
      // TODO. also should get rid of the Track dependency in track lanes...
      return {};
    }
  };
  friend void from_json (const nlohmann::json &j, TrackLaneList &p)
  {
    for (const auto &lane_json : j.at (kLanesKey))
      {
        TrackLanePtrVariant lane;
        utils::serialization::variant_from_json_with_builder (
          lane_json, lane, Builder{});
        p.lanes_.push_back (lane);
      }
  }

private:
  std::vector<TrackLanePtrVariant> lanes_;
};

#endif
