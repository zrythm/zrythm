// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "actions/arranger_selections.h"
#include "dsp/automation_point.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/channel.h"
#include "dsp/control_port.h"
#include "dsp/instrument_track.h"
#include "dsp/port.h"
#include "dsp/position.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <fmt/printf.h>

AutomationPoint::AutomationPoint (const Position &pos)
    : ArrangerObject (Type::AutomationPoint)
{
  pos_ = pos;
  curve_opts_.algo_ =
    ZRYTHM_TESTING
      ? CurveOptions::Algorithm::SuperEllipse
      : (CurveOptions::Algorithm) g_settings_get_enum (
        S_P_EDITING_AUTOMATION, "curve-algorithm");
}

AutomationPoint::AutomationPoint (
  const float     value,
  const float     normalized_val,
  const Position &pos)
    : AutomationPoint (pos)
{
  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (value);
      math_assert_nonnann (normalized_val);
    }

  fvalue_ = value;
  normalized_val_ = normalized_val;
}

std::string
AutomationPoint::print_to_str () const
{
  return fmt::format (
    "AutomationPoint(fvalue={}, normalized_val={}, pos={})", fvalue_,
    normalized_val_, pos_.to_string ());
}

ArrangerObject::ArrangerObjectPtr
AutomationPoint::find_in_project () const
{
  auto region = AutomationRegion::find (region_id_);
  z_return_val_if_fail (region && (int) region->aps_.size () > index_, nullptr);

  auto &ap = region->aps_[index_];
  z_return_val_if_fail (*this == *ap, nullptr);

  return ap;
}

void
AutomationPoint::init_after_cloning (const AutomationPoint &other)
{
  if (ZRYTHM_TESTING)
    {
      z_return_if_fail (
        math_assert_nonnann (other.normalized_val_)
        && math_assert_nonnann (other.fvalue_));
    }

  AutomationPoint (other.fvalue_, other.normalized_val_, other.pos_);
  curve_opts_ = other.curve_opts_;
  region_id_ = other.region_id_;
  index_ = other.index_;
  RegionOwnedObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}

ArrangerObject::ArrangerObjectPtr
AutomationPoint::add_clone_to_project (bool fire_events) const
{
  return get_region ()->append_object (clone_shared (), true);
}

ArrangerObject::ArrangerObjectPtr
AutomationPoint::insert_clone_to_project () const
{
  return get_region ()->insert_object (clone_shared (), index_, true);
}

bool
AutomationPoint::curves_up () const
{
  auto              region = dynamic_cast<AutomationRegion *> (get_region ());
  AutomationPoint * next_ap = region->get_next_ap (*this, true, true);

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
AutomationPoint::set_fvalue (float real_val, bool is_normalized, bool pub_events)
{
  auto port = get_port ();
  z_return_if_fail (port);

  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (real_val);
    }

  float normalized_val;
  if (is_normalized)
    {
      z_info ("received normalized val %f", (double) real_val);
      normalized_val = CLAMP (real_val, 0.f, 1.f);
      real_val = port->normalized_val_to_real (normalized_val);
    }
  else
    {
      z_info ("reveived real val %f", (double) real_val);
      real_val = CLAMP (real_val, port->minf_, port->maxf_);
      normalized_val = port->real_val_to_normalized (real_val);
    }
  z_info ("setting to %f", (double) real_val);
  fvalue_ = real_val;
  normalized_val_ = normalized_val;

  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (fvalue_);
      math_assert_nonnann (normalized_val_);
    }

  z_return_if_fail (get_region ());

  /* don't set value - wait for engine to process it */
#if 0
  control_port_set_val_from_normalized (
    port, self->normalized_val, Z_F_AUTOMATING);
#endif

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CHANGED, this);
    }
}

std::string
AutomationPoint::get_fvalue_as_string (void * self)
{
  AutomationPoint * ap = static_cast<AutomationPoint *> (self);
  return fmt::sprintf ("%f", ap->fvalue_);
}

void
AutomationPoint::set_fvalue_with_action (void * self, const std::string &fval_str)
{
  auto * ap = static_cast<AutomationPoint *> (self);
  Port * port = ap->get_port ();
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  float val;
  int   res = sscanf (fval_str.c_str (), "%f", &val);
  if (res != 1 || res == EOF || val < port->minf_ || val > port->maxf_)
    {
      ui_show_error_message_printf (
        _ ("Invalid Value"), _ ("Please enter a number between %f and %f"),
        port->minf_, port->maxf_);
      return;
    }

  ap->edit_begin ();
  ap->set_fvalue (val, false, false);
  ap->edit_finish ((int) ArrangerSelectionsAction::EditType::Primitive);
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
  AutomationPoint * next_ap = region->get_next_ap (*this, true, true);
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
  if (math_doubles_equal (curve_opts_.curviness_, curviness))
    return;

  curve_opts_.curviness_ = curviness;
}

ControlPort *
AutomationPoint::get_port () const
{
  const AutomationTrack * const at = get_automation_track ();
  z_return_val_if_fail (at, nullptr);
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  z_return_val_if_fail (port, nullptr);

  return port;
}

ArrangerWidget *
AutomationPoint::get_arranger () const
{
  return (ArrangerWidget *) (MW_AUTOMATION_ARRANGER);
}

AutomationTrack *
AutomationPoint::get_automation_track () const
{
  const auto region = dynamic_cast<AutomationRegion *> (get_region ());
  z_return_val_if_fail (region, nullptr);
  return region->get_automation_track ();
}

bool
AutomationPoint::validate (bool is_project, double frames_to_tick) const
{
  // TODO
  return true;
}

AutomationPoint::~AutomationPoint ()
{
  object_free_w_func_and_null (gsk_render_node_unref, cairo_node_);
  object_free_w_func_and_null (gsk_render_node_unref, cairo_node_tl_);
}