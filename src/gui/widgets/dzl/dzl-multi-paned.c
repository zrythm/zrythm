/* dzl-multi-paned.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "gui/widgets/dzl/dzl-multi-paned.h"
#include "gui/widgets/dzl/dzl-util-private.h"

#define HANDLE_WIDTH  10
#define HANDLE_HEIGHT 10

#define IS_HORIZONTAL(o) (o == GTK_ORIENTATION_HORIZONTAL)

/**
 * SECTION:dzlmultipaned
 * @title: DzlMultiPaned
 * @short_description: A widget with multiple adjustable panes
 *
 * This widget is similar to #GtkPaned except that it allows adding more than
 * two children to the widget. For each additional child added to the
 * #DzlMultiPaned, an additional resize grip is added.
 */

typedef struct
{
  /*
   * The child widget in question.
   */
  GtkWidget *widget;

  /*
   * The input only window for resize grip.
   * Has a cursor associated with it.
   */
  GdkWindow *handle;

  /*
   * The position the handle has been dragged to.
   * This is used to adjust size requests.
   */
  gint position;

  /*
   * Cached size requests to avoid extra sizing calls during
   * the layout procedure.
   */
  GtkRequisition min_req;
  GtkRequisition nat_req;

  /*
   * A cached size allocation used during the size_allocate()
   * cycle. This allows us to do a first pass to allocate
   * natural sizes, and then followup when dealing with
   * expanding children.
   */
  GtkAllocation alloc;

  /*
   * If the position field has been set.
   */
  guint position_set : 1;
} DzlMultiPanedChild;

typedef struct
{
  /*
   * A GArray of DzlMultiPanedChild containing everything we need to
   * do size requests, drag operations, resize handles, and temporary
   * space needed in such operations.
   */
  GArray *children;

  /*
   * The gesture used for dragging resize handles.
   *
   * TODO: GtkPaned now uses two gestures, one for mouse and one for touch.
   *       We should do the same as it improved things quite a bit.
   */
  GtkGesturePan *gesture;

  /*
   * For GtkOrientable:orientation.
   */
  GtkOrientation orientation;

  /*
   * This is the child that is currently being dragged. Keep in mind that
   * the drag handle is immediately after the child. So the final visible
   * child has the handle input-only window hidden.
   */
  DzlMultiPanedChild *drag_begin;

  /*
   * The position (width or height) of the child when the drag began.
   * We use the pan delta offset to determine what the size should be
   * by adding (or subtracting) to this value.
   */
  gint drag_begin_position;

  /*
   * If we are dragging a handle in a fashion that would shrink the
   * previous widgets, we need to track how much to subtract from their
   * target allocations. This is set during the drag operation and used
   * in allocation_stage_drag_overflow() to adjust the neighbors.
   */
  gint drag_extra_offset;
} DzlMultiPanedPrivate;

typedef struct
{
  DzlMultiPanedChild **children;
  guint                n_children;
  GtkOrientation       orientation;
  GtkAllocation        top_alloc;
  gint                 avail_width;
  gint                 avail_height;
  gint                 handle_size;
} AllocationState;

typedef void (*AllocationStage) (DzlMultiPaned   *self,
                                 AllocationState *state);

static void allocation_stage_allocate      (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_borders       (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_cache_request (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_drag_overflow (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_expand        (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_handles       (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_minimums      (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_naturals      (DzlMultiPaned   *self,
                                            AllocationState *state);
static void allocation_stage_positions     (DzlMultiPaned   *self,
                                            AllocationState *state);

G_DEFINE_TYPE_EXTENDED (DzlMultiPaned, dzl_multi_paned, GTK_TYPE_CONTAINER, 0,
                        G_ADD_PRIVATE (DzlMultiPaned)
                        G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

enum {
  PROP_0,
  PROP_ORIENTATION,
  N_PROPS
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_INDEX,
  CHILD_PROP_POSITION,
  N_CHILD_PROPS
};

enum {
  STYLE_PROP_0,
  STYLE_PROP_HANDLE_SIZE,
  N_STYLE_PROPS
};

enum {
  RESIZE_DRAG_BEGIN,
  RESIZE_DRAG_END,
  N_SIGNALS
};

/*
 * TODO: An obvious optimization here would be to move the constant
 *       branches outside the loops.
 */

static GParamSpec *properties [N_PROPS];
static GParamSpec *child_properties [N_CHILD_PROPS];
static GParamSpec *style_properties [N_STYLE_PROPS];
static guint signals [N_SIGNALS];
static AllocationStage allocation_stages[] = {
  allocation_stage_borders,
  allocation_stage_cache_request,
  allocation_stage_minimums,
  allocation_stage_handles,
  allocation_stage_positions,
  allocation_stage_drag_overflow,
  allocation_stage_naturals,
  allocation_stage_expand,
  allocation_stage_allocate,
};

static void
dzl_multi_paned_reset_positions (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      child->position = -1;
      child->position_set = FALSE;

      gtk_container_child_notify_by_pspec (GTK_CONTAINER (self),
                                           child->widget,
                                           child_properties [CHILD_PROP_POSITION]);
    }

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static DzlMultiPanedChild *
dzl_multi_paned_get_next_visible_child (DzlMultiPaned      *self,
                                        DzlMultiPanedChild *child)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (child != NULL);
  g_assert (priv->children != NULL);
  g_assert (priv->children->len > 0);

  i = child - ((DzlMultiPanedChild *)(gpointer)priv->children->data);

  for (++i; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *next = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (gtk_widget_get_visible (next->widget))
        return next;
    }

  return NULL;
}

static gboolean
dzl_multi_paned_is_last_visible_child (DzlMultiPaned      *self,
                                       DzlMultiPanedChild *child)
{
  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (child != NULL);

  return !dzl_multi_paned_get_next_visible_child (self, child);
}

static void
dzl_multi_paned_get_handle_rect (DzlMultiPaned      *self,
                                 DzlMultiPanedChild *child,
                                 GdkRectangle       *handle_rect)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkAllocation alloc;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (child != NULL);
  g_assert (handle_rect != NULL);

  handle_rect->x = -1;
  handle_rect->y = -1;
  handle_rect->width = 0;
  handle_rect->height = 0;

  if (!gtk_widget_get_visible (child->widget) ||
      !gtk_widget_get_realized (child->widget))
    return;

  if (dzl_multi_paned_is_last_visible_child (self, child))
    return;

  gtk_widget_get_allocation (child->widget, &alloc);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      handle_rect->x = alloc.x + alloc.width - (HANDLE_WIDTH / 2);
      handle_rect->width = HANDLE_WIDTH;
      handle_rect->y = alloc.y;
      handle_rect->height = alloc.height;
    }
  else
    {
      handle_rect->x = alloc.x;
      handle_rect->width = alloc.width;
      handle_rect->y = alloc.y + alloc.height - (HANDLE_HEIGHT / 2);
      handle_rect->height = HANDLE_HEIGHT;
    }
}

static void
dzl_multi_paned_create_child_handle (DzlMultiPaned      *self,
                                     DzlMultiPanedChild *child)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GdkWindowAttr attributes = { 0 };
  GdkDisplay *display;
  GdkWindow *parent;
  const char *cursor_name;
  GdkRectangle handle_rect;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (child != NULL);
  g_assert (child->handle == NULL);

  display = gtk_widget_get_display (GTK_WIDGET (self));
  parent = gtk_widget_get_window (GTK_WIDGET (self));

  cursor_name = (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                ? "col-resize"
                : "row-resize";

  dzl_multi_paned_get_handle_rect (self, child, &handle_rect);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = handle_rect.x;
  attributes.y = -handle_rect.y;
  attributes.width = handle_rect.width;
  attributes.height = handle_rect.height;
  attributes.visual = gtk_widget_get_visual (GTK_WIDGET (self));
  attributes.event_mask = (GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_ENTER_NOTIFY_MASK |
                           GDK_LEAVE_NOTIFY_MASK |
                           GDK_POINTER_MOTION_MASK);
  attributes.cursor = gdk_cursor_new_from_name (display, cursor_name);

  child->handle = gdk_window_new (parent, &attributes, GDK_WA_CURSOR);
  gtk_widget_register_window (GTK_WIDGET (self), child->handle);

  g_clear_object (&attributes.cursor);
}

static gint
dzl_multi_paned_calc_handle_size (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  gint visible_children = 0;
  gint handle_size = 1;
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  gtk_widget_style_get (GTK_WIDGET (self), "handle-size", &handle_size, NULL);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (gtk_widget_get_visible (child->widget))
        visible_children++;
    }

  return MAX (0, (visible_children - 1) * handle_size);
}

static void
dzl_multi_paned_destroy_child_handle (DzlMultiPaned      *self,
                                      DzlMultiPanedChild *child)
{
  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (child != NULL);

  if (child->handle != NULL)
    {
      gtk_widget_unregister_window (GTK_WIDGET (self), child->handle);
      gdk_window_destroy (child->handle);
      child->handle = NULL;
    }
}

static void
dzl_multi_paned_update_child_handles (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkWidget *widget = GTK_WIDGET (self);

  if (gtk_widget_get_realized (widget))
    {
      GdkCursor *cursor;
      guint i;

      if (gtk_widget_is_sensitive (widget))
        cursor = gdk_cursor_new_from_name (gtk_widget_get_display (widget),
                                           priv->orientation == GTK_ORIENTATION_HORIZONTAL
                                           ? "col-resize"
                                           : "row-resize");
      else
        cursor = NULL;

      for (i = 0; i < priv->children->len; i++)
        {
          DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

          gdk_window_set_cursor (child->handle, cursor);
        }

      if (cursor)
        g_object_unref (cursor);
    }
}

static DzlMultiPanedChild *
dzl_multi_paned_get_child (DzlMultiPaned *self,
                           GtkWidget     *widget)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));

  for (guint i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (child->widget == widget)
        return child;
    }

  g_assert_not_reached ();

  return NULL;
}

static gint
dzl_multi_paned_get_child_index (DzlMultiPaned *self,
                                 GtkWidget     *widget)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));

  for (guint i = 0; i < priv->children->len; i++)
    {
      const DzlMultiPanedChild *ele = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (ele->widget == widget)
        return i;
    }

  return -1;
}

static void
dzl_multi_paned_set_child_index (DzlMultiPaned *self,
                                 GtkWidget     *widget,
                                 gint           index)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));
  g_assert (index >= -1);
  g_assert (priv->children->len > 0);

  if (index < 0)
    index = priv->children->len - 1;

  index = CLAMP (index, 0, priv->children->len - 1);

  for (guint i = 0; i < priv->children->len; i++)
    {
      const DzlMultiPanedChild *ele = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (ele->widget == widget)
        {
          DzlMultiPanedChild copy = { 0 };

          copy.widget = ele->widget;
          copy.handle = ele->handle;
          copy.position = -1;
          copy.position_set = FALSE;

          g_array_remove_index (priv->children, i);
          g_array_insert_val (priv->children, index, copy);
          gtk_container_child_notify_by_pspec (GTK_CONTAINER (self), widget,
                                               child_properties [CHILD_PROP_INDEX]);
          gtk_widget_queue_resize (GTK_WIDGET (self));

          break;
        }
    }
}

static gint
dzl_multi_paned_get_child_position (DzlMultiPaned *self,
                                    GtkWidget     *widget)
{
  DzlMultiPanedChild *child;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));

  child = dzl_multi_paned_get_child (self, widget);

  return child->position;
}

static void
dzl_multi_paned_set_child_position (DzlMultiPaned *self,
                                    GtkWidget     *widget,
                                    gint           position)
{
  DzlMultiPanedChild *child;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));
  g_assert (position >= -1);

  child = dzl_multi_paned_get_child (self, widget);

  if (child->position != position)
    {
      child->position = position;
      child->position_set = (position != -1);
      gtk_container_child_notify_by_pspec (GTK_CONTAINER (self), widget,
                                           child_properties [CHILD_PROP_POSITION]);
      gtk_widget_queue_resize (GTK_WIDGET (self));
    }
}

static void
dzl_multi_paned_add (GtkContainer *container,
                     GtkWidget    *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)container;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  DzlMultiPanedChild child = { 0 };

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));

  child.widget = g_object_ref_sink (widget);
  child.position = -1;

  if (gtk_widget_get_realized (GTK_WIDGET (self)))
    dzl_multi_paned_create_child_handle (self, &child);

  gtk_widget_set_parent (widget, GTK_WIDGET (self));

  g_array_append_val (priv->children, child);

  dzl_multi_paned_reset_positions (self);

  gtk_gesture_set_state (GTK_GESTURE (priv->gesture), GTK_EVENT_SEQUENCE_DENIED);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
dzl_multi_paned_remove (GtkContainer *container,
                        GtkWidget    *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)container;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (widget));

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (child->widget == widget)
        {
          dzl_multi_paned_destroy_child_handle (self, child);

          g_array_remove_index (priv->children, i);
          child = NULL;

          gtk_widget_unparent (widget);
          g_object_unref (widget);

          break;
        }
    }

  dzl_multi_paned_reset_positions (self);

  gtk_gesture_set_state (GTK_GESTURE (priv->gesture), GTK_EVENT_SEQUENCE_DENIED);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
dzl_multi_paned_forall (GtkContainer *container,
                        gboolean      include_internals,
                        GtkCallback   callback,
                        gpointer      user_data)
{
  DzlMultiPaned *self = (DzlMultiPaned *)container;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  gint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (callback != NULL);

  for (i = priv->children->len; i > 0; i--)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i - 1);

      callback (child->widget, user_data);
    }
}

static GtkSizeRequestMode
dzl_multi_paned_get_request_mode (GtkWidget *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_assert (DZL_IS_MULTI_PANED (self));

  return (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ? GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT
                                                           : GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
dzl_multi_paned_get_preferred_height (GtkWidget *widget,
                                      gint      *min_height,
                                      gint      *nat_height)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkStyleContext *style_context;
  GtkBorder borders;
  guint i;
  gint real_min_height = 0;
  gint real_nat_height = 0;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (min_height != NULL);
  g_assert (nat_height != NULL);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);
      gint child_min_height = 0;
      gint child_nat_height = 0;

      if (gtk_widget_get_visible (child->widget))
        {
          gtk_widget_get_preferred_height (child->widget, &child_min_height, &child_nat_height);

          if (priv->orientation == GTK_ORIENTATION_VERTICAL)
            {
              real_min_height += child_min_height;
              real_nat_height += child_nat_height;
            }
          else
            {
              real_min_height = MAX (real_min_height, child_min_height);
              real_nat_height = MAX (real_nat_height, child_nat_height);
            }
        }
    }

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      gint handle_size = dzl_multi_paned_calc_handle_size (self);

      real_min_height += handle_size;
      real_nat_height += handle_size;
    }

  *min_height = real_min_height;
  *nat_height = real_nat_height;

  style_context = gtk_widget_get_style_context (widget);
  dzl_gtk_style_context_get_borders (style_context, &borders);

  *min_height += borders.top + borders.bottom;
  *nat_height += borders.top + borders.bottom;
}

static void
dzl_multi_paned_get_child_preferred_height_for_width (DzlMultiPaned      *self,
                                                      DzlMultiPanedChild *children,
                                                      gint                n_children,
                                                      gint                width,
                                                      gint               *min_height,
                                                      gint               *nat_height)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  DzlMultiPanedChild *child = children;
  gint child_min_height = 0;
  gint child_nat_height = 0;
  gint neighbor_min_height = 0;
  gint neighbor_nat_height = 0;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (n_children == 0 || children != NULL);
  g_assert (min_height != NULL);
  g_assert (nat_height != NULL);

  *min_height = 0;
  *nat_height = 0;

  if (n_children == 0)
    return;

  if (gtk_widget_get_visible (child->widget))
    gtk_widget_get_preferred_height_for_width (child->widget,
                                               width,
                                               &child_min_height,
                                               &child_nat_height);

  dzl_multi_paned_get_child_preferred_height_for_width (self,
                                                        children + 1,
                                                        n_children - 1,
                                                        width,
                                                        &neighbor_min_height,
                                                        &neighbor_nat_height);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      *min_height = child_min_height + neighbor_min_height;
      *nat_height = child_nat_height + neighbor_nat_height;
    }
  else
    {
      *min_height = MAX (child_min_height, neighbor_min_height);
      *nat_height = MAX (child_nat_height, neighbor_nat_height);
    }
}

static void
dzl_multi_paned_get_preferred_height_for_width (GtkWidget *widget,
                                                gint       width,
                                                gint      *min_height,
                                                gint      *nat_height)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkStyleContext *style_context;
  GtkBorder borders;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (min_height != NULL);
  g_assert (nat_height != NULL);

  *min_height = 0;
  *nat_height = 0;

  dzl_multi_paned_get_child_preferred_height_for_width (self,
                                                        (DzlMultiPanedChild *)(gpointer)priv->children->data,
                                                        priv->children->len,
                                                        width,
                                                        min_height,
                                                        nat_height);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      gint handle_size = dzl_multi_paned_calc_handle_size (self);

      *min_height += handle_size;
      *nat_height += handle_size;
    }

  style_context = gtk_widget_get_style_context (widget);
  dzl_gtk_style_context_get_borders (style_context, &borders);

  *min_height += borders.top + borders.bottom;
  *nat_height += borders.top + borders.bottom;
}

static void
dzl_multi_paned_get_preferred_width (GtkWidget *widget,
                                     gint      *min_width,
                                     gint      *nat_width)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkStyleContext *style_context;
  GtkBorder borders;
  guint i;
  gint real_min_width = 0;
  gint real_nat_width = 0;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (min_width != NULL);
  g_assert (nat_width != NULL);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);
      gint child_min_width = 0;
      gint child_nat_width = 0;

      if (gtk_widget_get_visible (child->widget))
        {
          gtk_widget_get_preferred_width (child->widget, &child_min_width, &child_nat_width);

          if (priv->orientation == GTK_ORIENTATION_VERTICAL)
            {
              real_min_width = MAX (real_min_width, child_min_width);
              real_nat_width = MAX (real_nat_width, child_nat_width);
            }
          else
            {
              real_min_width += child_min_width;
              real_nat_width += child_nat_width;
            }
        }
    }

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gint handle_size = dzl_multi_paned_calc_handle_size (self);

      real_min_width += handle_size;
      real_nat_width += handle_size;
    }

  *min_width = real_min_width;
  *nat_width = real_nat_width;

  style_context = gtk_widget_get_style_context (widget);
  dzl_gtk_style_context_get_borders (style_context, &borders);

  *min_width += borders.left + borders.right;
  *nat_width += borders.left + borders.right;
}

static void
dzl_multi_paned_get_child_preferred_width_for_height (DzlMultiPaned      *self,
                                                      DzlMultiPanedChild *children,
                                                      gint                n_children,
                                                      gint                height,
                                                      gint               *min_width,
                                                      gint               *nat_width)
{
  DzlMultiPanedChild *child = children;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  gint child_min_width = 0;
  gint child_nat_width = 0;
  gint neighbor_min_width = 0;
  gint neighbor_nat_width = 0;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (n_children == 0 || children != NULL);
  g_assert (min_width != NULL);
  g_assert (nat_width != NULL);

  *min_width = 0;
  *nat_width = 0;

  if (n_children == 0)
    return;

  if (gtk_widget_get_visible (child->widget))
    gtk_widget_get_preferred_width_for_height (child->widget,
                                               height,
                                               &child_min_width,
                                               &child_nat_width);

  dzl_multi_paned_get_child_preferred_width_for_height (self,
                                                        children + 1,
                                                        n_children - 1,
                                                        height,
                                                        &neighbor_min_width,
                                                        &neighbor_nat_width);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      *min_width = child_min_width + neighbor_min_width;
      *nat_width = child_nat_width + neighbor_nat_width;
    }
  else
    {
      *min_width = MAX (child_min_width, neighbor_min_width);
      *nat_width = MAX (child_nat_width, neighbor_nat_width);
    }
}

static void
dzl_multi_paned_get_preferred_width_for_height (GtkWidget *widget,
                                                gint       height,
                                                gint      *min_width,
                                                gint      *nat_width)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkStyleContext *style_context;
  GtkBorder borders;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (min_width != NULL);
  g_assert (nat_width != NULL);

  dzl_multi_paned_get_child_preferred_width_for_height (self,
                                                        (DzlMultiPanedChild *)(gpointer)priv->children->data,
                                                        priv->children->len,
                                                        height,
                                                        min_width,
                                                        nat_width);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gint handle_size = dzl_multi_paned_calc_handle_size (self);

      *min_width += handle_size;
      *nat_width += handle_size;
    }

  style_context = gtk_widget_get_style_context (widget);
  dzl_gtk_style_context_get_borders (style_context, &borders);

  *min_width += borders.left + borders.right;
  *nat_width += borders.left + borders.right;
}

static void
allocation_stage_handles (DzlMultiPaned   *self,
                          AllocationState *state)
{
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  /*
   * Push each child allocation forward by the sum handle widths up to
   * their position in the paned.
   */

  for (i = 1; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      if (IS_HORIZONTAL (state->orientation))
        child->alloc.x += (i * state->handle_size);
      else
        child->alloc.y += (i * state->handle_size);
    }

  if (IS_HORIZONTAL (state->orientation))
    state->avail_width -= (state->n_children - 1) * state->handle_size;
  else
    state->avail_height -= (state->n_children - 1) * state->handle_size;
}

static void
allocation_stage_minimums (DzlMultiPaned   *self,
                           AllocationState *state)
{
  gint next_x;
  gint next_y;
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  next_x = state->top_alloc.x;
  next_y = state->top_alloc.y;

  for (i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      if (IS_HORIZONTAL (state->orientation))
        {
          child->alloc.x = next_x;
          child->alloc.y = state->top_alloc.y;
          child->alloc.width = child->min_req.width;
          child->alloc.height = state->top_alloc.height;

          next_x = child->alloc.x + child->alloc.width;

          state->avail_width -= child->alloc.width;
        }
      else
        {
          child->alloc.x = state->top_alloc.x;
          child->alloc.y = next_y;
          child->alloc.width = state->top_alloc.width;
          child->alloc.height = child->min_req.height;

          next_y = child->alloc.y + child->alloc.height;

          state->avail_height -= child->alloc.height;
        }
    }
}

static void
allocation_stage_naturals (DzlMultiPaned   *self,
                           AllocationState *state)
{
  gint x_adjust = 0;
  gint y_adjust = 0;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  for (guint i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      child->alloc.x += x_adjust;
      child->alloc.y += y_adjust;

      if (!child->position_set)
        {
          if (IS_HORIZONTAL (state->orientation))
            {
              if (child->nat_req.width > child->alloc.width)
                {
                  gint adjust = MIN (state->avail_width, child->nat_req.width - child->alloc.width);

                  child->alloc.width += adjust;
                  state->avail_width -= adjust;
                  x_adjust += adjust;
                }
            }
          else
            {
              if (child->nat_req.height > child->alloc.height)
                {
                  gint adjust = MIN (state->avail_height, child->nat_req.height - child->alloc.height);

                  child->alloc.height += adjust;
                  state->avail_height -= adjust;
                  y_adjust += adjust;
                }
            }
        }
    }
}

static void
allocation_stage_borders (DzlMultiPaned   *self,
                          AllocationState *state)
{
  GtkStyleContext *style_context;
  GtkBorder borders;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  /*
   * This subtracts the border+padding from the allocation area so the
   * children are guaranteed to fall within that area.
   */

  style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
  dzl_gtk_style_context_get_borders (style_context, &borders);

  state->top_alloc.x += borders.left;
  state->top_alloc.y += borders.right;
  state->top_alloc.width -= (borders.left + borders.right);
  state->top_alloc.height -= (borders.top + borders.bottom);

  if (state->top_alloc.width < 0)
    state->top_alloc.width = 0;

  if (state->top_alloc.height < 0)
    state->top_alloc.height = 0;

  state->avail_width = state->top_alloc.width;
  state->avail_height = state->top_alloc.height;
}

static void
allocation_stage_cache_request (DzlMultiPaned   *self,
                                AllocationState *state)
{
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  for (i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      if (IS_HORIZONTAL (state->orientation))
        gtk_widget_get_preferred_width_for_height (child->widget,
                                                   state->avail_height,
                                                   &child->min_req.width,
                                                   &child->nat_req.width);
      else
        gtk_widget_get_preferred_height_for_width (child->widget,
                                                   state->avail_width,
                                                   &child->min_req.height,
                                                   &child->nat_req.height);
    }
}

static void
allocation_stage_positions (DzlMultiPaned   *self,
                            AllocationState *state)
{
  gint x_adjust = 0;
  gint y_adjust = 0;
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  /*
   * Child may have a position set, which happens when dragging the input
   * window (handle) to resize the child. If so, we want to try to allocate
   * extra space above the minimum size.
   */

  for (i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      child->alloc.x += x_adjust;
      child->alloc.y += y_adjust;

      if (child->position_set)
        {
          if (IS_HORIZONTAL (state->orientation))
            {
              if (child->position > child->alloc.width)
                {
                  gint adjust = MIN (state->avail_width, child->position - child->alloc.width);

                  child->alloc.width += adjust;
                  state->avail_width -= adjust;
                  x_adjust += adjust;
                }
            }
          else
            {
              if (child->position > child->alloc.height)
                {
                  gint adjust = MIN (state->avail_height, child->position - child->alloc.height);

                  child->alloc.height += adjust;
                  state->avail_height -= adjust;
                  y_adjust += adjust;
                }
            }
        }
    }
}

static void
allocation_stage_drag_overflow (DzlMultiPaned   *self,
                                AllocationState *state)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint drag_index;
  gint j;
  gint drag_overflow;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  if (priv->drag_begin == NULL)
    return;

  drag_overflow = ABS (priv->drag_extra_offset);

  for (drag_index = 0; drag_index < state->n_children; drag_index++)
    if (state->children [drag_index] == priv->drag_begin)
      break;

  if (drag_index == 0 ||
      drag_index >= state->n_children ||
      state->children [drag_index] != priv->drag_begin)
    return;

  /*
   * If the user is dragging and we have run out of room in the drag
   * child, then we need to start stealing space from the previous
   * items.
   *
   * This works our way back to the beginning from the drag child
   * stealing available space and giving it to the child *AFTER* the
   * drag item. This is because the drag handle is after the drag
   * child, so logically to the user, its drag_index+1.
   */

  for (j = (int)drag_index; j >= 0 && drag_overflow > 0; j--)
    {
      DzlMultiPanedChild *child = state->children [j];
      guint k;
      gint adjust = 0;

      if (IS_HORIZONTAL (state->orientation))
        {
          if (child->alloc.width > child->min_req.width)
            {
              if (drag_overflow > (child->alloc.width - child->min_req.width))
                adjust = child->alloc.width - child->min_req.width;
              else
                adjust = drag_overflow;
              drag_overflow -= adjust;
              child->alloc.width -= adjust;
              state->children [drag_index + 1]->alloc.width += adjust;
            }
        }
      else
        {
          if (child->alloc.height > child->min_req.height)
            {
              if (drag_overflow > (child->alloc.height - child->min_req.height))
                adjust = child->alloc.height - child->min_req.height;
              else
                adjust = drag_overflow;
              drag_overflow -= adjust;
              child->alloc.height -= adjust;
              state->children [drag_index + 1]->alloc.height += adjust;
            }
        }

      /*
       * Now walk back forward and adjust x/y offsets for all of the
       * children that will have just shifted.
       */

      for (k = j + 1; k <= drag_index + 1; k++)
        {
          DzlMultiPanedChild *neighbor = state->children [k];

          if (IS_HORIZONTAL (state->orientation))
            neighbor->alloc.x -= adjust;
          else
            neighbor->alloc.y -= adjust;
        }
    }
}

static gint
sort_children_horizontal (const DzlMultiPanedChild * const *first,
                          const DzlMultiPanedChild * const *second)
{
  return (*first)->alloc.width - (*second)->alloc.width;
}

static gint
sort_children_vertical (const DzlMultiPanedChild * const *first,
                        const DzlMultiPanedChild * const *second)
{
  return (*first)->alloc.height - (*second)->alloc.height;
}

static void
allocation_stage_expand (DzlMultiPaned   *self,
                         AllocationState *state)
{
  g_autoptr(GPtrArray) expanding = NULL;
  gint x_adjust = 0;
  gint y_adjust = 0;
  gint adjust;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  if (state->n_children == 1)
    {
      DzlMultiPanedChild *child = state->children [0];

      /*
       * Special case for single child, just expand to the
       * available space. Ideally we would have much shorter
       * allocation stages in this case.
       */

      if (gtk_widget_compute_expand (child->widget, state->orientation))
        {
          if (IS_HORIZONTAL (state->orientation))
            child->alloc.width = state->top_alloc.width;
          else
            child->alloc.height = state->top_alloc.height;
        }

      return;
    }

  /*
   * First, we need to collect all of the children who are expanding
   * in the direction matching our orientation. After that, we will
   * sort them by their allocation size so that we can divy out extra
   * allocation starting from the smallest. This will give us an effect
   * of homogeneous size when all children are set to expand and enough
   * space is available.
   */

  expanding = g_ptr_array_new ();

  for (guint i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      if (!child->position_set)
        {
          if (gtk_widget_compute_expand (child->widget, state->orientation))
            g_ptr_array_add (expanding, child);
        }
    }

  if (expanding->len == 0)
    goto fill_last;

  /* Now sort, smallest first, based on size in given orientation. */
  if (IS_HORIZONTAL (state->orientation))
    g_ptr_array_sort (expanding, (GCompareFunc) sort_children_horizontal);
  else
    g_ptr_array_sort (expanding, (GCompareFunc) sort_children_vertical);

  /*
   * While we have additional space available to hand out, allocate
   * as much space as it takes to get to the next item in the array.
   * If we run out of additional space, that is okay (as long as we
   * dont hand out more than we have).
   */

  g_assert (expanding->len > 0);

  for (guint i = 0; i < expanding->len - 1; i++)
    {
      DzlMultiPanedChild *child = g_ptr_array_index (expanding, i);
      DzlMultiPanedChild *next = g_ptr_array_index (expanding, i + 1);

      if (IS_HORIZONTAL (state->orientation))
        {
          guint j;

          g_assert (next->alloc.width >= child->alloc.width);

          adjust = next->alloc.width - child->alloc.width;
          if (adjust > state->avail_width)
            adjust = state->avail_width;

          child->alloc.width += adjust;
          state->avail_width -= adjust;

          /* Adjust X of children following */
          for (j = 0; j < state->n_children; j++)
            if (state->children[j] == child)
              break;
          for (++j; j < state->n_children; j++)
            state->children[j]->alloc.x += adjust;

          g_assert (state->avail_width >= 0);

          if (state->avail_width == 0)
            break;
        }
      else
        {
          guint j;

          g_assert (next->alloc.height >= child->alloc.height);

          adjust = next->alloc.height - child->alloc.height;
          if (adjust > state->avail_height)
            adjust = state->avail_height;

          child->alloc.height += adjust;
          state->avail_height -= adjust;

          /* Adjust Y of children following */
          for (j = 0; j < state->n_children; j++)
            if (state->children[j] == child)
              break;
          for (++j; j < state->n_children; j++)
            state->children[j]->alloc.y += adjust;

          g_assert (state->avail_height >= 0);

          if (state->avail_height == 0)
            break;
        }
    }

  /*
   * If we still have more space we can divy out, then do the normal
   * expansion case now that we are starting with evenly sized
   * children.
   */

  if (IS_HORIZONTAL (state->orientation))
    adjust = state->avail_width / expanding->len;
  else
    adjust = state->avail_height / expanding->len;

  for (guint i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      child->alloc.x += x_adjust;
      child->alloc.y += y_adjust;

      if (!child->position_set)
        {
          if (gtk_widget_compute_expand (child->widget, state->orientation))
            {
              if (IS_HORIZONTAL (state->orientation))
                {
                  child->alloc.width += adjust;
                  state->avail_width -= adjust;
                  x_adjust += adjust;
                }
              else
                {
                  child->alloc.height += adjust;
                  state->avail_height -= adjust;
                  y_adjust += adjust;
                }
            }
        }
    }

fill_last:

  if (IS_HORIZONTAL (state->orientation))
    {
      if (state->avail_width > 0)
        {
          state->children [state->n_children - 1]->alloc.width += state->avail_width;
          state->avail_width = 0;
        }
    }
  else
    {
      if (state->avail_height > 0)
        {
          state->children [state->n_children - 1]->alloc.height += state->avail_height;
          state->avail_height = 0;
        }
    }
}

static void
allocation_stage_allocate (DzlMultiPaned   *self,
                           AllocationState *state)
{
  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (state != NULL);
  g_assert (state->children != NULL);
  g_assert (state->n_children > 0);

  for (guint i = 0; i < state->n_children; i++)
    {
      DzlMultiPanedChild *child = state->children [i];

      gtk_widget_size_allocate (child->widget, &child->alloc);

      if (child->handle != NULL)
        {
          if (state->n_children != (i + 1))
            {
              if (IS_HORIZONTAL (state->orientation))
                {
                  gdk_window_move_resize (child->handle,
                                          child->alloc.x + child->alloc.width - (HANDLE_WIDTH / 2),
                                          child->alloc.y,
                                          HANDLE_WIDTH,
                                          child->alloc.height);
                }
              else
                {
                  gdk_window_move_resize (child->handle,
                                          child->alloc.x,
                                          child->alloc.y + child->alloc.height - (HANDLE_HEIGHT / 2),
                                          child->alloc.width,
                                          HANDLE_HEIGHT);
                }

              gdk_window_show (child->handle);
            }
          else
            {
              gdk_window_hide (child->handle);
            }
        }
    }
}

static void
dzl_multi_paned_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  AllocationState state = { 0 };
  g_autoptr(GPtrArray) children = NULL;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (allocation != NULL);

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->size_allocate (widget, allocation);

  if (priv->children->len == 0)
    return;

  children = g_ptr_array_new ();

  for (guint i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      child->alloc.x = 0;
      child->alloc.y = 0;
      child->alloc.width = 0;
      child->alloc.height = 0;

      if (child->widget != NULL &&
          gtk_widget_get_child_visible (child->widget) &&
          gtk_widget_get_visible (child->widget))
        g_ptr_array_add (children, child);
      else if (child->handle)
        gdk_window_hide (child->handle);
    }

  state.children = (DzlMultiPanedChild **)children->pdata;
  state.n_children = children->len;

  if (state.n_children == 0)
    return;

  gtk_widget_style_get (GTK_WIDGET (self),
                        "handle-size", &state.handle_size,
                        NULL);

  state.orientation = priv->orientation;
  state.top_alloc = *allocation;
  state.avail_width = allocation->width;
  state.avail_height = allocation->height;

  for (guint i = 0; i < G_N_ELEMENTS (allocation_stages); i++)
    allocation_stages [i] (self, &state);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
dzl_multi_paned_realize (GtkWidget *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->realize (widget);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      dzl_multi_paned_create_child_handle (self, child);
    }
}

static void
dzl_multi_paned_unrealize (GtkWidget *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      dzl_multi_paned_destroy_child_handle (self, child);
    }

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->unrealize (widget);
}

static void
dzl_multi_paned_map (GtkWidget *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->map (widget);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      gdk_window_show (child->handle);
    }
}

static void
dzl_multi_paned_unmap (GtkWidget *widget)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      gdk_window_hide (child->handle);
    }

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->unmap (widget);
}

static gboolean
dzl_multi_paned_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  DzlMultiPaned *self = (DzlMultiPaned *)widget;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkStyleContext *style_context;
  GtkAllocation alloc;
  GtkBorder margin;
  GtkBorder borders;
  GtkStateFlags state;
  gint handle_size = 1;
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (cr != NULL);

  gtk_widget_get_allocation (widget, &alloc);

  alloc.x = 0;
  alloc.y = 0;

  style_context = gtk_widget_get_style_context (widget);
  state = gtk_style_context_get_state (style_context);

  dzl_gtk_style_context_get_borders (style_context, &borders);

  gtk_style_context_get_margin (style_context, state, &margin);
  dzl_gtk_allocation_subtract_border (&alloc, &margin);

  gtk_render_background (style_context, cr, alloc.x, alloc.y, alloc.width, alloc.height);

  gtk_widget_style_get (widget, "handle-size", &handle_size, NULL);

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (!gtk_widget_get_realized (child->widget) ||
          !gtk_widget_get_visible (child->widget))
        continue;

      gtk_container_propagate_draw (GTK_CONTAINER (self), child->widget, cr);
    }

  if (priv->children->len > 0)
    {
      gtk_style_context_save (style_context);
      gtk_style_context_add_class (style_context, "handle");

      for (i = 0; i < priv->children->len; i++)
        {
          DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);
          GtkAllocation child_alloc;

          if (!gtk_widget_get_visible (child->widget) ||
              !gtk_widget_get_child_visible (child->widget))
            continue;

          if (dzl_multi_paned_is_last_visible_child (self, child))
            break;

          gtk_widget_get_allocation (child->widget, &child_alloc);
          gtk_widget_translate_coordinates (child->widget, widget, 0, 0, &child_alloc.x, &child_alloc.y);

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            gtk_render_handle (style_context,
                               cr,
                               child_alloc.x + child_alloc.width,
                               borders.top,
                               handle_size,
                               child_alloc.height);
          else
            gtk_render_handle (style_context,
                               cr,
                               borders.left,
                               child_alloc.y + child_alloc.height,
                               child_alloc.width,
                               handle_size);

        }

      gtk_style_context_restore (style_context);
    }

  gtk_render_frame (style_context, cr, alloc.x, alloc.y, alloc.width, alloc.height);

  return FALSE;
}

static void
dzl_multi_paned_pan_gesture_drag_begin (DzlMultiPaned *self,
                                        gdouble        x,
                                        gdouble        y,
                                        GtkGesturePan *gesture)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GdkEventSequence *sequence;
  const GdkEvent *event;
  guint i;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_GESTURE_PAN (gesture));
  g_assert (gesture == priv->gesture);

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  event = gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);

  priv->drag_begin = NULL;
  priv->drag_begin_position = 0;
  priv->drag_extra_offset = 0;

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (child->handle == event->any.window)
        {
          priv->drag_begin = child;
          break;
        }
    }

  if (priv->drag_begin == NULL)
    {
      gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  for (i = 0; i < priv->children->len; i++)
    {
      DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

      if (child->handle == event->any.window)
        break;

      /*
       * We want to make any child before the drag child "sticky" so that it
       * will no longer have expand adjustments while we perform the drag
       * operation.
       */
      if (gtk_widget_get_child_visible (child->widget) &&
          gtk_widget_get_visible (child->widget))
        {
          child->position_set = TRUE;
          child->position = IS_HORIZONTAL (priv->orientation)
            ? child->alloc.width
            : child->alloc.height;
        }
    }

  if (IS_HORIZONTAL (priv->orientation))
    priv->drag_begin_position = priv->drag_begin->alloc.width;
  else
    priv->drag_begin_position = priv->drag_begin->alloc.height;

  gtk_gesture_pan_set_orientation (gesture, priv->orientation);
  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

  g_signal_emit (self, signals [RESIZE_DRAG_BEGIN], 0, priv->drag_begin->widget);
}

static void
dzl_multi_paned_pan_gesture_drag_end (DzlMultiPaned *self,
                                      gdouble        x,
                                      gdouble        y,
                                      GtkGesturePan *gesture)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GdkEventSequence *sequence;
  GtkEventSequenceState state;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_GESTURE_PAN (gesture));
  g_assert (gesture == priv->gesture);

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  state = gtk_gesture_get_sequence_state (GTK_GESTURE (gesture), sequence);

  if (state != GTK_EVENT_SEQUENCE_CLAIMED)
    goto cleanup;

  g_assert (priv->drag_begin != NULL);

  g_signal_emit (self, signals [RESIZE_DRAG_END], 0, priv->drag_begin->widget);

cleanup:
  priv->drag_begin = NULL;
  priv->drag_begin_position = 0;
  priv->drag_extra_offset = 0;
}

static void
dzl_multi_paned_pan_gesture_pan (DzlMultiPaned   *self,
                                 GtkPanDirection  direction,
                                 gdouble          offset,
                                 GtkGesturePan   *gesture)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkAllocation alloc;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_GESTURE_PAN (gesture));
  g_assert (gesture == priv->gesture);
  g_assert (priv->drag_begin != NULL);

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      if (direction == GTK_PAN_DIRECTION_LEFT)
        offset = -offset;
    }
  else
    {
      g_assert (priv->orientation == GTK_ORIENTATION_VERTICAL);

      if (direction == GTK_PAN_DIRECTION_UP)
        offset = -offset;
    }

  if ((priv->drag_begin_position + offset) < 0)
    priv->drag_extra_offset = (priv->drag_begin_position + offset);
  else
    priv->drag_extra_offset = 0;

  priv->drag_begin->position = MAX (0, priv->drag_begin_position + offset);
  priv->drag_begin->position_set = TRUE;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
dzl_multi_paned_create_pan_gesture (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkGesture *gesture;

  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (priv->gesture == NULL);

  gesture = gtk_gesture_pan_new (GTK_WIDGET (self), GTK_ORIENTATION_HORIZONTAL);
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture), FALSE);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);

  g_signal_connect_object (gesture,
                           "drag-begin",
                           G_CALLBACK (dzl_multi_paned_pan_gesture_drag_begin),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (gesture,
                           "drag-end",
                           G_CALLBACK (dzl_multi_paned_pan_gesture_drag_end),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (gesture,
                           "pan",
                           G_CALLBACK (dzl_multi_paned_pan_gesture_pan),
                           self,
                           G_CONNECT_SWAPPED);

  priv->gesture = GTK_GESTURE_PAN (gesture);
}

static void
dzl_multi_paned_resize_drag_begin (DzlMultiPaned *self,
                                   GtkWidget     *child)
{
  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (child));

}

static void
dzl_multi_paned_resize_drag_end (DzlMultiPaned *self,
                                 GtkWidget     *child)
{
  g_assert (DZL_IS_MULTI_PANED (self));
  g_assert (GTK_IS_WIDGET (child));

}

static void
dzl_multi_paned_get_child_property (GtkContainer *container,
                                    GtkWidget    *widget,
                                    guint         prop_id,
                                    GValue       *value,
                                    GParamSpec   *pspec)
{
  DzlMultiPaned *self = DZL_MULTI_PANED (container);

  switch (prop_id)
    {
    case CHILD_PROP_INDEX:
      g_value_set_int (value, dzl_multi_paned_get_child_index (self, widget));
      break;

    case CHILD_PROP_POSITION:
      g_value_set_int (value, dzl_multi_paned_get_child_position (self, widget));
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
    }
}

static void
dzl_multi_paned_set_child_property (GtkContainer *container,
                                    GtkWidget    *widget,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  DzlMultiPaned *self = DZL_MULTI_PANED (container);

  switch (prop_id)
    {
    case CHILD_PROP_INDEX:
      dzl_multi_paned_set_child_index (self, widget, g_value_get_int (value));
      break;

    case CHILD_PROP_POSITION:
      dzl_multi_paned_set_child_position (self, widget, g_value_get_int (value));
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
    }
}

static void
dzl_multi_paned_finalize (GObject *object)
{
  DzlMultiPaned *self = (DzlMultiPaned *)object;
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_assert (priv->children->len == 0);

  g_clear_pointer (&priv->children, g_array_unref);
  g_clear_object (&priv->gesture);

  G_OBJECT_CLASS (dzl_multi_paned_parent_class)->finalize (object);
}

static void
dzl_multi_paned_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  DzlMultiPaned *self = DZL_MULTI_PANED (object);
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_multi_paned_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  DzlMultiPaned *self = DZL_MULTI_PANED (object);
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);
      for (guint i = 0; i < priv->children->len; i++)
        {
          DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);
          child->position_set = FALSE;
        }
      dzl_multi_paned_update_child_handles (self);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_multi_paned_state_flags_changed (GtkWidget     *widget,
                                     GtkStateFlags  previous_state)
{
  dzl_multi_paned_update_child_handles (DZL_MULTI_PANED (widget));

  GTK_WIDGET_CLASS (dzl_multi_paned_parent_class)->state_flags_changed (widget, previous_state);
}

static void
dzl_multi_paned_class_init (DzlMultiPanedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = dzl_multi_paned_get_property;
  object_class->set_property = dzl_multi_paned_set_property;
  object_class->finalize = dzl_multi_paned_finalize;

  widget_class->get_request_mode = dzl_multi_paned_get_request_mode;
  widget_class->get_preferred_width = dzl_multi_paned_get_preferred_width;
  widget_class->get_preferred_height = dzl_multi_paned_get_preferred_height;
  widget_class->get_preferred_width_for_height = dzl_multi_paned_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = dzl_multi_paned_get_preferred_height_for_width;
  widget_class->size_allocate = dzl_multi_paned_size_allocate;
  widget_class->realize = dzl_multi_paned_realize;
  widget_class->unrealize = dzl_multi_paned_unrealize;
  widget_class->map = dzl_multi_paned_map;
  widget_class->unmap = dzl_multi_paned_unmap;
  widget_class->draw = dzl_multi_paned_draw;
  widget_class->state_flags_changed = dzl_multi_paned_state_flags_changed;

  container_class->add = dzl_multi_paned_add;
  container_class->remove = dzl_multi_paned_remove;
  container_class->get_child_property = dzl_multi_paned_get_child_property;
  container_class->set_child_property = dzl_multi_paned_set_child_property;
  container_class->forall = dzl_multi_paned_forall;

  klass->resize_drag_begin = dzl_multi_paned_resize_drag_begin;
  klass->resize_drag_end = dzl_multi_paned_resize_drag_end;

  gtk_widget_class_set_css_name (widget_class, "dzlmultipaned");

  properties [PROP_ORIENTATION] =
    g_param_spec_enum ("orientation",
                       "Orientation",
                       "Orientation",
                       GTK_TYPE_ORIENTATION,
                       GTK_ORIENTATION_VERTICAL,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  child_properties [CHILD_PROP_INDEX] =
    g_param_spec_int ("index",
                      "Index",
                      "The index of the child",
                      -1,
                      G_MAXINT,
                      -1,
                      (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  child_properties [CHILD_PROP_POSITION] =
    g_param_spec_int ("position",
                      "Position",
                      "Position",
                      -1,
                      G_MAXINT,
                      0,
                      (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gtk_container_class_install_child_properties (container_class, N_CHILD_PROPS, child_properties);

  style_properties [STYLE_PROP_HANDLE_SIZE] =
    g_param_spec_int ("handle-size",
                      "Handle Size",
                      "Width of the resize handle",
                      0,
                      G_MAXINT,
                      1,
                      (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  gtk_widget_class_install_style_property (widget_class, style_properties [STYLE_PROP_HANDLE_SIZE]);

  signals [RESIZE_DRAG_BEGIN] =
    g_signal_new ("resize-drag-begin",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlMultiPanedClass, resize_drag_begin),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

  signals [RESIZE_DRAG_END] =
    g_signal_new ("resize-drag-end",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlMultiPanedClass, resize_drag_end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
}

static void
dzl_multi_paned_init (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  priv->orientation = GTK_ORIENTATION_VERTICAL;
  priv->children = g_array_new (FALSE, TRUE, sizeof (DzlMultiPanedChild));

  dzl_multi_paned_create_pan_gesture (self);
}

GtkWidget *
dzl_multi_paned_new (void)
{
  return g_object_new (DZL_TYPE_MULTI_PANED, NULL);
}

guint
dzl_multi_paned_get_n_children (DzlMultiPaned *self)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MULTI_PANED (self), 0);

  return priv->children ? priv->children->len : 0;
}

/**
 * dzl_multi_paned_get_nth_child:
 * @self: a #DzlMultiPaned
 *
 * Gets the @nth child of the #DzlMultiPaned.
 *
 * It is an error to call this function with a value >= the result of
 * dzl_multi_paned_get_nth_child().
 *
 * The index starts from 0.
 *
 * Returns: (transfer none): A #GtkWidget
 */
GtkWidget *
dzl_multi_paned_get_nth_child (DzlMultiPaned *self,
                               guint          nth)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_MULTI_PANED (self), NULL);
  g_return_val_if_fail (nth < priv->children->len, NULL);

  return g_array_index (priv->children, DzlMultiPanedChild, nth).widget;
}

/**
 * dzl_multi_paned_get_at_point:
 * @self: a #DzlMultiPaned
 * @x: x coordinate
 * @y: y coordinate
 *
 * Locates the widget at position x,y within widget.
 *
 * @x and @y should be relative to @self.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget or %NULL
 *
 * Since: 3.28
 */
GtkWidget *
dzl_multi_paned_get_at_point (DzlMultiPaned *self,
                              gint           x,
                              gint           y)
{
  DzlMultiPanedPrivate *priv = dzl_multi_paned_get_instance_private (self);
  GtkAllocation alloc;

  g_return_val_if_fail (DZL_IS_MULTI_PANED (self), NULL);

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  if (IS_HORIZONTAL (priv->orientation))
    {
      for (guint i = 0; i < priv->children->len; i++)
        {
          const DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

          if (x >= child->alloc.x && x < (child->alloc.x + child->alloc.width))
            return child->widget;
        }
    }
  else
    {
      for (guint i = 0; i < priv->children->len; i++)
        {
          const DzlMultiPanedChild *child = &g_array_index (priv->children, DzlMultiPanedChild, i);

          if (y >= child->alloc.y && y < (child->alloc.y + child->alloc.height))
            return child->widget;
        }
    }

  return NULL;
}
