/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/math.h"

#include <gtk/gtk.h>

static Automatable *
_create_blank ()
{
  Automatable * self =
    calloc (1, sizeof (Automatable));

  self->slot = -1;

  return self;
}

static float
get_minf (Automatable * a)
{
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      return lilv_node_as_float (a->control->min);
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return 0.f;
    }
  g_warn_if_reached ();
  return -1;
}

static float
get_maxf (Automatable * a)
{
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      return lilv_node_as_float (a->control->max);
      break;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      return 2.f;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return 1.f;
    }
  g_warn_if_reached ();
  return -1;
}

static Lv2Control *
get_lv2_control (Automatable * self)
{
  g_return_val_if_fail (self->port, NULL);
  g_return_val_if_fail (self->port->lv2_port, NULL);

  /* Note: plugin must be instantiated by now. */
  if (self->type == AUTOMATABLE_TYPE_PLUGIN_CONTROL)
    return  lv2_control_get_from_port (
              self->port->lv2_port);

  return NULL;
}


/**
 * Inits a loaded automatable.
 */
void
automatable_init_loaded (Automatable * self)
{

  switch (self->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      {
        self->port =
          port_find_from_identifier (self->port_id);
        self->control = get_lv2_control (self);
      }
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      {
        self->plugin =
          self->track->channel->plugins[
            self->slot];
      }
      break;
    default:
      /* TODO */
      break;
    }
  self->minf = get_minf (self);
  self->maxf = get_maxf (self);
  self->sizef = self->maxf - self->minf;
}

/**
 * Finds the Automatable in the project from the
 * given clone.
 */
Automatable *
automatable_find (
  Automatable * clone)
{
  switch (clone->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      break;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      break;
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      break;
    }

  return NULL;
}

Automatable *
automatable_create_fader (Channel * channel)
{
  Automatable * a = _create_blank ();

  a->track = channel->track;
  a->label = g_strdup ("Volume");
  a->type = AUTOMATABLE_TYPE_CHANNEL_FADER;
  a->minf = get_minf (a);
  a->maxf = get_maxf (a);
  a->sizef = a->maxf - a->minf;

  return a;
}

Automatable *
automatable_create_pan (Channel * channel)
{
  Automatable * a = _create_blank ();

  a->track = channel->track;
  a->label = g_strdup ("Pan");
  a->type = AUTOMATABLE_TYPE_CHANNEL_PAN;
  a->minf = get_minf (a);
  a->maxf = get_maxf (a);
  a->sizef = a->maxf - a->minf;

  return a;
}

Automatable *
automatable_create_mute (Channel * channel)
{
  Automatable * a = _create_blank ();

  a->track = channel->track;
  a->label = g_strdup ("Mute");
  a->type = AUTOMATABLE_TYPE_CHANNEL_MUTE;
  a->minf = get_minf (a);
  a->maxf = get_maxf (a);
  a->sizef = a->maxf - a->minf;

  return a;
}

Automatable *
automatable_create_lv2_control (
  Plugin *       plugin,
  Lv2Control * control)
{
  Automatable * a = _create_blank ();

  a->control = control;
  Lv2Port * port =
    &control->plugin->ports[control->index];
  a->port = port->port;
  a->port_id = calloc (1, sizeof (PortIdentifier));
  port_identifier_copy (
    &a->port->identifier, a->port_id);

  /*a->index = control->index;*/
  a->type = AUTOMATABLE_TYPE_PLUGIN_CONTROL;
  a->track = plugin->track;
  a->slot = plugin->slot;
  a->label =
    g_strdup (lv2_control_get_label (control));
  a->minf = get_minf (a);
  a->maxf = get_maxf (a);
  a->sizef = a->maxf - a->minf;

  return a;
}

Automatable *
automatable_create_plugin_enabled (
  Plugin * plugin)
{
  Automatable * a = _create_blank ();

  a->type = AUTOMATABLE_TYPE_PLUGIN_ENABLED;
  a->track = plugin->track;
  a->slot = plugin->slot;
  a->label = g_strdup ("Enable/disable");
  a->minf = get_minf (a);
  a->maxf = get_maxf (a);
  a->sizef = a->maxf - a->minf;

  return a;
}

Automatable *
automatable_clone (
  Automatable * src)
{
  Automatable * a = _create_blank ();

  a->index = src->index;
  a->type = src->type;
  a->track_id = src->track_id;
  a->slot = src->slot;
  a->label = g_strdup (src->label);
  a->minf = src->minf;
  a->maxf = src->maxf;
  a->sizef = src->sizef;

  return a;
}

int
automatable_is_bool (Automatable * a)
{
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      if (a->control->value_type ==
          a->control->plugin->forge.Bool)
        {
          return 1;
        }
      return 0;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      return 1;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return 0;
    }
  g_warn_if_reached ();
  return -1;
}

/**
 * Returns the type of its value (float, bool, etc.)
 * as a string.
 *
 * Must be free'd.
 */
char *
automatable_stringize_value_type (Automatable * a)
{
  char * val_type = NULL;
  if (automatable_is_float (a))
    val_type = g_strdup ("Float");
  else if (automatable_is_bool (a))
    val_type = g_strdup ("Boolean");

  return val_type;
}

int
automatable_is_float (Automatable * a)
{
  return 1;

  /* FIXME aren't all floats? this should only
   * be used to determine how the curve should
   * look*/
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      if (a->control->value_type ==
          a->control->plugin->forge.Float)
        {
          return 1;
        }
      return 0;
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      return 0;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return 1;
    }
  g_warn_if_reached ();
  return -1;
}

/**
 * Gets the current value of the parameter the
 * automatable is for.
 *
 * This does not consider the automation track, it
 * only looks in the actual parameter for its
 * current value.
 */
float
automatable_get_val (Automatable * a)
{
  Plugin * plugin;
  Channel * ch;
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      plugin = a->port->plugin;
      if (plugin->descr->protocol == PROT_LV2)
        {
          Lv2Control * control = a->control;
          Lv2Port* port = &control->plugin->ports[control->index];
          return port->control;
        }
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      plugin = a->port->plugin;
      return (float) plugin->enabled;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      ch = track_get_channel (a->track);
      return ch->fader.amp;
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      return (float) a->track->mute;
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      ch = track_get_channel (a->track);
      return ch->fader.pan;
    }
  g_warn_if_reached ();
  return -1;
}

/**
 * Converts normalized value (0.0 to 1.0) to
 * real value (eg. -10.0 to 100.0).
 */
float
automatable_normalized_val_to_real (
  Automatable * a,
  float         val)
{
  Plugin * plugin;
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      plugin = a->port->plugin;
      if (plugin->descr->protocol == PROT_LV2)
        {
          Lv2Control * ctrl = a->control;
          float real_val;
          if (ctrl->is_logarithmic)
            {
              /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
              real_val =
                a->minf *
                  powf (a->maxf / a->minf, val);
            }
          else if (ctrl->is_toggle)
            {
              real_val = val >= 0.5f ? 1.f : 0.f;
            }
          else
            {
              real_val =
                a->minf + val * (a->maxf - a->minf);
            }
          return real_val;
        }
      g_warn_if_reached ();
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      plugin = a->port->plugin;
      return val > 0.5f;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      return
        (float) math_get_amp_val_from_fader (val);
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      return val > 0.5f;
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return val;
    }
  return -1.f;
}

/**
 * Converts real value (eg. -10.0 to 100.0) to
 * normalized value (0.0 to 1.0).
 */
float
automatable_real_val_to_normalized (
  Automatable * a,
  float         real_val)
{
  Plugin * plugin;
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      plugin = a->port->plugin;
      if (plugin->descr->protocol == PROT_LV2)
        {
          Lv2Control * ctrl = a->control;
          float normalized_val;
          if (ctrl->is_logarithmic)
            {
              /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
              normalized_val =
                logf (real_val / a->minf) /
                (a->maxf / a->minf);
            }
          else if (ctrl->is_toggle)
            {
              normalized_val = real_val;
            }
          else
            {
              normalized_val =
                (a->sizef -
                  (a->maxf - real_val)) / a->sizef;
            }
          return normalized_val;
        }
      g_warn_if_reached ();
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      return real_val;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      return
        (float)
        math_get_fader_val_from_amp (real_val);
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      return real_val;
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      return real_val;
    }
  return -1.f;
}

/**
 * Updates the actual value.
 *
 * The given value is always a normalized 0.0-1.0
 * value and must be translated to the actual value
 * before setting it.
 *
 * @param automating 1 if this is from an automation
 *   event. This will set Lv2Port's automating field
 *   to 1 which will cause the plugin to receive
 *   a UI event for this change.
 */
void
automatable_set_val_from_normalized (
  Automatable * a,
  float         val,
  int           automating)
{
  /*g_warn_if_fail (val <= 1.f && val >= 0.f);*/
  Plugin * plugin;
  Channel * ch;
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      plugin = a->port->plugin;
      if (plugin->descr->protocol == PROT_LV2)
        {
          Lv2Control * ctrl = a->control;
          float real_val;
          if (ctrl->is_logarithmic)
            {
              /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
              real_val =
                a->minf *
                  powf (a->maxf / a->minf, val);
            }
          else if (ctrl->is_toggle)
            {
              real_val = val >= 0.5f ? 1.f : 0.f;
            }
          else
            {
              real_val =
                a->minf + val * (a->maxf - a->minf);
            }
          ctrl->plugin->
            ports[ctrl->index].control = real_val;
          ctrl->plugin->
            ports[ctrl->index].automating =
              automating;
          a->port->base_value = real_val;
        }
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      plugin = a->port->plugin;
      plugin->enabled = val > 0.5f;
      break;
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      ch = track_get_channel (a->track);
      fader_set_amp (
        &ch->fader,
        (float)
        math_get_amp_val_from_fader (val));
      break;
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      track_set_muted (a->track,
                      val > 0.5f,
                      0);
      break;
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      ch = track_get_channel (a->track);
      channel_set_pan (ch, val);
      break;
    }
  EVENTS_PUSH (ET_AUTOMATION_VALUE_CHANGED,
               a);
}

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automatable_get_automation_track (Automatable * automatable)
{
  Track * track = automatable->track;
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);

  AutomationTrack * at;
  for (int i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];
      if (at->automatable == automatable)
        return at;
    }
  return NULL;
}

void
automatable_free (Automatable * self)
{
  /* TODO go through every track and plugins
   * and set associated automatables to NULL */
  g_free (self->label);

  free (self);
}
