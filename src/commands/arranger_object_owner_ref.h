// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <variant>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner_variant.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_lane.h"
#include "utils/logger.h"
#include "utils/typed_uuid_reference.h"

namespace zrythm::commands
{

/**
 * @brief A handle to the owner of arranger objects.
 *
 * For every owner type that is a registry object (tracks, lanes, clips
 * and the tempo object manager), the handle pairs the owner's registry
 * identity with a keep-alive reference. Resolution goes through the
 * registry on every resolve() call, so an owner detached and reattached
 * in between is still found.
 *
 * Owner types that are not registry objects (currently AutomationTrack)
 * are held as raw owner-base pointers instead. These carry no identity
 * or keep-alive: the caller guarantees the owner outlives the handle.
 */
class ArrangerObjectOwnerRef
{
public:
  using PtrVariant = structure::arrangement::ArrangerObjectOwnerPtrVariant;

  explicit ArrangerObjectOwnerRef (structure::tracks::TrackUuidReference ref)
      : storage_ (std::move (ref))
  {
  }
  explicit ArrangerObjectOwnerRef (structure::tracks::TrackLaneUuidReference ref)
      : storage_ (std::move (ref))
  {
  }
  explicit ArrangerObjectOwnerRef (
    structure::arrangement::ArrangerObjectUuidReference ref)
      : storage_ (std::move (ref))
  {
  }
  explicit ArrangerObjectOwnerRef (
    structure::arrangement::TempoObjectManagerUuidReference ref)
      : storage_ (std::move (ref))
  {
  }

  /**
   * @brief Fallback handle for owners that are not registry objects.
   *
   * @param owner_ptrs raw owner-base pointers; the referenced owner must
   * outlive this handle.
   */
  static ArrangerObjectOwnerRef from_owner_pointers (PtrVariant owner_ptrs)
  {
    return ArrangerObjectOwnerRef (PrivateTag{}, std::move (owner_ptrs));
  }

  /**
   * @brief Resolves the owner as the owner base of @p ObjectT.
   *
   * @return the owner, or null if the referenced object does not derive
   * ArrangerObjectOwner<ObjectT> (wrong owner kind for the object type,
   * or the registry entry no longer resolves to such an object).
   */
  template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
  structure::arrangement::ArrangerObjectOwner<ObjectT> * resolve () const
  {
    using Owner = structure::arrangement::ArrangerObjectOwner<ObjectT>;
    return std::visit (
      [] (auto &&alt) -> Owner * {
        using AltT = std::decay_t<decltype (alt)>;
        if constexpr (std::is_same_v<AltT, PtrVariant>)
          {
            return std::visit (
              [] (auto * owner) -> Owner * {
                return dynamic_cast<Owner *> (owner);
              },
              alt);
          }
        else
          {
            return dynamic_cast<Owner *> (alt.get ());
          }
      },
      storage_);
  }

  /**
   * @brief Resolves the owner as the owner base of @p ObjectT.
   *
   * @throw std::invalid_argument if resolve() would return null.
   */
  template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
  structure::arrangement::ArrangerObjectOwner<ObjectT> &
  resolve_or_throw () const
  {
    auto * owner = resolve<ObjectT> ();
    if (owner == nullptr)
      {
        throw std::invalid_argument (
          "owner handle does not resolve to the requested owner base");
      }
    return *owner;
  }

private:
  struct PrivateTag
  {
  };
  ArrangerObjectOwnerRef (PrivateTag, PtrVariant owner_ptrs)
      : storage_ (std::move (owner_ptrs))
  {
  }

  std::variant<
    structure::tracks::TrackUuidReference,
    structure::tracks::TrackLaneUuidReference,
    structure::arrangement::ArrangerObjectUuidReference,
    structure::arrangement::TempoObjectManagerUuidReference,
    PtrVariant>
    storage_;
};

/**
 * @brief Builds a handle for an owner that is a registry object.
 *
 * @throw std::runtime_error if the owner is not registered.
 */
inline ArrangerObjectOwnerRef
make_owner_ref (structure::tracks::Track &track, utils::IObjectRegistry &registry)
{
  return ArrangerObjectOwnerRef (
    structure::tracks::TrackUuidReference (track.get_uuid (), registry));
}
inline ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::TrackLane &lane,
  utils::IObjectRegistry       &registry)
{
  return ArrangerObjectOwnerRef (
    structure::tracks::TrackLaneUuidReference (lane.get_uuid (), registry));
}
inline ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::ArrangerObject &obj,
  utils::IObjectRegistry                 &registry)
{
  return ArrangerObjectOwnerRef (
    structure::arrangement::ArrangerObjectUuidReference (
      obj.get_uuid (), registry));
}
inline ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::TempoObjectManager &manager,
  utils::IObjectRegistry                     &registry)
{
  return ArrangerObjectOwnerRef (
    structure::arrangement::TempoObjectManagerUuidReference (
      manager.get_uuid (), registry));
}

/**
 * @brief Builds a handle for an AutomationTrack owner.
 *
 * AutomationTracks are not registry objects yet, so the handle falls back
 * to raw owner pointers.
 */
inline ArrangerObjectOwnerRef
make_owner_ref (structure::tracks::AutomationTrack &at)
{
  return ArrangerObjectOwnerRef::from_owner_pointers (
    structure::arrangement::ArrangerObjectOwnerPtrVariant{
      static_cast<structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AutomationClip> *> (&at) });
}

/**
 * @brief Converts an owner resolved as raw owner-base pointers into a
 * handle, using the registry identity when the owner is a registry
 * object and raw pointers otherwise.
 *
 * @throw std::invalid_argument if @p owner_ptrs holds a null pointer.
 * @throw std::runtime_error propagated from make_owner_ref when a
 * cast-matched owner is not registered.
 */
inline ArrangerObjectOwnerRef
to_owner_ref (
  structure::arrangement::ArrangerObjectOwnerPtrVariant owner_ptrs,
  utils::IObjectRegistry                               &registry)
{
  return std::visit (
    [&] (auto * owner) -> ArrangerObjectOwnerRef {
      if (owner == nullptr)
        {
          throw std::invalid_argument ("owner pointer is null");
        }
      if (auto * track = dynamic_cast<structure::tracks::Track *> (owner))
        return make_owner_ref (*track, registry);
      if (auto * lane = dynamic_cast<structure::tracks::TrackLane *> (owner))
        return make_owner_ref (*lane, registry);
      if (
        auto * manager =
          dynamic_cast<structure::arrangement::TempoObjectManager *> (owner))
        return make_owner_ref (*manager, registry);
      if (
        auto * automation_track =
          dynamic_cast<structure::tracks::AutomationTrack *> (owner))
        return make_owner_ref (*automation_track);
      if (
        auto * obj =
          dynamic_cast<structure::arrangement::ArrangerObject *> (owner))
        return make_owner_ref (*obj, registry);
      // Owner kind without a registry identity: the handle holds raw
      // owner pointers, so the owner stays alive through the caller's
      // ownership rather than a registry keep-alive
      z_warning (
        "Arranger-object owner of type {} is not a registry object: "
        "holding it without keep-alive",
        typeid (*owner).name ());
      return ArrangerObjectOwnerRef::from_owner_pointers (
        structure::arrangement::ArrangerObjectOwnerPtrVariant{ owner });
    },
    std::move (owner_ptrs));
}

} // namespace zrythm::commands
