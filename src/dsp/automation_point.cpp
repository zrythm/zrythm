// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/** \file
 */

#include <math.h>

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
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "plugins/plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static AutomationPoint *
_create_new (const Position * pos)
{
  AutomationPoint * self = object_new (AutomationPoint);

  ArrangerObject * obj = (ArrangerObject *) self;
  arranger_object_init (obj);
  obj->pos = *pos;
  obj->type = ArrangerObjectType::ARRANGER_OBJECT_TYPE_AUTOMATION_POINT;
  curve_opts_init (&self->curve_opts);
  self->curve_opts.curviness = 0;
  self->curve_opts.algo =
    ZRYTHM_TESTING
      ? CurveAlgorithm::SUPERELLIPSE
      : (CurveAlgorithm) g_settings_get_enum (
        S_P_EDITING_AUTOMATION, "curve-algorithm");

  self->index = -1;

  return self;
}

/**
 * Sets the ZRegion and the index in the
 * region that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_region_and_index (
  AutomationPoint * ap,
  ZRegion *         region,
  int               index)
{
  g_return_if_fail (ap && region);
  ArrangerObject * obj = (ArrangerObject *) ap;
  region_identifier_copy (&obj->region_id, &region->id);
  ap->index = index;

  /* set the info to the transient too */
  if (
    (ZRYTHM_HAVE_UI || ZRYTHM_TESTING) && PROJECT->loaded && obj->transient
    && arranger_object_should_orig_be_visible (obj, NULL))
    {
      ArrangerObject *  trans_obj = obj->transient;
      AutomationPoint * trans_ap = (AutomationPoint *) trans_obj;
      region_identifier_copy (&trans_obj->region_id, &region->id);
      trans_ap->index = index;
    }
}

int
automation_point_is_equal (AutomationPoint * a, AutomationPoint * b)
{
  ArrangerObject * a_obj = (ArrangerObject *) a;
  ArrangerObject * b_obj = (ArrangerObject *) b;
  return position_is_equal_ticks (&a_obj->pos, &b_obj->pos)
         && math_floats_equal_epsilon (a->fvalue, b->fvalue, 0.001f);
}

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 */
AutomationPoint *
automation_point_new_float (
  const float      value,
  const float      normalized_val,
  const Position * pos)
{
  AutomationPoint * self = _create_new (pos);

  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (value);
      math_assert_nonnann (normalized_val);
    }

  self->fvalue = value;
  self->normalized_val = normalized_val;

  return self;
}

/**
 * Returns if the curve of the AutomationPoint
 * curves upwards as you move right on the x axis.
 */
bool
automation_point_curves_up (AutomationPoint * self)
{
  ZRegion * region = arranger_object_get_region ((ArrangerObject *) self);
  AutomationPoint * next_ap =
    automation_region_get_next_ap (region, self, true, true);

  if (!next_ap)
    return false;

  /* fvalue can be equal in non-float automation even though there is a curve.
   * use the normalized value instead */
  if (next_ap->normalized_val > self->normalized_val)
    return true;
  else
    return false;
}

/**
 * Sets the value from given real or normalized
 * value and notifies interested parties.
 *
 * @param is_normalized Whether the given value is
 *   normalized.
 */
void
automation_point_set_fvalue (
  AutomationPoint * self,
  float             real_val,
  bool              is_normalized,
  bool              pub_events)
{
  g_return_if_fail (self);

  Port * port = automation_point_get_port (self);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (real_val);
    }

  float normalized_val;
  if (is_normalized)
    {
      g_message ("received normalized val %f", (double) real_val);
      normalized_val = CLAMP (real_val, 0.f, 1.f);
      real_val = control_port_normalized_val_to_real (port, normalized_val);
    }
  else
    {
      g_message ("reveived real val %f", (double) real_val);
      real_val = CLAMP (real_val, port->minf, port->maxf);
      normalized_val = control_port_real_val_to_normalized (port, real_val);
    }
  g_message ("setting to %f", (double) real_val);
  self->fvalue = real_val;
  self->normalized_val = normalized_val;

  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (self->fvalue);
      math_assert_nonnann (self->normalized_val);
    }

  ZRegion * region = arranger_object_get_region ((ArrangerObject *) self);
  g_return_if_fail (region);

  /* don't set value - wait for engine to process
   * it */
#if 0
  control_port_set_val_from_normalized (
    port, self->normalized_val, Z_F_AUTOMATING);
#endif

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CHANGED, self);
    }
}

const char *
automation_point_get_fvalue_as_string (AutomationPoint * self)
{
  if (self->tmp_str)
    g_free (self->tmp_str);

  self->tmp_str = g_strdup_printf ("%f", self->fvalue);

  return self->tmp_str;
}

void
automation_point_set_fvalue_with_action (
  AutomationPoint * self,
  const char *      fval_str)
{
  Port * port = automation_point_get_port (self);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  float val;
  int   res = sscanf (fval_str, "%f", &val);
  if (res != 1 || res == EOF || val < port->minf || val > port->maxf)
    {
      ui_show_error_message_printf (
        _ ("Invalid Value"), _ ("Please enter a number between %f and %f"),
        port->minf, port->maxf);
      return;
    }

  ArrangerObject * obj = (ArrangerObject *) self;
  arranger_object_edit_begin (obj);
  automation_point_set_fvalue (self, val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  arranger_object_edit_finish (
    obj,
    ArrangerSelectionsActionEditType::ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);
}

/**
 * The function to return a point on the curve.
 *
 * See
 * https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ap The start point (0, 0).
 * @param region region The automation region (if known),
 *   otherwise the non-cached region will be used.
 * @param x Normalized x.
 */
double
automation_point_get_normalized_value_in_curve (
  AutomationPoint * self,
  ZRegion *         region,
  double            x)
{
  g_return_val_if_fail (self && x >= 0.0 && x <= 1.0, 0.0);

  if (!region)
    {
      region = arranger_object_get_region ((ArrangerObject *) self);
    }
  g_return_val_if_fail (IS_REGION_AND_NONNULL (region), 0.0);
  AutomationPoint * next_ap =
    automation_region_get_next_ap (region, self, true, true);
  if (!next_ap)
    {
      return self->fvalue;
    }

  double dy;

  int start_higher = next_ap->normalized_val < self->normalized_val;
  dy = curve_get_normalized_y (x, &self->curve_opts, start_higher);
  return dy;
}

/**
 * Sets the curviness of the AutomationPoint.
 */
void
automation_point_set_curviness (
  AutomationPoint * self,
  const curviness_t curviness)
{
  if (math_doubles_equal (self->curve_opts.curviness, curviness))
    return;

  self->curve_opts.curviness = curviness;
}

/**
 * Convenience function to return the control port
 * that this AutomationPoint is for.
 */
Port *
automation_point_get_port (const AutomationPoint * const self)
{
  const AutomationTrack * const at =
    automation_point_get_automation_track (self);
  g_return_val_if_fail (at, NULL);
  Port * port = port_find_from_identifier (&at->port_id);
  g_return_val_if_fail (port, NULL);

  return port;
}

/**
 * Convenience function to return the
 * AutomationTrack that this AutomationPoint is in.
 */
AutomationTrack *
automation_point_get_automation_track (const AutomationPoint * const self)
{
  g_return_val_if_fail (self, NULL);
  const ZRegion * const region =
    arranger_object_get_region ((const ArrangerObject *) (self));
  g_return_val_if_fail (region, NULL);
  return region_get_automation_track (region);
}
