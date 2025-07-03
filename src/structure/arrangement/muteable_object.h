// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"

#include <boost/describe.hpp>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

class ArrangerObjectMuteFunctionality : public QObject
{
  Q_OBJECT
  Q_PROPERTY (bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
  QML_ELEMENT

public:
  ArrangerObjectMuteFunctionality (QObject * parent = nullptr) noexcept
      : QObject (parent)
  {
  }
  ~ArrangerObjectMuteFunctionality () override = default;
  Z_DISABLE_COPY_MOVE (ArrangerObjectMuteFunctionality)

  // ========================================================================
  // QML Interface
  // ========================================================================
  bool muted () const { return muted_; }
  void setMuted (bool muted)
  {
    if (muted_ != muted)
      {
        muted_ = muted;
        Q_EMIT mutedChanged (muted);
      }
  }
  Q_SIGNAL void mutedChanged (bool muted);

  // ========================================================================

// TODO
#if 0
template <typename RegionT>
bool
RegionImpl<RegionT>::get_muted (bool check_parent) const
{
  if (check_parent)
    {
      if constexpr (is_laned ())
        {
          auto &lane = get_derived ().get_lane ();
          if (lane.is_effectively_muted ())
            return true;
        }
    }
  return muted ();
}
#endif

private:
  friend void init_from (
    ArrangerObjectMuteFunctionality       &obj,
    const ArrangerObjectMuteFunctionality &other,
    utils::ObjectCloneType                 clone_type)
  {
    obj.muted_ = other.muted_;
  }

  static constexpr std::string_view kMutedKey = "muted";
  friend void
  to_json (nlohmann::json &j, const ArrangerObjectMuteFunctionality &object)
  {
    j[kMutedKey] = object.muted_;
  }
  friend void
  from_json (const nlohmann::json &j, ArrangerObjectMuteFunctionality &object)
  {
    j.at (kMutedKey).get_to (object.muted_);
    Q_EMIT object.mutedChanged (object.muted_);
  }

private:
  /** Whether muted or not. */
  bool muted_{ false };

  BOOST_DESCRIBE_CLASS (ArrangerObjectMuteFunctionality, (), (), (), (muted_))
};

} // namespace zrythm::structure::arrangement
