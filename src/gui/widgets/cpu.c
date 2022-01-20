/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include "zrythm-config.h"

#include <stdio.h>

#include "audio/engine.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/cpu.h"
#include "project.h"
#include "utils/cpu_windows.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"

#ifdef __APPLE__
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#endif

#include "zrythm_app.h"

G_DEFINE_TYPE (
  CpuWidget, cpu_widget, GTK_TYPE_WIDGET)

#define BAR_HEIGHT 12
#define BAR_WIDTH 3
#define NUM_BARS 12
#define PADDING 2
#define ICON_SIZE BAR_HEIGHT
#define TOTAL_H \
  (PADDING * 3 + BAR_HEIGHT * 2)
#define TOTAL_W \
  (ICON_SIZE + PADDING * 3 + NUM_BARS * PADDING * 2)

/**
 * Taken from
 * http://zetcode.com/gui/gtk2/customwidget/
 */
static void
cpu_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  CpuWidget * self = Z_CPU_WIDGET (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  GdkRGBA active_color;
  gdk_rgba_parse (&active_color, "#33D17A");
    /*Z_GDK_RGBA_INIT (0.6, 1.0, 0.0, 1.0);*/
  GdkRGBA inactive_color = active_color;
  inactive_color.alpha = 0.4f;
    /*Z_GDK_RGBA_INIT (0.2, 0.4, 0.0, 1.0);*/
  GdkRGBA color = active_color;

  graphene_matrix_t color_matrix;
  graphene_matrix_init_from_float (
    &color_matrix,
    (float[16]) {
      1, 1, 1, 0,
      1, 1, 1, 0,
      1, 1, 1, 0,
      0, 0, 0, color.alpha });
  graphene_vec4_t color_offset;
  graphene_vec4_init (
    &color_offset,
    color.red, color.green, color.blue, 0);

  gtk_snapshot_push_color_matrix (
    snapshot, &color_matrix, &color_offset);

  gtk_snapshot_append_texture (
    snapshot, self->cpu_texture,
    &GRAPHENE_RECT_INIT (
      PADDING, PADDING, ICON_SIZE, ICON_SIZE));

  gtk_snapshot_append_texture (
    snapshot, self->dsp_texture,
    &GRAPHENE_RECT_INIT (
      PADDING, PADDING * 2 + ICON_SIZE,
      ICON_SIZE, ICON_SIZE));

  gtk_snapshot_pop (snapshot);

  int limit, i;

  /* CPU */
  limit =
    (self->cpu * NUM_BARS) / 100;

  for (i = 1; i <= NUM_BARS; i++)
    {
      if (i <= limit)
        color = active_color;
      else
        color = inactive_color;

      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          ICON_SIZE + PADDING + i * PADDING * 2,
          PADDING,
          BAR_WIDTH,
          BAR_HEIGHT));
    }

  /* DSP */
  limit =
    (self->dsp * NUM_BARS) / 100;

  for (i = 1; i <= NUM_BARS; i++)
    {
      if (i <= limit)
        color = active_color;
      else
        color = inactive_color;

      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          ICON_SIZE + PADDING + i * PADDING * 2,
          PADDING * 2 + BAR_HEIGHT,
          BAR_WIDTH,
          BAR_HEIGHT));
    }
}

/**
 * Refreshes DSP load percentage.
 *
 * This is a running average of the time it takes to
 * execute a full process cycle for all JACK clients
 * as a percentage of the real time available per
 * cycle
 */
static gboolean
refresh_dsp_load (
  CpuWidget * self)
{
  if (!PROJECT || !AUDIO_ENGINE)
    {
      return G_SOURCE_CONTINUE;
    }

  if (g_atomic_int_get (&AUDIO_ENGINE->run))
    {
      gint64 block_latency =
        (AUDIO_ENGINE->block_length * 1000000) /
        AUDIO_ENGINE->sample_rate;
      self->dsp =
        (int)
        ((double) AUDIO_ENGINE->max_time_taken *
           100.0 /
         (double) block_latency);
    }
  else
    self->dsp = 0;

  AUDIO_ENGINE->max_time_taken = 0;

  return G_SOURCE_CONTINUE;
}

#if defined(__linux__)
static long double a[4], b[4] = {0,0,0,0}, loadavg;
#endif

#ifdef __APPLE__
static unsigned long long _previousTotalTicks = 0;
static unsigned long long _previousIdleTicks = 0;

float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
  unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
  unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;
  float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);
  _previousTotalTicks = totalTicks;
  _previousIdleTicks  = idleTicks;
  return ret;
}
#endif

/**
 * Refreshes CPU load percentage.
 */
static int
refresh_cpu_load (
  CpuWidget * self)
{
#if defined(__linux__)

  /* ======= non libgtop ====== */
  FILE *fp;

  fp = fopen("/proc/stat","r");
  fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
  fclose(fp);

  loadavg = ((a[0]+a[1]+a[2]) - (b[0]+b[1]+b[2])) / ((a[0]+a[1]+a[2]+a[3]) - (b[0]+b[1]+b[2]+b[3]));
  self->cpu = (int) ((double) loadavg * (double) 100);

  b[0] = a[0];
  b[1] = a[1];
  b[2] = a[2];
  b[3] = a[3];
  /* ========== end ========= */

#elif defined(_WOE32)

  self->cpu = cpu_windows_get_usage (-1);

#elif defined (__APPLE__)

  host_cpu_load_info_data_t cpuinfo;
   mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
   if (host_statistics (
        mach_host_self(), HOST_CPU_LOAD_INFO,
        (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
   {
      unsigned long long totalTicks = 0;
      for(int i=0; i<CPU_STATE_MAX; i++) totalTicks += cpuinfo.cpu_ticks[i];
      self->cpu =
        (int)
        (CalculateCPULoad(cpuinfo.cpu_ticks[CPU_STATE_IDLE], totalTicks) * 100.f);
   }
   else
     {
       g_warn_if_reached ();
       self->cpu = 0;
     }

#endif

  char ttip[100];
  sprintf (
    ttip,
    "CPU: %d%%\nDSP: %d%%",
    self->cpu, self->dsp);
  /* FIXME reenable after GTK #4451 is fixed */
  /*gtk_widget_set_tooltip_text (*/
    /*(GtkWidget *) self, ttip);*/
  gtk_widget_queue_draw ((GtkWidget *) self);

  return G_SOURCE_CONTINUE;
}

static void
on_motion_enter (
  GtkEventControllerMotion * controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  CpuWidget * self = Z_CPU_WIDGET (user_data);
  gtk_widget_set_state_flags (
    GTK_WIDGET (self),
    GTK_STATE_FLAG_PRELIGHT, 0);
}

static void
on_motion_leave (
  GtkEventControllerMotion * controller,
  gpointer                   user_data)
{
  CpuWidget * self = Z_CPU_WIDGET (user_data);
  gtk_widget_unset_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

/**
 * Creates a new Cpu widget and binds it to the
 * given value.
 */
void
cpu_widget_setup (
  CpuWidget * self)
{
  self->cpu_source_id =
    g_timeout_add_seconds (
      1, (GSourceFunc) refresh_cpu_load, self);
  self->dsp_source_id =
    g_timeout_add_seconds (
      1, (GSourceFunc) refresh_dsp_load, self);
}

static void
finalize (
  CpuWidget * self)
{
  /* remove the timeout callbacks */
  g_source_remove (
    self->cpu_source_id);
  g_source_remove (
    self->dsp_source_id);

  object_free_w_func_and_null (
    g_object_unref, self->cpu_texture);
  object_free_w_func_and_null (
    g_object_unref, self->dsp_texture);

  G_OBJECT_CLASS (
    cpu_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
cpu_widget_init (CpuWidget * self)
{
  self->cpu = 0;
  self->dsp = 0;

  self->cpu_texture =
    z_gdk_texture_new_from_icon_name (
      "ext-iconfinder_cpu_2561419",
      ICON_SIZE, ICON_SIZE, 1);
  self->dsp_texture =
    z_gdk_texture_new_from_icon_name (
      "font-awesome-wave-square-solid",
      ICON_SIZE, ICON_SIZE, 1);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), TOTAL_W, TOTAL_H);

  /* connect signals */
  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_motion_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), motion_controller);
}

static void
cpu_widget_class_init (
  CpuWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = cpu_snapshot;
  gtk_widget_class_set_css_name (wklass, "cpu");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}
