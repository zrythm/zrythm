// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port.h"
#include "structure/arrangement/automation_point.h"
#include "structure/arrangement/automation_region.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/instrument_track.h"
#include "structure/tracks/track.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"

#include <fmt/printf.h>

namespace zrythm::structure::arrangement
{
AutomationPoint::AutomationPoint (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (Type::AutomationPoint, track_resolver), QObject (parent),
      RegionOwnedObject (obj_registry)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
}

void
init_from (
  AutomationPoint       &obj,
  const AutomationPoint &other,
  utils::ObjectCloneType clone_type)
{
  if (ZRYTHM_TESTING)
    {
      z_return_if_fail (
        utils::math::assert_nonnann (other.normalized_val_)
        && utils::math::assert_nonnann (other.fvalue_));
    }

  obj.fvalue_ = other.fvalue_;
  obj.normalized_val_ = other.normalized_val_;
  obj.curve_opts_ = other.curve_opts_;
  obj.region_id_ = other.region_id_;
  // index_ = other.index_;
  init_from (
    static_cast<RegionOwnedObject &> (obj),
    static_cast<const RegionOwnedObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

ArrangerObjectPtrVariant
AutomationPoint::add_clone_to_project (bool fire_events) const
{
  return {};
  // FIXME remove these "project" methods completely from ArrangerObject
#if 0
  auto * clone = clone_raw_ptr ();
  get_region ()->append_object (clone->get_uuid ());
  return clone;
#endif
}

ArrangerObjectPtrVariant
AutomationPoint::insert_clone_to_project () const
{
  return {};
// FIXME remove this method
#if 0
  auto * clone = clone_raw_ptr ();
  // FIXME index missing
  get_region ()->insert_object (clone->get_uuid (), 0);
  return clone;
#endif
}

bool
AutomationPoint::curves_up () const
{
  auto              region = dynamic_cast<AutomationRegion *> (get_region ());
  AutomationPoint * next_ap = region->get_next_ap (*this, true);

  if (!next_ap)
    return false;

  /* fvalue can be equal in non-float automation even though there is a curve.
   * use the normalized value instead */
  if (next_ap->normalized_val_ > normalized_val_)
    return true;
  else
    return false;
}

void
AutomationPoint::set_fvalue (float real_val, bool is_normalized)
{
  auto port = get_port ();
  z_return_if_fail (port);

  if (ZRYTHM_TESTING)
    {
      utils::math::assert_nonnann (real_val);
    }

  float normalized_val;
  if (is_normalized)
    {
      z_info ("received normalized val {:f}", (double) real_val);
      normalized_val = std::clamp (real_val, 0.f, 1.f);
      real_val = port->normalized_val_to_real (normalized_val);
    }
  else
    {
      z_info ("reveived real val {:f}", (double) real_val);
      real_val = port->range_.clamp_to_range (real_val);
      normalized_val = port->real_val_to_normalized (real_val);
    }
  z_info ("setting to {:f}", (double) real_val);
  fvalue_ = real_val;
  normalized_val_ = normalized_val;

  if (ZRYTHM_TESTING)
    {
      utils::math::assert_nonnann (fvalue_);
      utils::math::assert_nonnann (normalized_val_);
    }

  z_return_if_fail (get_region ());

  /* don't set value - wait for engine to process it */
#if 0
  control_port_set_val_from_normalized (
    port, self->normalized_val, Z_F_AUTOMATING);
#endif
}

std::string
AutomationPoint::get_fvalue_as_string () const
{
  return fmt::sprintf ("{:f}", fvalue_);
}

void
AutomationPoint::set_fvalue_with_action (const std::string &fval_str)
{
  Port * port = get_port ();

  float val;
  int   res = sscanf (fval_str.c_str (), "%f", &val);
  if (
    res != 1 || res == EOF || val < port->range_.minf_
    || val > port->range_.maxf_)
    {
#if 0
      ui_show_error_message_printf (
        QObject::tr ("Invalid Value"), QObject::tr ("Please enter a number between {:f} and {:f}"),
        port->minf_, port->maxf_);
#endif
      return;
    }

// TODO
#if 0
  edit_begin ();
  set_fvalue (val, false);
  edit_finish (
    (int) gui::actions::ArrangerSelectionsAction::EditType::Primitive);
#endif
}

double
AutomationPoint::get_normalized_value_in_curve (
  AutomationRegion * region,
  double             x) const
{
  z_return_val_if_fail (x >= 0.0 && x <= 1.0, 0.0);

  if (!region)
    {
      region = dynamic_cast<AutomationRegion *> (get_region ());
    }
  z_return_val_if_fail (region, 0.0);
  AutomationPoint * next_ap = region->get_next_ap (*this, true);
  if (!next_ap)
    {
      return fvalue_;
    }

  double dy;

  int start_higher = next_ap->normalized_val_ < normalized_val_;
  dy = curve_opts_.get_normalized_y (x, start_higher);
  return dy;
}

void
AutomationPoint::set_curviness (const curviness_t curviness)
{
  if (utils::math::floats_equal (curve_opts_.curviness_, curviness))
    return;

  curve_opts_.curviness_ = curviness;
}

ControlPort *
AutomationPoint::get_port () const
{
  const auto * const at = get_automation_track ();
  z_return_val_if_fail (at, nullptr);
  auto port_var = PROJECT->find_port_by_id (at->port_id_);
  z_return_val_if_fail (
    port_var && std::holds_alternative<ControlPort *> (*port_var), nullptr);
  auto * port = std::get<ControlPort *> (*port_var);
  z_return_val_if_fail (port, nullptr);

  return port;
}

tracks::AutomationTrack *
AutomationPoint::get_automation_track () const
{
  const auto region = dynamic_cast<AutomationRegion *> (get_region ());
  z_return_val_if_fail (region, nullptr);
  return region->get_automation_track ();
}

bool
AutomationPoint::validate (bool is_project, dsp::FramesPerTick frames_per_tick)
  const
{
  // TODO
  return true;
}

AutomationPoint::~AutomationPoint () = default;
}
