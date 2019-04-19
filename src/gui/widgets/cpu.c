/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#include "config.h"

#include "audio/engine.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/cpu.h"
#include "project.h"

#include <stdio.h>
#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/loadavg.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

G_DEFINE_TYPE (CpuWidget,
               cpu_widget,
               GTK_TYPE_DRAWING_AREA)

/**
 * In microseconds.
 */
#define TIME_TO_UPDATE_CPU_LOAD 600000
static gint64 last_time_updated_cpu = 0;
static gint64 last_time_updated_dsp = 0;

#define BAR_LENGTH 12
#define BAR_WIDTH 3
#define NUM_BARS 12

/**
 * Taken from
 * http://zetcode.com/gui/gtk2/customwidget/
 */
static int
cpu_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  CpuWidget * self)
{
  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /*cairo_translate(cr, 0, 7);*/

  cairo_set_source_rgba(cr, 0, 0, 0, 0.1);
  cairo_paint(cr);

  cairo_text_extents_t te;
  cairo_set_source_rgb(cr, 0.6, 1.0, 0);
  cairo_select_font_face (cr, "Segment7",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);
  cairo_text_extents (cr, "CPU", &te);

  char * text = g_strdup ("CPU");
  cairo_move_to (
    cr, 4, 4 + te.height);
  cairo_show_text (cr, text);
  g_free (text);
  text = g_strdup ("DSP");
  cairo_move_to (
    cr, 4, 4 + 2 + te.height * 2);
  cairo_show_text (cr, text);
  g_free (text);

  int limit, i;

  /* CPU */
  limit =
    (self->cpu * NUM_BARS) / 100;

  for (i = 1; i <= NUM_BARS; i++)
    {
      if (i <= limit)
        cairo_set_source_rgb(cr, 0.6, 1.0, 0);
      else
        cairo_set_source_rgb(cr, 0.2, 0.4, 0);

      cairo_rectangle(
        cr,
        te.width + 2 + i * 4,
        4,
        BAR_WIDTH,
        BAR_LENGTH);
      cairo_fill(cr);
    }

  /* DSP */
  limit =
    (self->dsp * NUM_BARS) / 100;

  for (i = 1; i <= NUM_BARS; i++)
    {
      if (i <= limit)
        cairo_set_source_rgb(cr, 0.6, 1.0, 0);
      else
        cairo_set_source_rgb(cr, 0.2, 0.4, 0);

      cairo_rectangle(
        cr,
        te.width + 2 + i * 4,
        4 + te.height + 2,
        BAR_WIDTH,
        BAR_LENGTH);
      cairo_fill(cr);
    }

	return 0;
}

unsigned long prev_total, prev_idle;

/**
 * Refreshes DSP load percentage.
 *
 * This is a running average of the time it takes to
 * execute a full process cycle for all JACK clients
 * as a percentage of the real time available per
 * cycle
 */
gboolean
refresh_dsp_load (GtkWidget * widget,
                  GdkFrameClock *frame_clock,
                  gpointer user_data)
{
  gint64 curr_time = g_get_monotonic_time ();
  if (curr_time - last_time_updated_dsp <
      TIME_TO_UPDATE_CPU_LOAD)
    return G_SOURCE_CONTINUE;

  CpuWidget * self = Z_CPU_WIDGET (widget);

#ifdef __APPLE__
  /* TODO engine not working yet */
#else
  gint64 block_latency =
    (AUDIO_ENGINE->block_length * 1000000) /
    AUDIO_ENGINE->sample_rate;
  self->dsp =
    AUDIO_ENGINE->max_time_taken * 100.0 /
    block_latency;
#endif

  AUDIO_ENGINE->max_time_taken = 0;
  last_time_updated_dsp = curr_time;

  return G_SOURCE_CONTINUE;
}

static long double a[4], b[4] = {0,0,0,0}, loadavg;

#ifdef _WIN32
static unsigned long long
FileTimeToInt64 (const FILETIME * ft)
{
  return
    (((unsigned long long)(ft->dwHighDateTime))<<32) |
    ((unsigned long long)ft->dwLowDateTime);
}
#endif

/**
 * Refreshes CPU load percentage.
 */
gboolean
refresh_cpu_load (GtkWidget * widget,
                  GdkFrameClock *frame_clock,
                  void * data)
{
  gint64 curr_time = g_get_monotonic_time ();
  if (curr_time - last_time_updated_cpu <
      TIME_TO_UPDATE_CPU_LOAD)
    return G_SOURCE_CONTINUE;

  CpuWidget * self = Z_CPU_WIDGET (widget);

#ifdef HAVE_LIBGTOP
  glibtop_cpu cpu;
  glibtop_get_cpu (&cpu);
  self->cpu =
    100 -
    (float) (cpu.idle - prev_idle) /
    (float) (cpu.total - prev_total) * 100;

  last_time_updated_cpu = curr_time;
  prev_total = cpu.total;
  prev_idle = cpu.idle;
#elif defined(__linux__)
  /* ======= non libgtop ====== */
  FILE *fp;
  char dump[50];

  fp = fopen("/proc/stat","r");
  fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
  fclose(fp);

  loadavg = ((a[0]+a[1]+a[2]) - (b[0]+b[1]+b[2])) / ((a[0]+a[1]+a[2]+a[3]) - (b[0]+b[1]+b[2]+b[3]));
  self->cpu = loadavg * 100;

  b[0] = a[0];
  b[1] = a[1];
  b[2] = a[2];
  b[3] = a[3];
  /* ========== end ========= */
#elif defined(_WIN32)
  FILETIME idleTime, kernelTime, userTime;

  if (GetSystemTimes (
        &idleTime, &kernelTime, &userTime))
    {
      unsigned long long idleTicks =
        FileTimeToInt64(&idleTime);
      unsigned long long totalTicks =
        FileTimeToInt64(&kernelTime) +
        FileTimeToInt64(&userTime);
      static unsigned long long _previousTotalTicks = 0;
      static unsigned long long _previousIdleTicks = 0;

      unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
      unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

      self->cpu = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

      _previousTotalTicks = totalTicks;
      _previousIdleTicks  = idleTicks;
    }
  else
    self->cpu = -1.0f;
#endif

  char * ttip =
    g_strdup_printf (
      "CPU: %d%%\nDSP: %d%%",
      self->cpu, self->dsp);
  gtk_widget_set_tooltip_text (
    widget, ttip);
  g_free (ttip);

  return G_SOURCE_CONTINUE;
}

static gboolean
on_motion (GtkWidget * widget, GdkEvent *event)
{
  CpuWidget * self = Z_CPU_WIDGET (widget);

  if (gdk_event_get_event_type (event) ==
        GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
      bot_bar_change_status (
        "CPU Usage");
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

/**
 * Creates a new Cpu widget and binds it to the given value.
 */
void
cpu_widget_setup (
  CpuWidget * self)
{
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    refresh_cpu_load,
    NULL,
    NULL);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    refresh_dsp_load,
    NULL,
    NULL);
}

static void
cpu_widget_init (CpuWidget * self)
{
  self->cpu = 0;
  self->dsp = 0;

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (cpu_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
}

static void
cpu_widget_class_init (CpuWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "cpu");
}
