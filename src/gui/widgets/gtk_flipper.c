// SPDX-FileCopyrightText: © 2021 Benjamin Otte
// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2021 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "gui/widgets/gtk_flipper.h"

/**
 * GtkFlipper:
 *
 * `GtkFlipper` is a widget to flip or rotate a child widget.
 *
 * ![An example GtkFlipper](flipper.png)
 *
 * Widgets can be flipped horiontally and vertically, and they can
 * be rotated. The properties of the flipper only allow rotating
 * clockwise by 90°, but flipping both horizontally and vertically
 * achieves a 180° rotation, so you can do that to achieve 180°
 * and 270° rotations.
 *
 * In technical terms this widget implements the dihedral group
 * [Dih4](https://en.wikipedia.org/wiki/Examples_of_groups#dihedral_group_of_order_8)
 * for the child widget.
 *
 * The order that operations on the child are performed when enabled is:
 *
 *  * flip horizontally
 *
 *  * flip vertically
 *
 *  * rotate
 */

struct _GtkFlipper
{
  GtkWidget parent_instance;

  GtkWidget * child;

  guint flip_horizontal : 1;
  guint flip_vertical : 1;
  guint rotate : 1;
};

enum
{
  PROP_0,
  PROP_CHILD,
  PROP_FLIP_HORIZONTAL,
  PROP_FLIP_VERTICAL,
  PROP_ROTATE,

  N_PROPS
};

G_DEFINE_TYPE (GtkFlipper, gtk_flipper, GTK_TYPE_WIDGET)

static GParamSpec * properties[N_PROPS] = {
  NULL,
};

static GtkSizeRequestMode
gtk_flipper_get_request_mode (GtkWidget * widget)
{
  GtkFlipper *       self = GTK_FLIPPER (widget);
  GtkSizeRequestMode mode;

  if (!self->child)
    return GTK_SIZE_REQUEST_CONSTANT_SIZE;

  mode = gtk_widget_get_request_mode (self->child);

  if (!self->rotate)
    return mode;

  switch (mode)
    {
    case GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
      return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
    case GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
      return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
    case GTK_SIZE_REQUEST_CONSTANT_SIZE:
    default:
      return mode;
    }
}

#define OPPOSITE_ORIENTATION(x) \
  ((x) == GTK_ORIENTATION_VERTICAL \
     ? GTK_ORIENTATION_HORIZONTAL \
     : GTK_ORIENTATION_VERTICAL)

static void
gtk_flipper_measure (
  GtkWidget *    widget,
  GtkOrientation orientation,
  int            for_size,
  int *          minimum,
  int *          natural,
  int *          minimum_baseline,
  int *          natural_baseline)
{
  GtkFlipper * self = GTK_FLIPPER (widget);

  if (self->rotate)
    {
      orientation = OPPOSITE_ORIENTATION (orientation);
    }

  if (self->child && gtk_widget_get_visible (self->child))
    gtk_widget_measure (
      self->child, orientation, for_size, minimum, natural,
      minimum_baseline, natural_baseline);

  if (self->rotate || self->flip_vertical)
    {
      *minimum_baseline = -1;
      *natural_baseline = -1;
    }
}

static void
gtk_flipper_size_allocate (
  GtkWidget * widget,
  int         width,
  int         height,
  int         baseline)
{
  GtkFlipper *   self = GTK_FLIPPER (widget);
  GskTransform * transform = NULL;

  if (!self->child || !gtk_widget_get_visible (self->child))
    return;

  if (self->flip_horizontal)
    {
      transform = gsk_transform_translate (
        transform, &GRAPHENE_POINT_INIT ((float) width, 0.f));
      transform = gsk_transform_scale (transform, -1, 1);
    }

  if (self->flip_vertical)
    {
      transform = gsk_transform_translate (
        transform, &GRAPHENE_POINT_INIT (0.f, (float) height));
      transform = gsk_transform_scale (transform, 1, -1);
      baseline = -1;
    }

  if (self->rotate)
    {
      int tmp = width;
      width = height;
      height = tmp;
      transform = gsk_transform_rotate (transform, 90);
      transform = gsk_transform_translate (
        transform,
        &GRAPHENE_POINT_INIT (0.f, -(float) height));
      baseline = -1;
    }

  gtk_widget_allocate (
    self->child, width, height, baseline, transform);
}

static void
gtk_flipper_compute_expand (
  GtkWidget * widget,
  gboolean *  hexpand,
  gboolean *  vexpand)
{
  GtkFlipper * self = GTK_FLIPPER (widget);

  if (self->child)
    {
      if (self->rotate)
        {
          *hexpand = gtk_widget_compute_expand (
            self->child, GTK_ORIENTATION_HORIZONTAL);
          *vexpand = gtk_widget_compute_expand (
            self->child, GTK_ORIENTATION_VERTICAL);
        }
      else
        {
          *hexpand = gtk_widget_compute_expand (
            self->child, GTK_ORIENTATION_VERTICAL);
          *vexpand = gtk_widget_compute_expand (
            self->child, GTK_ORIENTATION_HORIZONTAL);
        }
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
gtk_flipper_dispose (GObject * object)
{
  GtkFlipper * self = GTK_FLIPPER (object);

  g_clear_pointer (&self->child, gtk_widget_unparent);

  G_OBJECT_CLASS (gtk_flipper_parent_class)->dispose (object);
}

static void
gtk_flipper_get_property (
  GObject *    object,
  guint        property_id,
  GValue *     value,
  GParamSpec * pspec)
{
  GtkFlipper * self = GTK_FLIPPER (object);

  switch (property_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    case PROP_FLIP_HORIZONTAL:
      g_value_set_boolean (value, self->flip_horizontal);
      break;

    case PROP_FLIP_VERTICAL:
      g_value_set_boolean (value, self->flip_vertical);
      break;

    case PROP_ROTATE:
      g_value_set_boolean (value, self->rotate);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (
        object, property_id, pspec);
      break;
    }
}

static void
gtk_flipper_set_property (
  GObject *      object,
  guint          property_id,
  const GValue * value,
  GParamSpec *   pspec)
{
  GtkFlipper * self = GTK_FLIPPER (object);

  switch (property_id)
    {
    case PROP_CHILD:
      gtk_flipper_set_child (self, g_value_get_object (value));
      break;

    case PROP_FLIP_HORIZONTAL:
      gtk_flipper_set_flip_horizontal (
        self, g_value_get_boolean (value));
      break;

    case PROP_FLIP_VERTICAL:
      gtk_flipper_set_flip_vertical (
        self, g_value_get_boolean (value));
      break;

    case PROP_ROTATE:
      gtk_flipper_set_rotate (
        self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (
        object, property_id, pspec);
      break;
    }
}

static void
gtk_flipper_class_init (GtkFlipperClass * klass)
{
  GtkWidgetClass * widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *   gobject_class = G_OBJECT_CLASS (klass);

  widget_class->get_request_mode =
    gtk_flipper_get_request_mode;
  widget_class->measure = gtk_flipper_measure;
  widget_class->size_allocate = gtk_flipper_size_allocate;
  widget_class->compute_expand = gtk_flipper_compute_expand;

  gobject_class->dispose = gtk_flipper_dispose;
  gobject_class->get_property = gtk_flipper_get_property;
  gobject_class->set_property = gtk_flipper_set_property;

  /**
   * GtkFlipper:child: (attributes org.gtk.Property.get=gtk_flipper_get_child org.gtk.Property.set=gtk_flipper_set_child)
   *
   * The child to display
   */
  properties[PROP_CHILD] = g_param_spec_object (
    "child", "Child", "the child to display", GTK_TYPE_WIDGET,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY
      | G_PARAM_STATIC_STRINGS);

  /**
   * GtkFlipper:flip-horizontal: (attributes org.gtk.Property.get=gtk_flipper_get_flip_horizontal org.gtk.Property.set=gtk_flipper_set_flip_horizontal)
   *
   * If the flipper should automatically begin playing.
   */
  properties[PROP_FLIP_HORIZONTAL] = g_param_spec_boolean (
    "flip-horizontal", "Flip horizontal",
    "Swap the left and right of the child", FALSE,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY
      | G_PARAM_STATIC_STRINGS);

  /**
   * GtkFlipper:flip-vertical: (attributes org.gtk.Property.get=gtk_flipper_get_flip_vertical org.gtk.Property.set=gtk_flipper_set_flip_vertical)
   *
   * If the flipper should automatically begin playing.
   */
  properties[PROP_FLIP_VERTICAL] = g_param_spec_boolean (
    "flip-vertical", "Flip vertical",
    "Swap the top and bottom of the child", FALSE,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY
      | G_PARAM_STATIC_STRINGS);

  /**
   * GtkFlipper:rotate: (attributes org.gtk.Property.get=gtk_flipper_get_rotate org.gtk.Property.set=gtk_flipper_set_rotate)
   *
   * Rotates the child by 90° if set. This is applied after any flipping.
   */
  properties[PROP_ROTATE] = g_param_spec_boolean (
    "rotate", "Rotate", "Rotate the child clockwise by 90°",
    FALSE,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY
      | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (
    gobject_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "flipper");
}

static void
gtk_flipper_init (GtkFlipper * self)
{
}

/**
 * gtk_flipper_new:
 * @child (nullable): The child widget to display
 *
 * Creates a new `GtkFlipper` containing @child.
 *
 * Returns: a new `GtkFlipper`
 */
GtkWidget *
gtk_flipper_new (GtkWidget * child)
{
  return g_object_new (GTK_TYPE_FLIPPER, "child", child, NULL);
}

/**
 * gtk_flipper_get_child: (attributes org.gtk.Method.get_property=child)
 * @self: a `GtkFlipper`
 *
 * Returns the child of the flipper if one is set.
 *
 * Returns: (transfer none) (nullable): The child
 */
GtkWidget *
gtk_flipper_get_child (GtkFlipper * self)
{
  g_return_val_if_fail (GTK_IS_FLIPPER (self), FALSE);

  return self->child;
}

/**
 * gtk_flipper_set_child: (attributes org.gtk.Method.set_property=child)
 * @self: a `GtkFlipper`
 * @child (nullable): the child to display
 *
 * Sets the child to display.
 */
void
gtk_flipper_set_child (GtkFlipper * self, GtkWidget * child)
{
  g_return_if_fail (GTK_IS_FLIPPER (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, gtk_widget_unparent);

  if (child)
    {
      self->child = child;
      gtk_widget_set_parent (self->child, GTK_WIDGET (self));
    }

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (
    G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * gtk_flipper_get_flip_horizontal: (attributes org.gtk.Method.get_property=flip-horizontal)
 * @self: a `GtkFlipper`
 *
 * Returns %TRUE if flipper should flip the child's left and right side.
 *
 * Returns: %TRUE if the child should flip horizontally
 */
gboolean
gtk_flipper_get_flip_horizontal (GtkFlipper * self)
{
  g_return_val_if_fail (GTK_IS_FLIPPER (self), FALSE);

  return self->flip_horizontal;
}

/**
 * gtk_flipper_set_flip_horizontal: (attributes org.gtk.Method.set_property=flip-horizontal)
 * @self: a `GtkFlipper`
 * @flip_horizontal: whether the child should be flipped horizontal
 *
 * Sets whether the child should swap its left and right sides.
 *
 * The flipping is applied before rotating.
 */
void
gtk_flipper_set_flip_horizontal (
  GtkFlipper * self,
  gboolean     flip_horizontal)
{
  g_return_if_fail (GTK_IS_FLIPPER (self));

  flip_horizontal = !!flip_horizontal;

  if (self->flip_horizontal == flip_horizontal)
    return;

  self->flip_horizontal = flip_horizontal ? 1 : 0;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (
    G_OBJECT (self), properties[PROP_FLIP_HORIZONTAL]);
}

/**
 * gtk_flipper_get_flip_vertical: (attributes org.gtk.Method.get_property=flip-vertical)
 * @self: a `GtkFlipper`
 *
 * Returns %TRUE if flipper should flip the child's top and bottom side.
 *
 * Returns: %TRUE if the child should flip vertically
 */
gboolean
gtk_flipper_get_flip_vertical (GtkFlipper * self)
{
  g_return_val_if_fail (GTK_IS_FLIPPER (self), FALSE);

  return self->flip_vertical;
}

/**
 * gtk_flipper_set_flip_vertical: (attributes org.gtk.Method.set_property=flip-vertical)
 * @self: a `GtkFlipper`
 * @flip_vertical: whether the child should be flipped vertical
 *
 * Sets whether the child should swap its top and bottom sides.
 *
 * The flipping is applied before rotating.
 */
void
gtk_flipper_set_flip_vertical (
  GtkFlipper * self,
  gboolean     flip_vertical)
{
  g_return_if_fail (GTK_IS_FLIPPER (self));

  flip_vertical = !!flip_vertical;

  if (self->flip_vertical == flip_vertical)
    return;

  self->flip_vertical = flip_vertical ? 1 : 0;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (
    G_OBJECT (self), properties[PROP_FLIP_VERTICAL]);
}

/**
 * gtk_flipper_get_rotate: (attributes org.gtk.Method.get_property=rotate)
 * @self: a `GtkFlipper`
 *
 * Returns %TRUE if flipper should rotate the child clockwise by 90°.
 *
 * Returns: %TRUE if the child should rotate
 */
gboolean
gtk_flipper_get_rotate (GtkFlipper * self)
{
  g_return_val_if_fail (GTK_IS_FLIPPER (self), FALSE);

  return self->rotate;
}

/**
 * gtk_flipper_set_rotate: (attributes org.gtk.Method.set_property=rotate)
 * @self: a `GtkFlipper`
 * @rotate: whether the child should be rotated
 *
 * Sets whether the child should rotate the child clockwise by 90°.
 *
 * The rotation is done after any eventual flipping.
 */
void
gtk_flipper_set_rotate (GtkFlipper * self, gboolean rotate)
{
  g_return_if_fail (GTK_IS_FLIPPER (self));

  rotate = !!rotate;

  if (self->rotate == rotate)
    return;

  self->rotate = rotate ? 1 : 0;

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (
    G_OBJECT (self), properties[PROP_ROTATE]);
}
