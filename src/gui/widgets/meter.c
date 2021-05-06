/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/meter.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/fader.h"
#include "utils/error.h"
#include "utils/math.h"
#include "utils/resources.h"

#include <epoxy/gl.h>

#include <nanovg/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/nanovg_gl.h>

G_DEFINE_TYPE (
  MeterWidget, meter_widget, GTK_TYPE_GL_AREA)


/* position and color information for each vertex */
struct vertex_info {
  float position[3];
  float color[3];
};

/* the vertex data is constant */
static const struct vertex_info vertex_data[] = {
  { {  0.0f,  0.500f, 0.0f }, { 1.f, 0.f, 0.f } },
  { {  0.5f, -0.366f, 0.0f }, { 0.f, 1.f, 0.f } },
  { { -0.5f, -0.366f, 0.0f }, { 0.f, 0.f, 1.f } },
};

static bool
meter_render (
  GtkGLArea *    area,
  GdkGLContext * context,
  MeterWidget *  self)
{
  /* clear the viewport; the viewport is
   * automatically resized when the GtkGLArea gets
   * an allocation */
  /*glClearColor (0, 0, 0, 0);*/
  glClear (GL_COLOR_BUFFER_BIT);

#if 0
  /* draw our object */
  if (self->program != 0 && self->vao != 0)
    {
      /* load our program */
      glUseProgram (self->program);

      /* update the "mvp" matrix we use in the
       * shader */
      glUniformMatrix4fv (
        (GLint) self->mvp_location, 1, GL_FALSE,
        &(self->mvp[0]));

      /* use the buffers in the VAO */
      glBindVertexArray (self->vao);

      /* draw the three vertices as a triangle */
      glDrawArrays (GL_TRIANGLES, 0, 3);

      /* we finished using the buffers and program */
      glBindVertexArray (0);
      glUseProgram (0);
    }

  /* flush the contents of the pipeline */
  glFlush ();
#endif

  int width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  /*g_message ("rendering");*/
  nvgBeginFrame (
    self->nvg, width, height, 1);

  /* get values */
  float peak = self->meter_peak;
  float meter_val = self->meter_val;

  float value_px = (float) height * meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw filled in bar */
  float width_without_padding =
    (float) (width - self->padding * 2);

  GdkRGBA bar_color = { 0, 0, 0, 1 };
  double intensity = (double) meter_val;
  const double intensity_inv = 1.0 - intensity;
  bar_color.red =
    intensity_inv * self->end_color.red +
    intensity * self->start_color.red;
  bar_color.green =
    intensity_inv * self->end_color.green +
    intensity * self->start_color.green;
  bar_color.blue =
    intensity_inv * self->end_color.blue  +
    intensity * self->start_color.blue;

  float x = self->padding;

  /* draw body with gradient */
  nvgBeginPath (self->nvg);
  nvgRect (
    self->nvg, x,
    (float) height - value_px,
    x + width_without_padding,
    value_px);
  NVGcolor icol =
    nvgRGBf (
      (float) bar_color.red, (float) bar_color.green,
      (float) bar_color.blue);
  /*NVGcolor ocol = nvgRGBf (0, 0.2f, 1);*/
  NVGcolor ocol = nvgRGB (255, 192, 0);
  NVGpaint gradient =
    nvgLinearGradient (
      self->nvg, 0, 0, width, height,
      icol, ocol);
  nvgFillPaint (self->nvg, gradient);
  nvgFill (self->nvg);
  nvgClosePath (self->nvg);

  /* draw border line */
  nvgBeginPath (self->nvg);
  nvgRect (
    self->nvg, x, 0,
    x + width_without_padding, height);
  nvgStrokeWidth (self->nvg, 1.7f);
  nvgStrokeColor (
    self->nvg, nvgRGBf (0.1f, 0.1f, 0.1f));
  nvgStroke (self->nvg);
  nvgClosePath (self->nvg);

  /* draw meter line */
  nvgBeginPath (self->nvg);
  nvgMoveTo (
    self->nvg, x, (float) height - value_px);
  nvgLineTo (
    self->nvg, x * 2 + width_without_padding,
    (float) height - value_px);
  nvgStrokeColor (
    self->nvg, nvgRGBf (0.4f, 0.1f, 0.05f));
  nvgStroke (self->nvg);
  nvgClosePath (self->nvg);

  /* draw peak */
  float peak_amp =
    math_get_amp_val_from_fader (peak);
  nvgBeginPath (self->nvg);
  float peak_px = peak * height;
  nvgMoveTo (
    self->nvg, x, height - peak_px);
  nvgLineTo (
    self->nvg, x * 2 + width_without_padding,
    height - peak_px);
  if (peak_amp > 1.f)
    {
      /* make higher peak brighter */
      nvgStrokeColor (
        self->nvg,
        nvgRGBf (0.6f + 0.4f * peak, 0.1f, 0.05f));
    }
  else
    {
      nvgStrokeColor (
        self->nvg,
        nvgRGBf (
          0.4f + 0.4f * peak,
          0.4f + 0.4f * peak,
          0.4f + 0.4f * peak));
    }
  nvgStrokeWidth (self->nvg, 2.f);
  nvgStroke (self->nvg);
  nvgClosePath (self->nvg);
  nvgEndFrame (self->nvg);

  self->last_meter_val = self->meter_val;
  self->last_meter_peak = self->meter_peak;

  return false;
}

#if 0
static int
meter_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  MeterWidget * self)
{
  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* get values */
  float peak = self->meter_peak;
  float meter_val = self->meter_val;

  float value_px = (float) height * meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw filled in bar */
  float width_without_padding =
    (float) (width - self->padding * 2);

  GdkRGBA bar_color = { 0, 0, 0, 1 };
  double intensity = (double) meter_val;
  const double intensity_inv = 1.0 - intensity;
  bar_color.red =
    intensity_inv * self->end_color.red +
    intensity * self->start_color.red;
  bar_color.green =
    intensity_inv * self->end_color.green +
    intensity * self->start_color.green;
  bar_color.blue =
    intensity_inv * self->end_color.blue  +
    intensity * self->start_color.blue;

  gdk_cairo_set_source_rgba (cr, &bar_color);

  /* use gradient */
  cairo_pattern_t * pat =
    cairo_pattern_create_linear (
      0.0, 0.0, 0.0, height);
  cairo_pattern_add_color_stop_rgba (
    pat, 0,
    bar_color.red, bar_color.green,
    bar_color.blue, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 0.5,
    bar_color.red, bar_color.green,
    bar_color.blue, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 0.75, 0, 1, 0, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 1, 0, 0.2, 1, 1);
  cairo_set_source (
    cr, pat);

  float x = self->padding;
  cairo_rectangle (
    cr, x,
    (float) height - value_px,
    x + width_without_padding,
    value_px);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* draw border line */
  cairo_set_source_rgba (
    cr, 0.1, 0.1, 0.1, 1.0);
  cairo_set_line_width (cr, 1.7);
  cairo_rectangle (
    cr, x, 0,
    x + width_without_padding, height);
  cairo_stroke(cr);

  /* draw meter line */
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (
    cr, x, (float) height - value_px);
  cairo_line_to (
    cr, x * 2 + width_without_padding,
    (float) height - value_px);
  cairo_stroke (cr);

  /* draw peak */
  float peak_amp =
    math_get_amp_val_from_fader (peak);
  if (peak_amp > 1.f)
    {
      cairo_set_source_rgba (
        /* make higher peak brighter */
        cr, 0.6 + 0.4 * (double) peak, 0.1, 0.05, 1);
    }
  else
    {
      cairo_set_source_rgba (
        /* make higher peak brighter */
        cr,
        0.4 + 0.4 * (double) peak,
        0.4 + 0.4 * (double) peak,
        0.4 + 0.4 * (double) peak,
        1);
    }
  cairo_set_line_width (cr, 2.0);
  double peak_px = (double) peak * height;
  cairo_move_to (
    cr, x, height - peak_px);
  cairo_line_to (
    cr, x * 2 + width_without_padding,
    height - peak_px);
  cairo_stroke (cr);

  self->last_meter_val = self->meter_val;
  self->last_meter_peak = self->meter_peak;

  return FALSE;
}
#endif

static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  MeterWidget * self = Z_METER_WIDGET (widget);
  GdkEventType type =
    gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
        self->hover = 0;
    }
  gtk_gl_area_queue_render (GTK_GL_AREA (self));
}

static gboolean
tick_cb (
  GtkWidget * widget,
  GdkFrameClock * frame_clock,
  MeterWidget * self)
{
  if (!math_floats_equal (
        self->meter_val, self->last_meter_val) ||
      !math_floats_equal (
        self->meter_peak, self->last_meter_peak))
    {
      gtk_gl_area_queue_render (
        GTK_GL_AREA (self));
    }

  return G_SOURCE_CONTINUE;
}

/*
 * Timeout to "run" the meter.
 */
static gboolean
meter_timeout (
  MeterWidget * self)
{
  if (GTK_IS_WIDGET (self))
    {
      if (self->meter &&
          gtk_widget_get_mapped (
            GTK_WIDGET (self)))
        {
          meter_get_value (
            self->meter, AUDIO_VALUE_FADER,
            &self->meter_val,
            &self->meter_peak);
        }
      else
        {
          /* not mapped, skipping dsp */
        }

      return G_SOURCE_CONTINUE;
    }
  else
    return G_SOURCE_REMOVE;
}

static void
on_unrealize (
  GtkWidget *   widget,
  MeterWidget * self)
{
  /* we need to ensure that the GdkGLContext is set
   * before calling GL API */
  gtk_gl_area_make_current (GTK_GL_AREA (self));

  /* destroy all the resources we created */
  glDeleteVertexArrays (1, &self->vao);
  glDeleteProgram (self->program);
}

static int
create_shader (
  int          shader_type,
  const char * source,
  GError **    error,
  guint *      shader_out)
{
  guint shader =
    glCreateShader ((GLenum) shader_type);
  glShaderSource (shader, 1, &source, NULL);
  glCompileShader (shader);

  int status;
  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len;
      glGetShaderiv (
        shader, GL_INFO_LOG_LENGTH, &log_len);

      char *buffer = g_malloc ((gsize) log_len + 1);
      glGetShaderInfoLog (
        shader, log_len, NULL, buffer);

      g_set_error (
        error, ERROR_DOMAIN_GLAREA,
        ERROR_SHADER_COMPILATION,
        "Compilation failure in %s shader: %s",
        shader_type == GL_VERTEX_SHADER ?
          "vertex" : "fragment",
        buffer);

      g_free (buffer);

      glDeleteShader (shader);
      shader = 0;
    }

  if (shader_out != NULL)
    *shader_out = shader;

  return shader != 0;
}

static int
init_shaders (
  guint *   program_out,
  guint *   mvp_location_out,
  guint *   position_location_out,
  guint *   color_location_out,
  GError ** error)
{
  GBytes * source;
  guint program = 0;
  guint mvp_location = 0;
  guint vertex = 0, fragment = 0;
  guint position_location = 0;
  guint color_location = 0;
  int status = 0;

  /* load the vertex shader */
  source =
    resources_get_gl_shader_data (
      "meter-vertex.glsl");
  if (!source)
    {
      return -1;
    }
  create_shader (
    GL_VERTEX_SHADER,
    g_bytes_get_data (source, NULL), error, &vertex);
  g_bytes_unref (source);
  if (vertex == 0)
    goto out;

  /* load the fragment shader */
  source =
    resources_get_gl_shader_data (
      "meter-fragment.glsl");
  if (!source)
    return -1;
  create_shader (
    GL_FRAGMENT_SHADER,
    g_bytes_get_data (source, NULL), error,
    &fragment);
  g_bytes_unref (source);
  if (fragment == 0)
    goto out;

  /* link the vertex and fragment shaders together */
  program = glCreateProgram ();
  glAttachShader (program, vertex);
  glAttachShader (program, fragment);
  glLinkProgram (program);

  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len = 0;
      glGetProgramiv (
        program, GL_INFO_LOG_LENGTH, &log_len);

      char *buffer = g_malloc ((gsize) log_len + 1);
      glGetProgramInfoLog (
        program, log_len, NULL, buffer);

      g_set_error (
        error, ERROR_DOMAIN_GLAREA,
        ERROR_SHADER_LINK,
        "Linking failure in program: %s", buffer);

      g_free (buffer);

      glDeleteProgram (program);
      program = 0;

      goto out;
    }

  /* get the location of the "mvp" uniform */
  mvp_location =
    (guint)
    glGetUniformLocation (program, "mvp");

  /* get the location of the "position" and "color"
   * attributes */
  position_location =
    (guint)
    glGetAttribLocation (program, "position");
  color_location =
    (guint)
    glGetAttribLocation (program, "color");

  /* the individual shaders can be detached and
   * destroyed */
  glDetachShader (program, vertex);
  glDetachShader (program, fragment);

out:
  if (vertex != 0)
    glDeleteShader (vertex);
  if (fragment != 0)
    glDeleteShader (fragment);

  if (program_out != NULL)
    *program_out = program;
  if (mvp_location_out != NULL)
    *mvp_location_out = mvp_location;
  if (position_location_out != NULL)
    *position_location_out = position_location;
  if (color_location_out != NULL)
    *color_location_out = color_location;

  if (program == 0)
    {
      return -1;
    }

  return 0;
}

static void
init_buffers (
  guint   position_index,
  guint   color_index,
  guint * vao_out)
{
  guint vao, buffer;

  /* we need to create a VAO to store the other
   * buffers */
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);

  /* this is the VBO that holds the vertex data */
  glGenBuffers (1, &buffer);
  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glBufferData (
    GL_ARRAY_BUFFER, sizeof (vertex_data),
    vertex_data, GL_STATIC_DRAW);

  /* enable and set the position attribute */
  glEnableVertexAttribArray (position_index);
  glVertexAttribPointer (
    position_index, 3, GL_FLOAT, GL_FALSE,
    sizeof (struct vertex_info),
    (GLvoid *)
    (G_STRUCT_OFFSET (
       struct vertex_info, position)));

  /* enable and set the color attribute */
  glEnableVertexAttribArray (color_index);
  glVertexAttribPointer (
    color_index, 3, GL_FLOAT, GL_FALSE,
    sizeof (struct vertex_info),
    (GLvoid *)
    (G_STRUCT_OFFSET (
       struct vertex_info, color)));

  /* reset the state; we will re-enable the VAO when
   * needed */
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);

  /* the VBO is referenced by the VAO */
  glDeleteBuffers (1, &buffer);

  if (vao_out != NULL)
    {
      *vao_out = vao;
    }
}

static void
on_realize (
  GtkWidget *   widget,
  MeterWidget * self)
{
  /* we need to ensure that the GdkGLContext is set
   * before calling GL API */
  gtk_gl_area_make_current (GTK_GL_AREA (self));

  self->nvg = nvgCreateGL3 (0);

  /* initialize the shaders and retrieve the program
   * data */
  GError * error = NULL;
  if (init_shaders (
        &self->program,
        &self->mvp_location,
        &self->position_index,
        &self->color_index,
        &error))
    {
      gtk_gl_area_set_error (
        GTK_GL_AREA (self), error);
      g_error_free (error);
      return;
    }

  /* initialize the vertex buffers */
  init_buffers (
    self->position_index, self->color_index,
    &self->vao);
}

/**
 * Creates a new Meter widget and binds it to the
 * given value.
 *
 * @param port Port this meter is for.
 */
void
meter_widget_setup (
  MeterWidget *      self,
  Port *             port,
  int                width)
{
  if (self->meter)
    {
      meter_free (self->meter);
    }
  self->meter = meter_new_for_port (port);
  self->padding = 2;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, -1);
}

static void
finalize (
  MeterWidget * self)
{
  if (self->meter)
    meter_free (self->meter);

  g_source_remove (self->source_id);

  G_OBJECT_CLASS (
    meter_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
meter_widget_init (MeterWidget * self)
{
  /*gdk_rgba_parse (&self->start_color, "#00FF66");*/
  /*gdk_rgba_parse (&self->end_color, "#00FFCC");*/
  gdk_rgba_parse (&self->start_color, "#F9CA1B");
  gdk_rgba_parse (&self->end_color, "#1DDD6A");

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "realize",
    G_CALLBACK (on_realize), self);
  g_signal_connect (
    G_OBJECT (self), "unrealize",
    G_CALLBACK (on_unrealize), self);
  g_signal_connect (
    G_OBJECT (self), "render",
    G_CALLBACK (meter_render), self);
#if 0
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (meter_draw_cb), self);
#endif
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_crossing),  self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
  self->source_id =
    g_timeout_add (
      20, (GSourceFunc) meter_timeout, self);

  gtk_gl_area_set_auto_render (
    GTK_GL_AREA (self), false);
}

static void
meter_widget_class_init (MeterWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "meter");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
