/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 2017 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef __GTK_FISHBOWL_H__
#define __GTK_FISHBOWL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_FISHBOWL (gtk_fishbowl_get_type ())
#define GTK_FISHBOWL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FISHBOWL, GtkFishbowl))
#define GTK_FISHBOWL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FISHBOWL, GtkFishbowlClass))
#define GTK_IS_FISHBOWL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FISHBOWL))
#define GTK_IS_FISHBOWL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FISHBOWL))
#define GTK_FISHBOWL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FISHBOWL, GtkFishbowlClass))

typedef struct _GtkFishbowl      GtkFishbowl;
typedef struct _GtkFishbowlClass GtkFishbowlClass;

typedef GtkWidget * (*GtkFishCreationFunc) (void);

struct _GtkFishbowl
{
  GtkContainer parent;
};

struct _GtkFishbowlClass
{
  GtkContainerClass parent_class;
};

GType
gtk_fishbowl_get_type (void) G_GNUC_CONST;

GtkWidget *
gtk_fishbowl_new (void);

guint
gtk_fishbowl_get_count (GtkFishbowl * fishbowl);
void
gtk_fishbowl_set_count (GtkFishbowl * fishbowl, guint count);
gboolean
gtk_fishbowl_get_animating (GtkFishbowl * fishbowl);
void
gtk_fishbowl_set_animating (GtkFishbowl * fishbowl, gboolean animating);
gboolean
gtk_fishbowl_get_benchmark (GtkFishbowl * fishbowl);
void
gtk_fishbowl_set_benchmark (GtkFishbowl * fishbowl, gboolean animating);
double
gtk_fishbowl_get_framerate (GtkFishbowl * fishbowl);
gint64
gtk_fishbowl_get_update_delay (GtkFishbowl * fishbowl);
void
gtk_fishbowl_set_update_delay (GtkFishbowl * fishbowl, gint64 update_delay);
void
gtk_fishbowl_set_creation_func (
  GtkFishbowl *       fishbowl,
  GtkFishCreationFunc creation_func);

G_END_DECLS

#include <math.h>

#include "config.h"

typedef struct _GtkFishbowlPrivate GtkFishbowlPrivate;
typedef struct _GtkFishbowlChild   GtkFishbowlChild;

struct _GtkFishbowlPrivate
{
  GtkFishCreationFunc creation_func;
  GList *             children;
  guint               count;

  gint64 last_frame_time;
  gint64 update_delay;
  guint  tick_id;

  double framerate;
  int    last_benchmark_change;

  guint benchmark : 1;
};

struct _GtkFishbowlChild
{
  GtkWidget * widget;
  double      x;
  double      y;
  double      dx;
  double      dy;
};

enum
{
  PROP_0,
  PROP_ANIMATING,
  PROP_BENCHMARK,
  PROP_COUNT,
  PROP_FRAMERATE,
  PROP_UPDATE_DELAY,
  NUM_PROPERTIES
};

static GParamSpec * props[NUM_PROPERTIES] = {
  NULL,
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkFishbowl, gtk_fishbowl, GTK_TYPE_CONTAINER)

static void
gtk_fishbowl_init (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  gtk_widget_set_has_window (GTK_WIDGET (fishbowl), FALSE);

  priv->update_delay = G_USEC_PER_SEC;
}

/**
 * gtk_fishbowl_new:
 *
 * Creates a new #GtkFishbowl.
 *
 * Returns: a new #GtkFishbowl.
 */
GtkWidget *
gtk_fishbowl_new (void)
{
  return g_object_new (GTK_TYPE_FISHBOWL, NULL);
}

static void
gtk_fishbowl_get_preferred_width (GtkWidget * widget, int * minimum, int * natural)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (widget);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GList *              children;
  gint                 child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (!gtk_widget_get_visible (child->widget))
        continue;

      gtk_widget_get_preferred_width (child->widget, &child_min, &child_nat);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static void
gtk_fishbowl_get_preferred_height (
  GtkWidget * widget,
  int *       minimum,
  int *       natural)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (widget);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GList *              children;
  gint                 child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      int min_width;

      child = children->data;

      if (!gtk_widget_get_visible (child->widget))
        continue;

      gtk_widget_get_preferred_width (child->widget, &min_width, NULL);
      gtk_widget_get_preferred_height_for_width (
        child->widget, min_width, &child_min, &child_nat);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static void
gtk_fishbowl_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (widget);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GtkAllocation        child_allocation;
  GtkRequisition       child_requisition;
  GList *              children;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (!gtk_widget_get_visible (child->widget))
        continue;

      gtk_widget_get_preferred_size (child->widget, &child_requisition, NULL);
      child_allocation.x =
        allocation->x
        + round (child->x * (allocation->width - child_requisition.width));
      child_allocation.y =
        allocation->y
        + round (child->y * (allocation->height - child_requisition.height));
      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;

      gtk_widget_size_allocate (child->widget, &child_allocation);
    }
}

static double
new_speed (void)
{
  /* 5s to 50s to cross screen seems fair */
  return g_random_double_range (0.02, 0.2);
}

static void
gtk_fishbowl_add (GtkContainer * container, GtkWidget * widget)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (container);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child_info;

  g_return_if_fail (GTK_IS_FISHBOWL (fishbowl));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  child_info = g_new0 (GtkFishbowlChild, 1);
  child_info->widget = widget;
  child_info->x = 0;
  child_info->y = 0;
  child_info->dx = new_speed ();
  child_info->dy = new_speed ();

  gtk_widget_set_parent (widget, GTK_WIDGET (fishbowl));

  priv->children = g_list_prepend (priv->children, child_info);
  priv->count++;
  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_COUNT]);
}

static void
gtk_fishbowl_remove (GtkContainer * container, GtkWidget * widget)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (container);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GtkWidget *          widget_bowl = GTK_WIDGET (fishbowl);
  GList *              children;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child->widget == widget)
        {
          gboolean was_visible = gtk_widget_get_visible (widget);

          gtk_widget_unparent (widget);

          priv->children = g_list_remove_link (priv->children, children);
          g_list_free (children);
          g_free (child);

          if (was_visible && gtk_widget_get_visible (widget_bowl))
            gtk_widget_queue_resize (widget_bowl);

          priv->count--;
          g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_COUNT]);
          break;
        }
    }
}

static void
gtk_fishbowl_forall (
  GtkContainer * container,
  gboolean       include_internals,
  GtkCallback    callback,
  gpointer       callback_data)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (container);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GList *              children;

  if (!include_internals)
    return;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (*callback) (child->widget, callback_data);
    }
}

static void
gtk_fishbowl_dispose (GObject * object)
{
  GtkFishbowl * fishbowl = GTK_FISHBOWL (object);

  gtk_fishbowl_set_animating (fishbowl, FALSE);
  gtk_fishbowl_set_count (fishbowl, 0);

  G_OBJECT_CLASS (gtk_fishbowl_parent_class)->dispose (object);
}

static void
gtk_fishbowl_set_property (
  GObject *      object,
  guint          prop_id,
  const GValue * value,
  GParamSpec *   pspec)
{
  GtkFishbowl * fishbowl = GTK_FISHBOWL (object);

  switch (prop_id)
    {
    case PROP_ANIMATING:
      gtk_fishbowl_set_animating (fishbowl, g_value_get_boolean (value));
      break;

    case PROP_BENCHMARK:
      gtk_fishbowl_set_benchmark (fishbowl, g_value_get_boolean (value));
      break;

    case PROP_COUNT:
      gtk_fishbowl_set_count (fishbowl, g_value_get_uint (value));
      break;

    case PROP_UPDATE_DELAY:
      gtk_fishbowl_set_update_delay (fishbowl, g_value_get_int64 (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_fishbowl_get_property (
  GObject *    object,
  guint        prop_id,
  GValue *     value,
  GParamSpec * pspec)
{
  GtkFishbowl * fishbowl = GTK_FISHBOWL (object);

  switch (prop_id)
    {
    case PROP_ANIMATING:
      g_value_set_boolean (value, gtk_fishbowl_get_animating (fishbowl));
      break;

    case PROP_BENCHMARK:
      g_value_set_boolean (value, gtk_fishbowl_get_benchmark (fishbowl));
      break;

    case PROP_COUNT:
      g_value_set_uint (value, gtk_fishbowl_get_count (fishbowl));
      break;

    case PROP_FRAMERATE:
      g_value_set_double (value, gtk_fishbowl_get_framerate (fishbowl));
      break;

    case PROP_UPDATE_DELAY:
      g_value_set_int64 (value, gtk_fishbowl_get_update_delay (fishbowl));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_fishbowl_class_init (GtkFishbowlClass * klass)
{
  GObjectClass *      object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *    widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass * container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose = gtk_fishbowl_dispose;
  object_class->set_property = gtk_fishbowl_set_property;
  object_class->get_property = gtk_fishbowl_get_property;

  widget_class->get_preferred_width = gtk_fishbowl_get_preferred_width;
  widget_class->get_preferred_height = gtk_fishbowl_get_preferred_height;
  widget_class->size_allocate = gtk_fishbowl_size_allocate;

  container_class->add = gtk_fishbowl_add;
  container_class->remove = gtk_fishbowl_remove;
  container_class->forall = gtk_fishbowl_forall;

  props[PROP_ANIMATING] = g_param_spec_boolean (
    "animating", "animating", "Whether children are moving around", FALSE,
    G_PARAM_READWRITE);

  props[PROP_BENCHMARK] = g_param_spec_boolean (
    "benchmark", "Benchmark",
    "Adapt the count property to hit the maximum framerate", FALSE,
    G_PARAM_READWRITE);

  props[PROP_COUNT] = g_param_spec_uint (
    "count", "Count", "Number of widgets", 0, G_MAXUINT, 0, G_PARAM_READWRITE);

  props[PROP_FRAMERATE] = g_param_spec_double (
    "framerate", "Framerate", "Framerate of this widget in frames per second",
    0, G_MAXDOUBLE, 0, G_PARAM_READABLE);

  props[PROP_UPDATE_DELAY] = g_param_spec_int64 (
    "update-delay", "Update delay", "Number of usecs between updates", 0,
    G_MAXINT64, G_USEC_PER_SEC, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
}

guint
gtk_fishbowl_get_count (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  return priv->count;
}

void
gtk_fishbowl_set_count (GtkFishbowl * fishbowl, guint count)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  if (priv->count == count)
    return;

  g_object_freeze_notify (G_OBJECT (fishbowl));

  while (priv->count > count)
    {
      gtk_fishbowl_remove (
        GTK_CONTAINER (fishbowl),
        ((GtkFishbowlChild *) priv->children->data)->widget);
    }

  while (priv->count < count)
    {
      GtkWidget * new_widget;

      new_widget = priv->creation_func ();

      gtk_widget_show (new_widget);

      gtk_fishbowl_add (GTK_CONTAINER (fishbowl), new_widget);
    }

  g_object_thaw_notify (G_OBJECT (fishbowl));
}

gboolean
gtk_fishbowl_get_benchmark (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  return priv->benchmark;
}

void
gtk_fishbowl_set_benchmark (GtkFishbowl * fishbowl, gboolean benchmark)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  if (priv->benchmark == benchmark)
    return;

  priv->benchmark = benchmark;
  if (!benchmark)
    priv->last_benchmark_change = 0;

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_BENCHMARK]);
}

gboolean
gtk_fishbowl_get_animating (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  return priv->tick_id != 0;
}

static gint64
guess_refresh_interval (GdkFrameClock * frame_clock)
{
  gint64 interval;
  gint64 i;

  interval = G_MAXINT64;

  for (
    i = gdk_frame_clock_get_history_start (frame_clock);
    i < gdk_frame_clock_get_frame_counter (frame_clock); i++)
    {
      GdkFrameTimings *t, *before;
      gint64           ts, before_ts;

      t = gdk_frame_clock_get_timings (frame_clock, i);
      before = gdk_frame_clock_get_timings (frame_clock, i - 1);
      if (t == NULL || before == NULL)
        continue;

      ts = gdk_frame_timings_get_frame_time (t);
      before_ts = gdk_frame_timings_get_frame_time (before);
      if (ts == 0 || before_ts == 0)
        continue;

      interval = MIN (interval, ts - before_ts);
    }

  if (interval == G_MAXINT64)
    return 0;

  return interval;
}

static void
gtk_fishbowl_do_update (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GdkFrameClock *      frame_clock;
  GdkFrameTimings *    start, *end;
  gint64               start_counter, end_counter;
  gint64               n_frames, expected_frames;
  gint64               start_timestamp, end_timestamp;
  gint64               interval;

  frame_clock = gtk_widget_get_frame_clock (GTK_WIDGET (fishbowl));
  if (frame_clock == NULL)
    return;

  start_counter = gdk_frame_clock_get_history_start (frame_clock);
  end_counter = gdk_frame_clock_get_frame_counter (frame_clock);
  start = gdk_frame_clock_get_timings (frame_clock, start_counter);
  for (
    end = gdk_frame_clock_get_timings (frame_clock, end_counter);
    end_counter > start_counter && end != NULL
    && !gdk_frame_timings_get_complete (end);
    end = gdk_frame_clock_get_timings (frame_clock, end_counter))
    end_counter--;
  if (end_counter - start_counter < 4)
    return;

  start_timestamp = gdk_frame_timings_get_presentation_time (start);
  end_timestamp = gdk_frame_timings_get_presentation_time (end);
  if (start_timestamp == 0 || end_timestamp == 0)
    {
      start_timestamp = gdk_frame_timings_get_frame_time (start);
      end_timestamp = gdk_frame_timings_get_frame_time (end);
    }

  n_frames = end_counter - start_counter;
  priv->framerate =
    ((double) n_frames) * G_USEC_PER_SEC / (end_timestamp - start_timestamp);
  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_FRAMERATE]);

  if (!priv->benchmark)
    return;

  interval = gdk_frame_timings_get_refresh_interval (end);
  if (interval == 0)
    {
      interval = guess_refresh_interval (frame_clock);
      if (interval == 0)
        return;
    }
  expected_frames =
    round ((double) (end_timestamp - start_timestamp) / interval);

  if (n_frames >= expected_frames)
    {
      if (priv->last_benchmark_change > 0)
        priv->last_benchmark_change *= 2;
      else
        priv->last_benchmark_change = 1;
    }
  else if (n_frames + 1 < expected_frames)
    {
      if (priv->last_benchmark_change < 0)
        priv->last_benchmark_change--;
      else
        priv->last_benchmark_change = -1;
    }
  else
    {
      priv->last_benchmark_change = 0;
    }

  gtk_fishbowl_set_count (
    fishbowl, MAX (1, (int) priv->count + priv->last_benchmark_change));
}

static gboolean
gtk_fishbowl_tick (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        unused)
{
  GtkFishbowl *        fishbowl = GTK_FISHBOWL (widget);
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);
  GtkFishbowlChild *   child;
  GList *              l;
  gint64               frame_time, elapsed;
  gboolean             do_update;

  frame_time =
    gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (widget));
  elapsed = frame_time - priv->last_frame_time;
  do_update =
    frame_time / priv->update_delay
    != priv->last_frame_time / priv->update_delay;
  priv->last_frame_time = frame_time;

  /* last frame was 0, so we're just starting to animate */
  if (elapsed == frame_time)
    return G_SOURCE_CONTINUE;

  for (l = priv->children; l; l = l->next)
    {
      child = l->data;

      child->x += child->dx * ((double) elapsed / G_USEC_PER_SEC);
      child->y += child->dy * ((double) elapsed / G_USEC_PER_SEC);

      if (child->x <= 0)
        {
          child->x = 0;
          child->dx = new_speed ();
        }
      else if (child->x >= 1)
        {
          child->x = 1;
          child->dx = -new_speed ();
        }

      if (child->y <= 0)
        {
          child->y = 0;
          child->dy = new_speed ();
        }
      else if (child->y >= 1)
        {
          child->y = 1;
          child->dy = -new_speed ();
        }
    }

  gtk_widget_queue_allocate (widget);

  if (do_update)
    gtk_fishbowl_do_update (fishbowl);

  return G_SOURCE_CONTINUE;
}

void
gtk_fishbowl_set_animating (GtkFishbowl * fishbowl, gboolean animating)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  if (gtk_fishbowl_get_animating (fishbowl) == animating)
    return;

  if (animating)
    {
      priv->tick_id = gtk_widget_add_tick_callback (
        GTK_WIDGET (fishbowl), gtk_fishbowl_tick, NULL, NULL);
    }
  else
    {
      priv->last_frame_time = 0;
      gtk_widget_remove_tick_callback (GTK_WIDGET (fishbowl), priv->tick_id);
      priv->tick_id = 0;
      priv->framerate = 0;
      g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_FRAMERATE]);
    }

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_ANIMATING]);
}

double
gtk_fishbowl_get_framerate (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  return priv->framerate;
}

gint64
gtk_fishbowl_get_update_delay (GtkFishbowl * fishbowl)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  return priv->update_delay;
}

void
gtk_fishbowl_set_update_delay (GtkFishbowl * fishbowl, gint64 update_delay)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  if (priv->update_delay == update_delay)
    return;

  priv->update_delay = update_delay;

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_UPDATE_DELAY]);
}

void
gtk_fishbowl_set_creation_func (
  GtkFishbowl *       fishbowl,
  GtkFishCreationFunc creation_func)
{
  GtkFishbowlPrivate * priv = gtk_fishbowl_get_instance_private (fishbowl);

  g_object_freeze_notify (G_OBJECT (fishbowl));

  gtk_fishbowl_set_count (fishbowl, 0);
  priv->last_benchmark_change = 0;

  priv->creation_func = creation_func;

  gtk_fishbowl_set_count (fishbowl, 1);

  g_object_thaw_notify (G_OBJECT (fishbowl));
}

#endif /* __GTK_FISHBOWL_H__ */
