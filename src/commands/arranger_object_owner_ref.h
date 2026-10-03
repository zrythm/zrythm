// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <variant>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner_variant.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/lane_restoration.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_lane.h"
#include "utils/logger.h"
#include "utils/typed_uuid_reference.h"

namespace zrythm::commands
{

/**
 * @brief A handle to the owner of arranger objects.
 *
 * The handle pairs the owner's registry identity with a keep-alive
 * reference. Resolution goes through the registry on every resolve()
 * call, so an owner detached and reattached in between is still found.
 */
class ArrangerObjectOwnerRef
{
public:
  using PtrVariant = structure::arrangement::ArrangerObjectOwnerPtrVariant;

  explicit ArrangerObjectOwnerRef (structure::tracks::TrackUuidReference ref)
      : storage_ (std::move (ref))
  {
  }
  explicit ArrangerObjectOwnerRef (
    structure::tracks::AutomationTrackUuidReference ref)
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

  /**
   * @brief Returns the handle's registry identity as a reference to
   * @p T, or no value when the owner is not registry-identified as
   * @p T (e.g. an owner held as raw pointers).
   */
  template <typename T>
  std::optional<utils::TypedUuidReference<T>> typed_ref () const
  {
    if (const auto * ref = std::get_if<utils::TypedUuidReference<T>> (&storage_))
      {
        return *ref;
      }
    return std::nullopt;
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
    structure::tracks::AutomationTrackUuidReference,
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
ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::Track &track,
  utils::IObjectRegistry   &registry);
ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::TrackLane &lane,
  utils::IObjectRegistry       &registry);
ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::ArrangerObject &obj,
  utils::IObjectRegistry                 &registry);
ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::TempoObjectManager &manager,
  utils::IObjectRegistry                     &registry);

ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::AutomationTrack &automation_track,
  utils::IObjectRegistry             &registry);

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
      if (
        auto * automation_track =
          dynamic_cast<structure::tracks::AutomationTrack *> (owner))
        return make_owner_ref (*automation_track, registry);
      if (auto * lane = dynamic_cast<structure::tracks::TrackLane *> (owner))
        return make_owner_ref (*lane, registry);
      if (
        auto * manager =
          dynamic_cast<structure::arrangement::TempoObjectManager *> (owner))
        return make_owner_ref (*manager, registry);
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

/**
 * @brief Reattaches @p restoration's lane (when detached), then adds
 * @p object_ref to the owner.
 *
 * A trailing-lane trim can detach the owning lane, and objects added
 * to a detached lane are invisible and skip trailing-empty-lane
 * maintenance.
 *
 * @param owner_ref handle to the owner @p object_ref is added to.
 * @param object_ref reference to the object being added.
 * @param restoration identifies the lane to reattach; no value when the
 * owner is not a lane.
 * @throw std::invalid_argument when @p restoration names a lane other
 * than @p owner_ref's lane.
 */
template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
void
reattach_and_add (
  const ArrangerObjectOwnerRef                              &owner_ref,
  const structure::arrangement::ArrangerObjectUuidReference &object_ref,
  const std::optional<structure::tracks::LaneRestoration>   &restoration)
{
  const auto owner_lane_id =
    owner_ref.typed_ref<structure::tracks::TrackLane> ();
  if (
    restoration.has_value () && owner_lane_id.has_value ()
    && owner_lane_id->id () != restoration->lane.id ())
    {
      throw std::invalid_argument (
        "owner and lane restoration identify different lanes");
    }
  structure::tracks::reattach_lane_if_detached (restoration);
  owner_ref.resolve_or_throw<ObjectT> ().add_object (object_ref);
}

} // namespace zrythm::commands
