// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/automation_clip.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
AutomationClip::AutomationClip (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  utils::IObjectRegistry     &registry,
  QObject *                   parent)
    : Clip (Type::AutomationClip, tempo_map_wrapper, parent),
      ArrangerObjectOwner (registry, *this)
{
}

double
AutomationClip::get_normalized_value_in_curve (
  const AutomationPoint &ap,
  double                 x) const
{
  assert (x >= 0.0 && x <= 1.0);

  AutomationPoint * next_ap = get_next_ap (ap, true);
  if (next_ap == nullptr)
    {
      return ap.value ();
    }

  double dy;

  bool start_higher = next_ap->value () < ap.value ();
  dy = ap.curveOpts ()->normalizedY (x, start_higher);
  return dy;
}

bool
AutomationClip::curves_up (const AutomationPoint &ap) const
{
  AutomationPoint * next_ap = get_next_ap (ap, true);

  if (next_ap == nullptr)
    return false;

  // comment from previous implementation which split normalized value and real
  // value (currently all values are normalized):
  /* fvalue can be equal in non-float automation even though there is a curve.
   * use the normalized value instead */
  return next_ap->value () > ap.value ();
}

AutomationPoint *
AutomationClip::get_prev_ap (const AutomationPoint &ap) const
{
  auto it = std::ranges::find (
    get_children_vector (), ap.get_uuid (), &ArrangerObjectUuidReference::id);

  // if found and not the first element
  if (
    it != get_children_vector ().end () && it != get_children_vector ().begin ())
    {
      return (*std::ranges::prev (it)).template get_object_as<AutomationPoint> ();
    }

  return nullptr;
}

AutomationPoint *
AutomationClip::get_next_ap (const AutomationPoint &ap, bool check_positions) const
{
  if (check_positions)
    {
      AutomationPoint * next_ap = nullptr;
      for (auto * cur_ap_outer : get_children_view ())
        {
          AutomationPoint * cur_ap = cur_ap_outer;

          if (cur_ap->get_uuid () == ap.get_uuid ())
            continue;

          if (
            cur_ap->position ()->ticks() >= ap.position ()->ticks()
            && ((next_ap == nullptr) || cur_ap->position ()->ticks() < next_ap->position ()->ticks()))
            {
              next_ap = cur_ap;
            }
        }
      return next_ap;
    }

  auto it = std::ranges::find (
    get_children_vector (), ap.get_uuid (), &ArrangerObjectUuidReference::id);

  // if found and not the last element
  if (
    it != get_children_vector ().end ()
    && std::ranges::next (it) != get_children_vector ().end ())
    {

      return (*std::ranges::next (it)).template get_object_as<AutomationPoint> ();
    }

  return nullptr;
}

void
init_from (
  AutomationClip        &obj,
  const AutomationClip  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Clip &> (obj), static_cast<const Clip &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<AutomationPoint> &> (obj),
    static_cast<const ArrangerObjectOwner<AutomationPoint> &> (other),
    clone_type);
}

void
to_json (nlohmann::json &j, const AutomationClip &clip)
{
  to_json (j, static_cast<const Clip &> (clip));
  to_json (j, static_cast<const ArrangerObjectOwner<AutomationPoint> &> (clip));
}

void
from_json (const nlohmann::json &j, AutomationClip &clip)
{
  from_json (j, static_cast<Clip &> (clip));
  from_json (j, static_cast<ArrangerObjectOwner<AutomationPoint> &> (clip));
}
}
