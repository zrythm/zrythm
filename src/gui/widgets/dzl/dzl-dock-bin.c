/* dzl-dock-bin.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "gui/widgets/dzl/dzl-child-property-action.h"
#include "gui/widgets/dzl/dzl-dock-bin.h"
#include "gui/widgets/dzl/dzl-dock-bin-edge-private.h"
#include "gui/widgets/dzl/dzl-dock-item.h"

#define HANDLE_WIDTH  3
#define HANDLE_HEIGHT 3

typedef enum
{
  DZL_DOCK_BIN_CHILD_LEFT   = GTK_POS_LEFT,
  DZL_DOCK_BIN_CHILD_RIGHT  = GTK_POS_RIGHT,
  DZL_DOCK_BIN_CHILD_TOP    = GTK_POS_TOP,
  DZL_DOCK_BIN_CHILD_BOTTOM = GTK_POS_BOTTOM,
  DZL_DOCK_BIN_CHILD_CENTER = 4,
  LAST_DZL_DOCK_BIN_CHILD   = 5
} DzlDockBinChildType;

typedef struct
{
  /*
   * The child widget in question.
   * Typically this is a DzlDockBinEdge, but the
   * center widget can be whatever.
   */
  GtkWidget *widget;

  /*
   * The GdkWindow for the handle to resize the edge.
   * This is an input only window, the pane handle is drawn
   * with CSS by whatever styling the application has chose.
   */
  GdkWindow *handle;

  /*
   * When dragging, we need to know our offset relative to the
   * grab position to alter preferred size requests.
   */
  gint drag_offset;

  /*
   * This is the position of the child before the drag started.
   * We use this, combined with @drag_offset to determine the
   * size the child should be in the drag operation.
   */
  gint drag_begin_position;

  /*
   * Priority child property used to alter which child is
   * dominant in each slice stage. See
   * dzl_dock_bin_get_children_preferred_width() for more information
   * on how the slicing is performed.
   */
  gint priority;

  /*
   * Cached size request used during size allocation.
   */
  GtkRequisition min_req;
  GtkRequisition nat_req;

  /*
   * The type of child. The DZL_DOCK_BIN_CHILD_CENTER is always
   * the last child, and our sort function ensures that.
   */
  DzlDockBinChildType type : 3;

  /*
   * If the panel is pinned, this will be set to TRUE. A pinned panel
   * means that it is displayed juxtapose the center child, where as
   * an unpinned child is floating above teh center child.
   */
  guint pinned : 1;

  /*
   * Tracks if the widget was pinned before the start of an animation
   * sequence, as we may change the pinned state during the animation.
   */
  guint pre_anim_pinned : 1;
} DzlDockBinChild;

typedef struct
{
  /*
   * All of our dock children, including edges and center child.
   */
  DzlDockBinChild children[LAST_DZL_DOCK_BIN_CHILD];

  /*
   * Actions used to toggle edge visibility.
   */
  GSimpleActionGroup *actions;

  /*
   * The pan gesture is used to resize edges.
   */
  GtkGesturePan *pan_gesture;

  /*
   * While in a pan gesture, we need to drag the current edge
   * being dragged. This is left, right, top, or bottom only.
   */
  DzlDockBinChild *drag_child;

  /*
   * We need to track the position during a DnD request. We can use this
   * to highlight the area where the drop will occur.
   */
  gint dnd_drag_x;
  gint dnd_drag_y;
} DzlDockBinPrivate;

static void dzl_dock_bin_init_buildable_iface (GtkBuildableIface    *iface);
static void dzl_dock_bin_init_dock_iface      (DzlDockInterface     *iface);
static void dzl_dock_bin_init_dock_item_iface (DzlDockItemInterface *iface);
static void dzl_dock_bin_create_edge          (DzlDockBin           *self,
                                               DzlDockBinChild      *child,
                                               DzlDockBinChildType   type);

G_DEFINE_TYPE_EXTENDED (DzlDockBin, dzl_dock_bin, GTK_TYPE_CONTAINER, 0,
                        G_ADD_PRIVATE (DzlDockBin)
                        G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, dzl_dock_bin_init_buildable_iface)
                        G_IMPLEMENT_INTERFACE (DZL_TYPE_DOCK_ITEM, dzl_dock_bin_init_dock_item_iface)
                        G_IMPLEMENT_INTERFACE (DZL_TYPE_DOCK, dzl_dock_bin_init_dock_iface))

enum {
  PROP_0,
  PROP_LEFT_VISIBLE,
  PROP_RIGHT_VISIBLE,
  PROP_TOP_VISIBLE,
  PROP_BOTTOM_VISIBLE,
  N_PROPS,

  PROP_MANAGER,
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_PINNED,
  CHILD_PROP_POSITION,
  CHILD_PROP_PRIORITY,
  N_CHILD_PROPS
};

static GParamSpec *properties [N_PROPS];
static GParamSpec *child_properties [N_CHILD_PROPS];
static const gchar *pinned_names[] = {
  "left-pinned",
  "right-pinned",
  "top-pinned",
  "bottom-pinned",
  NULL,
  NULL
};
static const gchar *visible_names[] = {
  "left-visible",
  "right-visible",
  "top-visible",
  "bottom-visible",
  NULL,
  NULL
};

static DzlDockBinChild *
dzl_dock_bin_get_child_typed (DzlDockBin          *self,
                              DzlDockBinChildType  type)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (type >= DZL_DOCK_BIN_CHILD_LEFT);
  g_warn_if_fail (type < LAST_DZL_DOCK_BIN_CHILD);

  for (guint i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      if (priv->children[i].type == type)
        return &priv->children[i];
    }

  g_warn_if_reached ();

  return NULL;
}

static GtkWidget *
get_child_widget (DzlDockBin          *self,
                  DzlDockBinChildType  type)
{
  switch (type)
    {
    case DZL_DOCK_BIN_CHILD_LEFT:
      return dzl_dock_bin_get_left_edge (self);

    case DZL_DOCK_BIN_CHILD_RIGHT:
      return dzl_dock_bin_get_right_edge (self);

    case DZL_DOCK_BIN_CHILD_TOP:
      return dzl_dock_bin_get_top_edge (self);

    case DZL_DOCK_BIN_CHILD_BOTTOM:
      return dzl_dock_bin_get_bottom_edge (self);

    case DZL_DOCK_BIN_CHILD_CENTER:
    case LAST_DZL_DOCK_BIN_CHILD:
    default:
      return NULL;
    }
}

static gboolean
get_visible (DzlDockBin          *self,
             DzlDockBinChildType  type)
{
  GtkWidget *child;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (type >= DZL_DOCK_BIN_CHILD_LEFT);
  g_warn_if_fail (type <= DZL_DOCK_BIN_CHILD_BOTTOM);

  child = get_child_widget (self, type);

  return DZL_IS_DOCK_REVEALER (child) &&
         dzl_dock_revealer_get_reveal_child (DZL_DOCK_REVEALER (child));
}

static void
set_visible (DzlDockBin          *self,
             DzlDockBinChildType  type,
             gboolean             visible)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GtkWidget *widget;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (type >= DZL_DOCK_BIN_CHILD_LEFT);
  g_warn_if_fail (type <= DZL_DOCK_BIN_CHILD_BOTTOM);

  /* Ensure the panel is created */
  widget = get_child_widget (self, type);

  /*
   * When animating, we should set the panel as "unpinned" and then
   * swap the pinned status after the animation completes. That will
   * allow us to have more smooth animations without having to keep
   * updating all that sizing machinery.
   */

  if (DZL_IS_DOCK_REVEALER (widget))
    {
      DzlDockBinChild *child = &priv->children[type];

      if (visible != dzl_dock_revealer_get_reveal_child (DZL_DOCK_REVEALER (widget)))
        {

          /* If the widget isn't currently visible, first make it visible but keep
           * the reveal-child set as FALSE. That allows us to animate in from a
           * hiddent initial state.
           */
          if (visible && !gtk_widget_get_visible (widget))
            {
              dzl_dock_revealer_set_reveal_child (DZL_DOCK_REVEALER (widget), FALSE);
              gtk_widget_show (child->widget);
            }

          /* Only stash state if there is not an animation in progress.
           * Otherwise we stomp on our previous state.
           */
          if (!dzl_dock_revealer_is_animating (DZL_DOCK_REVEALER (widget)))
            child->pre_anim_pinned = child->pinned;

          /*
           * We only want to "unpin" the panel when we animate out. Generally,
           * shrinking widgets can be made fast by re-using their existing
           * content, but making things bigger has very large performance
           * costs.  Expecially with textviews/treeviews.
           */
          if (!visible)
            child->pinned = FALSE;

          dzl_dock_revealer_set_reveal_child (DZL_DOCK_REVEALER (widget), visible);

          g_object_notify (G_OBJECT (self), visible_names [type]);
          gtk_widget_queue_resize (GTK_WIDGET (self));
        }
    }
}

static gint
dzl_dock_bin_child_compare (gconstpointer a,
                            gconstpointer b,
                            void * ptr)
{
  const DzlDockBinChild *child_a = a;
  const DzlDockBinChild *child_b = b;

  if (child_a->type == DZL_DOCK_BIN_CHILD_CENTER)
    return 1;
  else if (child_b->type == DZL_DOCK_BIN_CHILD_CENTER)
    return -1;

  if ((child_a->pinned ^ child_b->pinned) != 0)
    return child_a->pinned - child_b->pinned;

  return child_a->priority - child_b->priority;
}

static void
dzl_dock_bin_resort_children (DzlDockBin *self)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  /*
   * Sort the children by priority/pinned status, but do not change
   * the position of the DZL_DOCK_BIN_CHILD_CENTER child. It should
   * always be at the last position.
   */

  g_qsort_with_data (
    &priv->children[0],
    DZL_DOCK_BIN_CHILD_CENTER,
    sizeof (DzlDockBinChild),
    (GCompareDataFunc)dzl_dock_bin_child_compare,
    NULL);

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static DzlDockBinChild *
dzl_dock_bin_get_child (DzlDockBin *self,
                        GtkWidget  *widget)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  for (i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      if ((GtkWidget *)child->widget == widget)
        return child;
    }

  g_warn_if_reached ();

  return NULL;
}

static gboolean
dzl_dock_bin_focus (GtkWidget        *widget,
                    GtkDirectionType  dir)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *next_child = NULL;
  DzlDockBinChild *child = NULL;
  GtkWidget *focus_child;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  if (!gtk_widget_get_can_focus (widget))
    return GTK_WIDGET_CLASS (dzl_dock_bin_parent_class)->focus (widget, dir);

  if (!(focus_child = gtk_container_get_focus_child (GTK_CONTAINER (self))))
    {
      child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);

      g_warn_if_fail (child != NULL);

      if (child->widget != NULL)
        {
          if (gtk_widget_child_focus (child->widget, dir))
            return TRUE;
        }

      return FALSE;
    }

  for (guint i = 0; i < DZL_DOCK_BIN_CHILD_CENTER; i++)
    {
      if (priv->children[i].widget == focus_child)
        {
          child = &priv->children[i];
          break;
        }
    }

  if (child != NULL)
    {
      switch (dir)
        {
        case GTK_DIR_TAB_BACKWARD:
        case GTK_DIR_LEFT:
          if (child->type == DZL_DOCK_BIN_CHILD_CENTER)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_LEFT);
          else if (child->type == DZL_DOCK_BIN_CHILD_RIGHT)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);
          break;

        case GTK_DIR_TAB_FORWARD:
        case GTK_DIR_RIGHT:
          if (child->type == DZL_DOCK_BIN_CHILD_LEFT)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);
          else if (child->type == DZL_DOCK_BIN_CHILD_CENTER)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_RIGHT);
          break;

        case GTK_DIR_UP:
          if (child->type == DZL_DOCK_BIN_CHILD_CENTER)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_TOP);
          else if (child->type == DZL_DOCK_BIN_CHILD_BOTTOM)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);
          break;

        case GTK_DIR_DOWN:
          if (child->type == DZL_DOCK_BIN_CHILD_TOP)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);
          else if (child->type == DZL_DOCK_BIN_CHILD_CENTER)
            next_child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_BOTTOM);
          break;

        default:
          break;
        }
    }

  if (next_child == NULL || next_child->widget == NULL)
    {
      if (dir == GTK_DIR_UP || dir == GTK_DIR_DOWN || dir == GTK_DIR_LEFT || dir == GTK_DIR_RIGHT)
        {
          if (gtk_widget_keynav_failed (GTK_WIDGET (self), dir))
            return TRUE;
        }

      return FALSE;
    }

  g_warn_if_fail (next_child != NULL);
  g_warn_if_fail (next_child->widget != NULL);

  return gtk_widget_child_focus (next_child->widget, dir);
}

static void
dzl_dock_bin_add (GtkContainer *container,
                  GtkWidget    *widget)
{
  DzlDockBin *self = (DzlDockBin *)container;
  DzlDockBinChild *child;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);

  if (child->widget != NULL)
    {
      g_warning ("Attempt to add a %s to a %s, but it already has a child of type %s",
                 G_OBJECT_TYPE_NAME (widget),
                 G_OBJECT_TYPE_NAME (self),
                 G_OBJECT_TYPE_NAME (child->widget));
      return;
    }

  if (DZL_IS_DOCK_ITEM (widget) &&
      !dzl_dock_item_adopt (DZL_DOCK_ITEM (self), DZL_DOCK_ITEM (widget)))
    {
      g_warning ("Child of type %s has a different DzlDockManager than %s",
                 G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (self));
      return;
    }

  child->widget = g_object_ref_sink (widget);
  gtk_widget_set_parent (widget, GTK_WIDGET (self));

  if (DZL_IS_DOCK_ITEM (widget))
    dzl_dock_item_emit_presented (DZL_DOCK_ITEM (widget));

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
dzl_dock_bin_notify_reveal_child (DzlDockBin *self,
                                  GParamSpec *pspec,
                                  GtkWidget  *child)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (child));

  for (guint i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      DzlDockBinChild *ele = &priv->children [i];

      if (ele->widget == child)
        g_object_notify (G_OBJECT (self), visible_names [ele->type]);
    }
}

static void
dzl_dock_bin_notify_child_revealed (DzlDockBin *self,
                                    GParamSpec *pspec,
                                    GtkWidget  *child)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (child));

  for (guint i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      DzlDockBinChild *ele = &priv->children [i];

      if (ele->widget == child)
        {
          ele->pinned = ele->pre_anim_pinned;
          gtk_widget_queue_resize (GTK_WIDGET (self));
          break;
        }
    }
}

static void
dzl_dock_bin_remove (GtkContainer *container,
                     GtkWidget    *widget)
{
  DzlDockBin *self = (DzlDockBin *)container;
  DzlDockBinChild *child;

  g_return_if_fail (DZL_IS_DOCK_BIN (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  child = dzl_dock_bin_get_child (self, widget);
  gtk_widget_unparent (child->widget);
  g_clear_object (&child->widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (gtk_widget_destroyed),
                                        &child->widget);
  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (dzl_dock_bin_notify_reveal_child),
                                        self);
  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (dzl_dock_bin_notify_child_revealed),
                                        self);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
dzl_dock_bin_forall (GtkContainer *container,
                     gboolean      include_internal,
                     GtkCallback   callback,
                     gpointer      user_data)
{
  DzlDockBin *self = (DzlDockBin *)container;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *child;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (callback != NULL);

  /*
   * Always call the "center" child callback first. This helps ensure that
   * it is the first child to be rendered. We need that to ensure that panels
   * are drawn *above* the center, even when floating. Note that the center
   * child is always the last child in the array.
   */
  child = &priv->children [G_N_ELEMENTS (priv->children) - 1];
  if (child->widget != NULL)
    callback (child->widget, user_data);

  /*
   * Normally, we would iterate the array backwards so that we can ensure that
   * the list is safe against widget destruction. However, since we have a
   * fixed size array, we can walk forwards safely.  This helps ensure we
   * preserve draw ordering for the various panels when we chain up to the
   * container draw vfunc.
   */

  for (i = 0; i < G_N_ELEMENTS (priv->children) - 1; i++)
    {
      child = &priv->children[i];

      if (child->widget != NULL)
        callback (GTK_WIDGET (child->widget), user_data);
    }
}

static void
dzl_dock_bin_get_children_preferred_width (DzlDockBin      *self,
                                           DzlDockBinChild *children,
                                           gint             n_children,
                                           gint            *min_width,
                                           gint            *nat_width)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *child = children;
  gint child_min_width = 0;
  gint child_nat_width = 0;
  gint neighbor_min_width = 0;
  gint neighbor_nat_width = 0;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (children != NULL);
  g_warn_if_fail (n_children > 0);
  g_warn_if_fail (min_width != NULL);
  g_warn_if_fail (nat_width != NULL);

  *min_width = 0;
  *nat_width = 0;

  /*
   * We have a fairly simple rule for deducing the size request of
   * the children layout. Since children edges can have any priority,
   * we need to know how to slice them into areas that allow us to
   * combine (additive) or negotiate (maximum) widths with the
   * neighboring widgets.
   *
   *          .
   *          .
   *     +----+---------------------------------+
   *     |    |              2                  |
   *     |    +=================================+.....
   *     |    |                            |    |
   *     |    |                            |    |
   *     | 1  |              5             |    |
   *     |    |                            | 3  |
   *     |    +==.==.==.==.==.==.==.==.==.=+    |
   *     |    |              4             |    |
   *     +----+----------------------------+----+
   *          .                            .
   *          .                            .
   *
   * The children are sorted in their weighting order. Each child
   * will dominate the leftover allocation, in the orientation that
   * matters.
   *
   * 1 and 3 in the diagram above will always be additive with their
   * neighbors horizontal neighbors. See the guide does for how this
   * gets sliced. Even if 3 were dominant (instead of 2), it would still
   * be additive to its neighbors. Same for 1.
   *
   * Both 2 and 4, will always negotiate their widths with the next
   * child.
   *
   * This allows us to make a fairly simple recursive function to
   * size ourselves and then call again with the next child, working our
   * way down to 5 (the center widget).
   *
   * At this point, we walk back up the recursive-stack and do our
   * adding or negotiating.
   */

  if (child->widget != NULL)
    gtk_widget_get_preferred_width (child->widget, &child_min_width, &child_nat_width);

  if (child == priv->drag_child)
    child_nat_width = MAX (child_min_width,
                           child->drag_begin_position + child->drag_offset);

  if (n_children > 1)
    dzl_dock_bin_get_children_preferred_width (self,
                                               &children [1],
                                               n_children - 1,
                                               &neighbor_min_width,
                                               &neighbor_nat_width);

  switch (child->type)
    {
    case DZL_DOCK_BIN_CHILD_LEFT:
    case DZL_DOCK_BIN_CHILD_RIGHT:
      if (child->pinned)
        {
          *min_width = (child_min_width + neighbor_min_width);
          *nat_width = (child_nat_width + neighbor_nat_width);
        }
      else
        {
          *min_width = MAX (child_min_width, neighbor_min_width);
          *nat_width = MAX (child_nat_width, neighbor_nat_width);
        }
      break;

    case DZL_DOCK_BIN_CHILD_TOP:
    case DZL_DOCK_BIN_CHILD_BOTTOM:
      *min_width = MAX (child_min_width, neighbor_min_width);
      *nat_width = MAX (child_nat_width, neighbor_nat_width);
      break;

    case DZL_DOCK_BIN_CHILD_CENTER:
      *min_width = child_min_width;
      *nat_width = child_min_width;
      break;

    case LAST_DZL_DOCK_BIN_CHILD:
    default:
      g_warn_if_reached ();
    }

  child->min_req.width = *min_width;
  child->nat_req.width = *nat_width;
}

static void
dzl_dock_bin_get_preferred_width (GtkWidget *widget,
                                  gint      *min_width,
                                  gint      *nat_width)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (min_width != NULL);
  g_warn_if_fail (nat_width != NULL);

  dzl_dock_bin_get_children_preferred_width (self,
                                             priv->children,
                                             G_N_ELEMENTS (priv->children),
                                             min_width,
                                             nat_width);
}

static void
dzl_dock_bin_get_children_preferred_height (DzlDockBin      *self,
                                            DzlDockBinChild *children,
                                            gint             n_children,
                                            gint            *min_height,
                                            gint            *nat_height)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *child = children;
  gint child_min_height = 0;
  gint child_nat_height = 0;
  gint neighbor_min_height = 0;
  gint neighbor_nat_height = 0;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (children != NULL);
  g_warn_if_fail (n_children > 0);
  g_warn_if_fail (min_height != NULL);
  g_warn_if_fail (nat_height != NULL);

  *min_height = 0;
  *nat_height = 0;

  /*
   * See dzl_dock_bin_get_children_preferred_width() for more information on
   * how this works. This works just like that but the negotiated/additive
   * operations are switched between the left/right and top/bottom.
   */

  if (child->widget != NULL)
    gtk_widget_get_preferred_height (child->widget, &child_min_height, &child_nat_height);

  if (child == priv->drag_child)
    child_nat_height = MAX (child_min_height,
                            child->drag_begin_position + child->drag_offset);

  if (n_children > 1)
    dzl_dock_bin_get_children_preferred_height (self,
                                                &children [1],
                                                n_children - 1,
                                                &neighbor_min_height,
                                                &neighbor_nat_height);

  switch (child->type)
    {
    case DZL_DOCK_BIN_CHILD_LEFT:
    case DZL_DOCK_BIN_CHILD_RIGHT:
      *min_height = MAX (child_min_height, neighbor_min_height);
      *nat_height = MAX (child_nat_height, neighbor_nat_height);
      break;

    case DZL_DOCK_BIN_CHILD_TOP:
    case DZL_DOCK_BIN_CHILD_BOTTOM:
      if (child->pinned)
        {
          *min_height = (child_min_height + neighbor_min_height);
          *nat_height = (child_nat_height + neighbor_nat_height);
        }
      else
        {
          *min_height = MAX (child_min_height, neighbor_min_height);
          *nat_height = MAX (child_nat_height, neighbor_nat_height);
        }
      break;

    case DZL_DOCK_BIN_CHILD_CENTER:
      *min_height = child_min_height;
      *nat_height = child_min_height;
      break;

    case LAST_DZL_DOCK_BIN_CHILD:
    default:
      g_warn_if_reached ();
    }

  child->min_req.height = *min_height;
  child->nat_req.height = *nat_height;
}

static void
dzl_dock_bin_get_preferred_height (GtkWidget *widget,
                                   gint      *min_height,
                                   gint      *nat_height)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (min_height != NULL);
  g_warn_if_fail (nat_height != NULL);

  dzl_dock_bin_get_children_preferred_height (self,
                                              priv->children,
                                              G_N_ELEMENTS (priv->children),
                                              min_height,
                                              nat_height);
}

static void
dzl_dock_bin_negotiate_size (DzlDockBin           *self,
                             const GtkAllocation  *allocation,
                             const GtkRequisition *child_min,
                             const GtkRequisition *child_nat,
                             const GtkRequisition *neighbor_min,
                             const GtkRequisition *neighbor_nat,
                             GtkAllocation        *child_alloc)
{
  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (allocation != NULL);
  g_warn_if_fail (child_min != NULL);
  g_warn_if_fail (child_nat != NULL);
  g_warn_if_fail (neighbor_min != NULL);
  g_warn_if_fail (neighbor_nat != NULL);
  g_warn_if_fail (child_alloc != NULL);

  if (allocation->width - child_nat->width < neighbor_min->width)
    child_alloc->width = allocation->width - neighbor_min->width;
  else
    child_alloc->width = child_nat->width;

  if (allocation->height - child_nat->height < neighbor_min->height)
    child_alloc->height = allocation->height - neighbor_min->height;
  else
    child_alloc->height = child_nat->height;
}

static void
dzl_dock_bin_child_size_allocate (DzlDockBin      *self,
                                  DzlDockBinChild *children,
                                  gint             n_children,
                                  GtkAllocation   *allocation)
{
  DzlDockBinChild *child = children;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (children != NULL);
  g_warn_if_fail (n_children >= 1);
  g_warn_if_fail (allocation != NULL);

  if (n_children == 1)
    {
      g_warn_if_fail (child->type == DZL_DOCK_BIN_CHILD_CENTER);

      if (child->widget != NULL && gtk_widget_get_visible (child->widget))
        gtk_widget_size_allocate (child->widget, allocation);

      return;
    }

  if (child->widget != NULL &&
      gtk_widget_get_visible (child->widget) &&
      gtk_widget_get_child_visible (child->widget))
    {
      GtkAllocation child_alloc = { 0 };
      GtkAllocation handle_alloc = { 0 };
      GtkRequisition neighbor_min = { 0 };
      GtkRequisition neighbor_nat = { 0 };
      GtkStyleContext *style_context = gtk_widget_get_style_context (child->widget);
      GtkStateFlags state = gtk_style_context_get_state (style_context);
      GtkBorder margin;

      gtk_style_context_get_margin (style_context, state, &margin);

      dzl_dock_bin_get_children_preferred_height (self, child, 1,
                                                  &child->min_req.height,
                                                  &child->nat_req.height);

      dzl_dock_bin_get_children_preferred_width (self, child, 1,
                                                 &child->min_req.width,
                                                 &child->nat_req.width);

      if (child->pinned && n_children > 1)
        {
          dzl_dock_bin_get_children_preferred_height (self,
                                                      &children [1],
                                                      n_children - 1,
                                                      &neighbor_min.height,
                                                      &neighbor_nat.height);

          dzl_dock_bin_get_children_preferred_width (self,
                                                     &children [1],
                                                     n_children - 1,
                                                     &neighbor_min.width,
                                                     &neighbor_nat.width);
        }

      dzl_dock_bin_negotiate_size (self,
                                   allocation,
                                   &child->min_req,
                                   &child->nat_req,
                                   &neighbor_min,
                                   &neighbor_nat,
                                   &child_alloc);

      switch (child->type)
        {
        case DZL_DOCK_BIN_CHILD_LEFT:
          child_alloc.x = allocation->x;
          child_alloc.y = allocation->y;
          child_alloc.height = allocation->height;

          if (child->pinned)
            {
              allocation->x += child_alloc.width;
              allocation->width -= child_alloc.width;
            }

          break;


        case DZL_DOCK_BIN_CHILD_RIGHT:
          child_alloc.x = allocation->x + allocation->width - child_alloc.width;
          child_alloc.y = allocation->y;
          child_alloc.height = allocation->height;

          if (child->pinned)
            allocation->width -= child_alloc.width;

          break;


        case DZL_DOCK_BIN_CHILD_TOP:
          child_alloc.x = allocation->x;
          child_alloc.y = allocation->y;
          child_alloc.width = allocation->width;

          if (child->pinned)
            {
              allocation->y += child_alloc.height;
              allocation->height -= child_alloc.height;
            }

          break;


        case DZL_DOCK_BIN_CHILD_BOTTOM:
          child_alloc.x = allocation->x;
          child_alloc.y = allocation->y + allocation->height - child_alloc.height;
          child_alloc.width = allocation->width;

          if (child->pinned)
            allocation->height -= child_alloc.height;

          break;


        case DZL_DOCK_BIN_CHILD_CENTER:
        case LAST_DZL_DOCK_BIN_CHILD:
        default:
          g_warn_if_reached ();
          break;
        }

      handle_alloc = child_alloc;

      switch (child->type)
        {
        case DZL_DOCK_BIN_CHILD_LEFT:

          /*
           * When left-to-right, we often have a scrollbar to deal
           * with right here. So fudge the allocation position a bit
           * to the right so that the scrollbar can still be easily
           * hovered.
           */
          if (gtk_widget_get_direction (child->widget) == GTK_TEXT_DIR_LTR)
            {
              handle_alloc.x += handle_alloc.width - (HANDLE_WIDTH / 2);
              handle_alloc.width = HANDLE_WIDTH;
            }
          else
            {
              handle_alloc.x += handle_alloc.width - HANDLE_WIDTH;
              handle_alloc.width = HANDLE_WIDTH;
            }

          handle_alloc.x -= margin.right;

          break;

        case DZL_DOCK_BIN_CHILD_RIGHT:
          handle_alloc.width = HANDLE_WIDTH;

          if (gtk_widget_get_direction (child->widget) == GTK_TEXT_DIR_RTL)
            handle_alloc.x -= (HANDLE_WIDTH / 2);

          handle_alloc.x += margin.left;

          break;

        case DZL_DOCK_BIN_CHILD_BOTTOM:
          handle_alloc.height = HANDLE_HEIGHT;
          handle_alloc.y += margin.top;
          break;

        case DZL_DOCK_BIN_CHILD_TOP:
          handle_alloc.y += handle_alloc.height - HANDLE_HEIGHT;
          handle_alloc.y -= margin.bottom;
          handle_alloc.height = HANDLE_HEIGHT;
          break;

        case DZL_DOCK_BIN_CHILD_CENTER:
        case LAST_DZL_DOCK_BIN_CHILD:
        default:
          break;
        }

      if (child_alloc.width > 0 && child_alloc.height > 0 && child->handle)
        gdk_window_move_resize (child->handle,
                                handle_alloc.x, handle_alloc.y,
                                handle_alloc.width, handle_alloc.height);

      gtk_widget_size_allocate (child->widget, &child_alloc);
    }

  dzl_dock_bin_child_size_allocate (self, &children [1], n_children - 1, allocation);
}

static void
dzl_dock_bin_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GtkAllocation child_allocation;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (gtk_widget_get_window (widget),
                              allocation->x,
                              allocation->y,
                              child_allocation.width,
                              child_allocation.height);
    }

  dzl_dock_bin_child_size_allocate (self,
                                    priv->children,
                                    G_N_ELEMENTS (priv->children),
                                    &child_allocation);

  /*
   * Hide all of the handle input windows that should be hidden
   * because the child has an empty allocation.
   */

  for (i = DZL_DOCK_BIN_CHILD_CENTER; i > 0; i--)
    {
      DzlDockBinChild *child = &priv->children [i - 1];

      if (child->handle != NULL)
        {
          if (DZL_IS_DOCK_BIN_EDGE (child->widget))
            {
              if (gtk_widget_get_realized (child->widget))
                gdk_window_raise (gtk_widget_get_window (child->widget));

              if (dzl_dock_revealer_get_reveal_child (DZL_DOCK_REVEALER (child->widget)))
                gdk_window_show (child->handle);
            }
          else
            gdk_window_hide (child->handle);
        }
    }

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
dzl_dock_bin_set_child_pinned (DzlDockBin *self,
                               GtkWidget  *widget,
                               gboolean    pinned)
{
  DzlDockBinChild *child;
  GtkStyleContext *style_context;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  child = dzl_dock_bin_get_child (self, widget);

  if (child->type == DZL_DOCK_BIN_CHILD_CENTER)
    return;

  child->pinned = !!pinned;

  style_context = gtk_widget_get_style_context (widget);

  if (child->pinned)
    gtk_style_context_add_class (style_context, DZL_DOCK_BIN_STYLE_CLASS_PINNED);
  else
    gtk_style_context_remove_class (style_context, DZL_DOCK_BIN_STYLE_CLASS_PINNED);

  child->pre_anim_pinned = child->pinned;

  dzl_dock_bin_resort_children (self);

  gtk_widget_queue_resize (GTK_WIDGET (self));

  if (child->widget != NULL)
    gtk_container_child_notify_by_pspec (GTK_CONTAINER (self), child->widget,
                                         child_properties [CHILD_PROP_PINNED]);
}

static void
dzl_dock_bin_set_child_priority (DzlDockBin *self,
                                 GtkWidget  *widget,
                                 gint        priority)
{
  DzlDockBinChild *child;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  child = dzl_dock_bin_get_child (self, widget);
  child->priority = priority;

  dzl_dock_bin_resort_children (self);

  gtk_widget_queue_resize (GTK_WIDGET (self));

  if (child->widget != NULL)
    gtk_container_child_notify_by_pspec (GTK_CONTAINER (self), child->widget,
                                         child_properties [CHILD_PROP_PRIORITY]);
}

static void
dzl_dock_bin_create_child_handle (DzlDockBin      *self,
                                  DzlDockBinChild *child)
{
  GdkWindowAttr attributes = { 0 };
  GdkDisplay *display;
  GdkWindow *parent;
  const gchar *cursor_name;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (child != NULL);
  g_warn_if_fail (child->type < DZL_DOCK_BIN_CHILD_CENTER);
  g_warn_if_fail (child->handle == NULL);

  display = gtk_widget_get_display (GTK_WIDGET (self));
  parent = gtk_widget_get_window (GTK_WIDGET (self));

  cursor_name = (child->type == DZL_DOCK_BIN_CHILD_LEFT || child->type == DZL_DOCK_BIN_CHILD_RIGHT)
              ? "col-resize"
              : "row-resize";

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = -1;
  attributes.y = -1;
  attributes.width = 1;
  attributes.height = 1;
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

static void
dzl_dock_bin_destroy_child_handle (DzlDockBin      *self,
                                   DzlDockBinChild *child)
{
  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (child != NULL);
  g_warn_if_fail (child->type < DZL_DOCK_BIN_CHILD_CENTER);

  if (child->handle != NULL)
    {
      gtk_widget_unregister_window (GTK_WIDGET (self), child->handle);
      gdk_window_destroy (child->handle);
      child->handle = NULL;
    }
}

static void
dzl_dock_bin_realize (GtkWidget *widget)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GdkWindowAttr attributes = { 0 };
  GdkWindow *parent;
  GdkWindow *window;
  GtkAllocation alloc;
  gint attributes_mask = 0;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  gtk_widget_set_realized (GTK_WIDGET (self), TRUE);

  parent = gtk_widget_get_parent_window (GTK_WIDGET (self));

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (GTK_WIDGET (self));
  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.event_mask = 0;

  attributes_mask = (GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL);

  window = gdk_window_new (parent, &attributes, attributes_mask);
  gtk_widget_set_window (GTK_WIDGET (self), window);
  gtk_widget_register_window (GTK_WIDGET (self), window);

  for (i = 0; i < DZL_DOCK_BIN_CHILD_CENTER; i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      dzl_dock_bin_create_child_handle (self, child);
    }
}

static void
dzl_dock_bin_unrealize (GtkWidget *widget)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  for (i = 0; i < DZL_DOCK_BIN_CHILD_CENTER; i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      dzl_dock_bin_destroy_child_handle (self, child);
    }

  GTK_WIDGET_CLASS (dzl_dock_bin_parent_class)->unrealize (widget);
}

static void
dzl_dock_bin_map (GtkWidget *widget)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  GTK_WIDGET_CLASS (dzl_dock_bin_parent_class)->map (widget);

  for (i = 0; i < DZL_DOCK_BIN_CHILD_CENTER; i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      if (child->handle != NULL)
        gdk_window_show (child->handle);
    }
}

static void
dzl_dock_bin_unmap (GtkWidget *widget)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  for (i = 0; i < DZL_DOCK_BIN_CHILD_CENTER; i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      if (child->handle != NULL)
        gdk_window_hide (child->handle);
    }

  GTK_WIDGET_CLASS (dzl_dock_bin_parent_class)->unmap (widget);
}

static void
dzl_dock_bin_pan_gesture_drag_begin (DzlDockBin    *self,
                                     gdouble        x,
                                     gdouble        y,
                                     GtkGesturePan *gesture)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GdkEventSequence *sequence;
  DzlDockBinChild *child = NULL;
  GtkAllocation child_alloc;
  const GdkEvent *event;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_GESTURE_PAN (gesture));

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  event = gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);

  for (i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      if (priv->children [i].handle == event->any.window)
        {
          child = &priv->children [i];
          break;
        }
    }

  if (child == NULL || child->type >= DZL_DOCK_BIN_CHILD_CENTER)
    {
      gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  gtk_widget_get_allocation (child->widget, &child_alloc);

  priv->drag_child = child;
  priv->drag_child->drag_offset = 0;

  if (child->type == DZL_DOCK_BIN_CHILD_LEFT || child->type == DZL_DOCK_BIN_CHILD_RIGHT)
    {
      gtk_gesture_pan_set_orientation (gesture, GTK_ORIENTATION_HORIZONTAL);
      priv->drag_child->drag_begin_position = child_alloc.width;
    }
  else
    {
      gtk_gesture_pan_set_orientation (gesture, GTK_ORIENTATION_VERTICAL);
      priv->drag_child->drag_begin_position = child_alloc.height;
    }

  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
dzl_dock_bin_pan_gesture_drag_end (DzlDockBin    *self,
                                   gdouble        x,
                                   gdouble        y,
                                   GtkGesturePan *gesture)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GdkEventSequence *sequence;
  GtkEventSequenceState state;
  GtkAllocation child_alloc;
  gint position;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_GESTURE_PAN (gesture));

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  state = gtk_gesture_get_sequence_state (GTK_GESTURE (gesture), sequence);

  if (state == GTK_EVENT_SEQUENCE_DENIED)
    goto cleanup;

  g_warn_if_fail (priv->drag_child != NULL);
  g_warn_if_fail (DZL_IS_DOCK_BIN_EDGE (priv->drag_child->widget));

  gtk_widget_get_allocation (priv->drag_child->widget, &child_alloc);

  if ((priv->drag_child->type == DZL_DOCK_BIN_CHILD_LEFT) ||
      (priv->drag_child->type == DZL_DOCK_BIN_CHILD_RIGHT))
    position = child_alloc.width;
  else
    position = child_alloc.height;

  dzl_dock_revealer_set_position (DZL_DOCK_REVEALER (priv->drag_child->widget), position);

cleanup:
  if (priv->drag_child != NULL)
    {
      priv->drag_child->drag_offset = 0;
      priv->drag_child->drag_begin_position = 0;
      priv->drag_child = NULL;
    }
}

static void
dzl_dock_bin_pan_gesture_pan (DzlDockBin      *self,
                              GtkPanDirection  direction,
                              gdouble          offset,
                              GtkGesturePan   *gesture)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  gint position;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_GESTURE_PAN (gesture));
  g_warn_if_fail (priv->drag_child != NULL);
  g_warn_if_fail (priv->drag_child->type < DZL_DOCK_BIN_CHILD_CENTER);

  /*
   * This callback is used to adjust the size allocation of the edge in
   * question (denoted by priv->drag_child). It is always one of the
   * left, right, top, or bottom children. Never the center child.
   *
   * Because of how GtkRevealer works, we are doing a bit of a workaround
   * here. We need the revealer (the DzlDockBinEdge) child to have a size
   * request that matches the visible area, otherwise animating out the
   * revealer will not look right.
   */

  if (direction == GTK_PAN_DIRECTION_UP)
    {
      if (priv->drag_child->type == DZL_DOCK_BIN_CHILD_TOP)
        offset = -offset;
    }
  else if (direction == GTK_PAN_DIRECTION_DOWN)
    {
      if (priv->drag_child->type == DZL_DOCK_BIN_CHILD_BOTTOM)
        offset = -offset;
    }
  else if (direction == GTK_PAN_DIRECTION_LEFT)
    {
      if (priv->drag_child->type == DZL_DOCK_BIN_CHILD_LEFT)
        offset = -offset;
    }
  else if (direction == GTK_PAN_DIRECTION_RIGHT)
    {
      if (priv->drag_child->type == DZL_DOCK_BIN_CHILD_RIGHT)
        offset = -offset;
    }

  priv->drag_child->drag_offset = (gint)offset;

  position = priv->drag_child->drag_offset + priv->drag_child->drag_begin_position;
  if (position >= 0)
    dzl_dock_revealer_set_position (DZL_DOCK_REVEALER (priv->drag_child->widget), position);
}

static void
dzl_dock_bin_create_pan_gesture (DzlDockBin *self)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GtkGesture *gesture;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (priv->pan_gesture == NULL);

  gesture = gtk_gesture_pan_new (GTK_WIDGET (self), GTK_ORIENTATION_HORIZONTAL);
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture), FALSE);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);

  g_signal_connect_object (gesture,
                           "drag-begin",
                           G_CALLBACK (dzl_dock_bin_pan_gesture_drag_begin),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (gesture,
                           "drag-end",
                           G_CALLBACK (dzl_dock_bin_pan_gesture_drag_end),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (gesture,
                           "pan",
                           G_CALLBACK (dzl_dock_bin_pan_gesture_pan),
                           self,
                           G_CONNECT_SWAPPED);

  priv->pan_gesture = GTK_GESTURE_PAN (gesture);
}

static void
dzl_dock_bin_drag_enter (DzlDockBin     *self,
                         GdkDragContext *drag_context,
                         gint            x,
                         gint            y,
                         guint           time_)
{
  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GDK_IS_DRAG_CONTEXT (drag_context));

}

static gboolean
dzl_dock_bin_drag_motion (GtkWidget      *widget,
                          GdkDragContext *drag_context,
                          gint            x,
                          gint            y,
                          guint           time_)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GDK_IS_DRAG_CONTEXT (drag_context));

  /*
   * The purpose of this function is to determine of the location for which
   * the drag is currently located, is a valid drop site. We first calculate
   * the locations for the various zones, and then simply determine which
   * zone we are in (or none).
   */

  if (priv->dnd_drag_x == -1 && priv->dnd_drag_y == -1)
    dzl_dock_bin_drag_enter (self, drag_context, x, y, time_);

  priv->dnd_drag_x = x;
  priv->dnd_drag_y = y;

  gtk_widget_queue_draw (GTK_WIDGET (self));

  return TRUE;
}

static void
dzl_dock_bin_drag_leave (GtkWidget      *widget,
                         GdkDragContext *context,
                         guint           time_)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GDK_IS_DRAG_CONTEXT (context));

  priv->dnd_drag_x = -1;
  priv->dnd_drag_y = -1;
}

static void
dzl_dock_bin_grab_focus (GtkWidget *widget)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *child;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_CENTER);

  if (child->widget != NULL)
    {
      if (gtk_widget_child_focus (child->widget, GTK_DIR_TAB_FORWARD))
        return;
    }

  for (i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      child = &priv->children [i];

      if (DZL_IS_DOCK_REVEALER (child->widget) &&
          gtk_widget_get_visible (child->widget) &&
          gtk_widget_get_child_visible (child->widget) &&
          dzl_dock_revealer_get_reveal_child (DZL_DOCK_REVEALER (child->widget)))
        {
          if (gtk_widget_child_focus (child->widget, GTK_DIR_TAB_FORWARD))
            return;
        }
    }
}

static GtkWidget *
dzl_dock_bin_real_create_edge (DzlDockBin      *self,
                               GtkPositionType  edge)
{
  g_warn_if_fail (DZL_IS_DOCK_BIN (self));

  return g_object_new (DZL_TYPE_DOCK_BIN_EDGE,
                       "edge", edge,
                       "visible", TRUE,
                       "reveal-child", FALSE,
                       NULL);
}

static void
dzl_dock_bin_create_edge (DzlDockBin          *self,
                          DzlDockBinChild     *child,
                          DzlDockBinChildType  type)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  g_autoptr(GSimpleActionGroup) map = NULL;
  g_autoptr(GAction) pinned = NULL;
  g_autoptr(GPropertyAction) visible = NULL;
  const gchar *visible_name;
  const gchar *pinned_name;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (child != NULL);
  g_warn_if_fail (type >= DZL_DOCK_BIN_CHILD_LEFT);
  g_warn_if_fail (type < DZL_DOCK_BIN_CHILD_CENTER);

  child->widget = DZL_DOCK_BIN_GET_CLASS (self)->create_edge (self, (GtkPositionType)type);

  if (child->widget == NULL)
    {
      g_warning ("%s failed to create edge widget",
                 G_OBJECT_TYPE_NAME (self));
      return;
    }
  else if (!DZL_IS_DOCK_BIN_EDGE (child->widget))
    {
      g_warning ("%s child %s is not a DzlDockBinEdge",
                 G_OBJECT_TYPE_NAME (self),
                 G_OBJECT_TYPE_NAME (child));
      return;
    }

  g_object_set (child->widget,
                "edge", (GtkPositionType)type,
                "reveal-child", FALSE,
                NULL);

  g_signal_connect (child->widget,
                    "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &child->widget);

  g_signal_connect_object (child->widget,
                           "notify::reveal-child",
                           G_CALLBACK (dzl_dock_bin_notify_reveal_child),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (child->widget,
                           "notify::child-revealed",
                           G_CALLBACK (dzl_dock_bin_notify_child_revealed),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_widget_set_parent (g_object_ref_sink (child->widget), GTK_WIDGET (self));

  dzl_dock_item_adopt (DZL_DOCK_ITEM (self), DZL_DOCK_ITEM (child->widget));

  /* Action for panel children to easily activate */
  map = g_simple_action_group_new ();
  pinned = dzl_child_property_action_new ("pinned", GTK_CONTAINER (self), child->widget, "pinned");
  g_action_map_add_action (G_ACTION_MAP (map), pinned);
  gtk_widget_insert_action_group (child->widget, "panel", G_ACTION_GROUP (map));
  g_clear_object (&pinned);

  visible_name = visible_names [child->type];
  pinned_name = pinned_names [child->type];

  /* Add our pinned action */
  pinned = dzl_child_property_action_new (pinned_name,
                                          GTK_CONTAINER (self),
                                          child->widget,
                                          "pinned");
  g_action_map_add_action (G_ACTION_MAP (priv->actions), pinned);

  /* Add our visible action */
  visible = g_property_action_new (visible_name, self, visible_name);
  g_action_map_add_action (G_ACTION_MAP (priv->actions), G_ACTION (visible));

  if (child->pinned)
    gtk_style_context_add_class (gtk_widget_get_style_context (child->widget),
                                 DZL_DOCK_BIN_STYLE_CLASS_PINNED);

  g_object_notify (G_OBJECT (self), visible_name);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
dzl_dock_bin_init_child (DzlDockBin          *self,
                         DzlDockBinChild     *child,
                         DzlDockBinChildType  type)
{
  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (child != NULL);
  g_warn_if_fail (type >= DZL_DOCK_BIN_CHILD_LEFT);
  g_warn_if_fail (type < LAST_DZL_DOCK_BIN_CHILD);

  child->type = type;
  child->priority = (int)type * 100;
  child->pinned = TRUE;
  child->pre_anim_pinned = TRUE;
}

static gboolean
dzl_dock_bin_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  DzlDockBin *self = (DzlDockBin *)widget;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  const guint draw_order[] = { DZL_DOCK_BIN_CHILD_CENTER,
                               DZL_DOCK_BIN_CHILD_LEFT,
                               DZL_DOCK_BIN_CHILD_RIGHT,
                               DZL_DOCK_BIN_CHILD_TOP,
                               DZL_DOCK_BIN_CHILD_BOTTOM };

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (cr != NULL);

  /* All pinned children, in proper draw order */
  for (guint i = 0; i < G_N_ELEMENTS (draw_order); i++)
    {
      const DzlDockBinChild *child = &priv->children[draw_order[i]];

      if (!child->pinned ||
          !GTK_IS_WIDGET (child->widget) ||
          !gtk_widget_get_visible (child->widget) ||
          !gtk_widget_get_child_visible (child->widget))
        continue;

      gtk_container_propagate_draw (GTK_CONTAINER (self), child->widget, cr);
    }

  /* All unpinned children, in proper draw order */
  for (guint i = 1; i < G_N_ELEMENTS (draw_order); i++)
    {
      const DzlDockBinChild *child = &priv->children[draw_order[i]];

      if (child->pinned ||
          !GTK_IS_WIDGET (child->widget) ||
          !gtk_widget_get_visible (child->widget) ||
          !gtk_widget_get_child_visible (child->widget))
        continue;

      gtk_container_propagate_draw (GTK_CONTAINER (self), child->widget, cr);
    }

  return FALSE;
}

static void
dzl_dock_bin_destroy (GtkWidget *widget)
{
  DzlDockBin *self = DZL_DOCK_BIN (widget);
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_clear_object (&priv->actions);
  g_clear_object (&priv->pan_gesture);

  GTK_WIDGET_CLASS (dzl_dock_bin_parent_class)->destroy (widget);
}

static void
dzl_dock_bin_get_child_property (GtkContainer *container,
                                 GtkWidget    *widget,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  DzlDockBin *self = DZL_DOCK_BIN (container);
  DzlDockBinChild *child = dzl_dock_bin_get_child (self, widget);

  switch (prop_id)
    {
    case CHILD_PROP_PRIORITY:
      g_value_set_int (value, child->priority);
      break;

    case CHILD_PROP_POSITION:
      g_value_set_enum (value, (GtkPositionType)child->type);
      break;

    case CHILD_PROP_PINNED:
      g_value_set_boolean (value, child->pinned);
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
    }
}

static void
dzl_dock_bin_set_child_property (GtkContainer *container,
                                 GtkWidget    *widget,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  DzlDockBin *self = DZL_DOCK_BIN (container);

  switch (prop_id)
    {
    case CHILD_PROP_PINNED:
      dzl_dock_bin_set_child_pinned (self, widget, g_value_get_boolean (value));
      break;

    case CHILD_PROP_PRIORITY:
      dzl_dock_bin_set_child_priority (self, widget, g_value_get_int (value));
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
    }
}

static void
dzl_dock_bin_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DzlDockBin *self = DZL_DOCK_BIN (object);

  switch (prop_id)
    {
    case PROP_LEFT_VISIBLE:
      g_value_set_boolean (value, get_visible (self, DZL_DOCK_BIN_CHILD_LEFT));
      break;

    case PROP_RIGHT_VISIBLE:
      g_value_set_boolean (value, get_visible (self, DZL_DOCK_BIN_CHILD_RIGHT));
      break;

    case PROP_TOP_VISIBLE:
      g_value_set_boolean (value, get_visible (self, DZL_DOCK_BIN_CHILD_TOP));
      break;

    case PROP_BOTTOM_VISIBLE:
      g_value_set_boolean (value, get_visible (self, DZL_DOCK_BIN_CHILD_BOTTOM));
      break;

    case PROP_MANAGER:
      g_value_set_object (value, dzl_dock_item_get_manager (DZL_DOCK_ITEM (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_bin_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DzlDockBin *self = DZL_DOCK_BIN (object);

  switch (prop_id)
    {
    case PROP_LEFT_VISIBLE:
      set_visible (self, DZL_DOCK_BIN_CHILD_LEFT, g_value_get_boolean (value));
      break;

    case PROP_RIGHT_VISIBLE:
      set_visible (self, DZL_DOCK_BIN_CHILD_RIGHT, g_value_get_boolean (value));
      break;

    case PROP_TOP_VISIBLE:
      set_visible (self, DZL_DOCK_BIN_CHILD_TOP, g_value_get_boolean (value));
      break;

    case PROP_BOTTOM_VISIBLE:
      set_visible (self, DZL_DOCK_BIN_CHILD_BOTTOM, g_value_get_boolean (value));
      break;

    case PROP_MANAGER:
      dzl_dock_item_set_manager (DZL_DOCK_ITEM (self), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_bin_class_init (DzlDockBinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = dzl_dock_bin_get_property;
  object_class->set_property = dzl_dock_bin_set_property;

  widget_class->destroy = dzl_dock_bin_destroy;
  widget_class->drag_leave = dzl_dock_bin_drag_leave;
  widget_class->drag_motion = dzl_dock_bin_drag_motion;
  widget_class->draw = dzl_dock_bin_draw;
  widget_class->focus = dzl_dock_bin_focus;
  widget_class->get_preferred_height = dzl_dock_bin_get_preferred_height;
  widget_class->get_preferred_width = dzl_dock_bin_get_preferred_width;
  widget_class->grab_focus = dzl_dock_bin_grab_focus;
  widget_class->map = dzl_dock_bin_map;
  widget_class->realize = dzl_dock_bin_realize;
  widget_class->size_allocate = dzl_dock_bin_size_allocate;
  widget_class->unmap = dzl_dock_bin_unmap;
  widget_class->unrealize = dzl_dock_bin_unrealize;

  container_class->add = dzl_dock_bin_add;
  container_class->forall = dzl_dock_bin_forall;
  container_class->get_child_property = dzl_dock_bin_get_child_property;
  container_class->remove = dzl_dock_bin_remove;
  container_class->set_child_property = dzl_dock_bin_set_child_property;

  klass->create_edge = dzl_dock_bin_real_create_edge;

  g_object_class_override_property (object_class, PROP_MANAGER, "manager");

  properties [PROP_LEFT_VISIBLE] =
    g_param_spec_boolean ("left-visible",
                          "Left Visible",
                          "If the left panel is visible.",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_RIGHT_VISIBLE] =
    g_param_spec_boolean ("right-visible",
                          "Right Visible",
                          "If the right panel is visible.",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_TOP_VISIBLE] =
    g_param_spec_boolean ("top-visible",
                          "Top Visible",
                          "If the top panel is visible.",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_BOTTOM_VISIBLE] =
    g_param_spec_boolean ("bottom-visible",
                          "Bottom Visible",
                          "If the bottom panel is visible.",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  child_properties [CHILD_PROP_PINNED] =
    g_param_spec_boolean ("pinned",
                          "Pinned",
                          "If the child panel is pinned",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  child_properties [CHILD_PROP_POSITION] =
    g_param_spec_enum ("position",
                       "Position",
                       "The position of the dock edge",
                       GTK_TYPE_POSITION_TYPE,
                       GTK_POS_LEFT,
                       (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  child_properties [CHILD_PROP_PRIORITY] =
    g_param_spec_int ("priority",
                      "Priority",
                      "The priority of the dock edge",
                      G_MININT,
                      G_MAXINT,
                      0,
                      (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gtk_container_class_install_child_properties (container_class, N_CHILD_PROPS, child_properties);

  gtk_widget_class_set_css_name (widget_class, "dzldockbin");
}

static void
dzl_dock_bin_init (DzlDockBin *self)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  static GtkTargetEntry drag_entries[] = {
    { (gchar *)"DZL_DOCK_BIN_WIDGET", GTK_TARGET_SAME_APP, 0 },
  };

  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);

  dzl_dock_bin_create_pan_gesture (self);

  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     drag_entries,
                     G_N_ELEMENTS (drag_entries),
                     GDK_ACTION_MOVE);

  priv->dnd_drag_x = -1;
  priv->dnd_drag_y = -1;

  dzl_dock_bin_init_child (self, &priv->children [0], DZL_DOCK_BIN_CHILD_LEFT);
  dzl_dock_bin_init_child (self, &priv->children [1], DZL_DOCK_BIN_CHILD_RIGHT);
  dzl_dock_bin_init_child (self, &priv->children [2], DZL_DOCK_BIN_CHILD_BOTTOM);
  dzl_dock_bin_init_child (self, &priv->children [3], DZL_DOCK_BIN_CHILD_TOP);
  dzl_dock_bin_init_child (self, &priv->children [4], DZL_DOCK_BIN_CHILD_CENTER);

  priv->actions = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (self), "dockbin", G_ACTION_GROUP (priv->actions));
}

GtkWidget *
dzl_dock_bin_new (void)
{
  return g_object_new (DZL_TYPE_DOCK_BIN, NULL);
}

/**
 * dzl_dock_bin_get_center_widget:
 * @self: A #DzlDockBin
 *
 * Gets the center widget for the dock.
 *
 * Returns: (transfer none) (nullable): A #GtkWidget or %NULL.
 */
GtkWidget *
dzl_dock_bin_get_center_widget (DzlDockBin *self)
{
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  DzlDockBinChild *child;

  g_return_val_if_fail (DZL_IS_DOCK_BIN (self), NULL);

  child = &priv->children [DZL_DOCK_BIN_CHILD_CENTER];

  return child->widget;
}

/**
 * dzl_dock_bin_get_top_edge:
 * Returns: (transfer none): A #GtkWidget
 */
GtkWidget *
dzl_dock_bin_get_top_edge (DzlDockBin *self)
{
  DzlDockBinChild *child;

  g_return_val_if_fail (DZL_IS_DOCK_BIN (self), NULL);

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_TOP);

  if (child->widget == NULL)
    dzl_dock_bin_create_edge (self, child, DZL_DOCK_BIN_CHILD_TOP);

  return child->widget;
}

/**
 * dzl_dock_bin_get_left_edge:
 * Returns: (transfer none): A #GtkWidget
 */
GtkWidget *
dzl_dock_bin_get_left_edge (DzlDockBin *self)
{
  DzlDockBinChild *child;

  g_return_val_if_fail (DZL_IS_DOCK_BIN (self), NULL);

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_LEFT);

  if (child->widget == NULL)
    dzl_dock_bin_create_edge (self, child, DZL_DOCK_BIN_CHILD_LEFT);

  return child->widget;
}

/**
 * dzl_dock_bin_get_bottom_edge:
 * Returns: (transfer none): A #GtkWidget
 */
GtkWidget *
dzl_dock_bin_get_bottom_edge (DzlDockBin *self)
{
  DzlDockBinChild *child;

  g_return_val_if_fail (DZL_IS_DOCK_BIN (self), NULL);

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_BOTTOM);

  if (child->widget == NULL)
    dzl_dock_bin_create_edge (self, child, DZL_DOCK_BIN_CHILD_BOTTOM);

  return child->widget;
}

/**
 * dzl_dock_bin_get_right_edge:
 * Returns: (transfer none): A #GtkWidget
 */
GtkWidget *
dzl_dock_bin_get_right_edge (DzlDockBin *self)
{
  DzlDockBinChild *child;

  g_return_val_if_fail (DZL_IS_DOCK_BIN (self), NULL);

  child = dzl_dock_bin_get_child_typed (self, DZL_DOCK_BIN_CHILD_RIGHT);

  if (child->widget == NULL)
    dzl_dock_bin_create_edge (self, child, DZL_DOCK_BIN_CHILD_RIGHT);

  return child->widget;
}

static void
dzl_dock_bin_init_dock_iface (DzlDockInterface *iface)
{
}

static void
dzl_dock_bin_add_child (GtkBuildable *buildable,
                        GtkBuilder   *builder,
                        GObject      *child,
                        const gchar  *type)
{
  DzlDockBin *self = (DzlDockBin *)buildable;
  GtkWidget *parent;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_BUILDER (builder));
  g_warn_if_fail (G_IS_OBJECT (child));

  if (!GTK_IS_WIDGET (child))
    {
      g_warning ("Attempt to add a child of type \"%s\" to a \"%s\"",
                 G_OBJECT_TYPE_NAME (child), G_OBJECT_TYPE_NAME (self));
      return;
    }

  if (DZL_IS_DOCK_ITEM (child) &&
      !dzl_dock_item_adopt (DZL_DOCK_ITEM (self), DZL_DOCK_ITEM (child)))
    {
      g_warning ("Child of type %s has a different DzlDockManager than %s",
                 G_OBJECT_TYPE_NAME (child), G_OBJECT_TYPE_NAME (self));
      return;
    }

  if (!type || !*type || (g_strcmp0 ("center", type) == 0))
    {
      gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (child));
      return;
    }

  if (g_strcmp0 ("top", type) == 0)
    parent = dzl_dock_bin_get_top_edge (self);
  else if (g_strcmp0 ("bottom", type) == 0)
    parent = dzl_dock_bin_get_bottom_edge (self);
  else if (g_strcmp0 ("right", type) == 0)
    parent = dzl_dock_bin_get_right_edge (self);
  else
    parent = dzl_dock_bin_get_left_edge (self);

  gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (child));
}

static GObject *
dzl_dock_bin_get_internal_child (GtkBuildable *buildable,
                                 GtkBuilder   *builder,
                                 const gchar  *childname)
{
  DzlDockBin *self = (DzlDockBin *)buildable;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (GTK_IS_BUILDER (builder));

  if (g_strcmp0 ("top", childname) == 0)
    return G_OBJECT (dzl_dock_bin_get_top_edge (self));
  else if (g_strcmp0 ("bottom", childname) == 0)
    return G_OBJECT (dzl_dock_bin_get_bottom_edge (self));
  else if (g_strcmp0 ("right", childname) == 0)
    return G_OBJECT (dzl_dock_bin_get_right_edge (self));
  else if (g_strcmp0 ("left", childname) == 0)
    return G_OBJECT (dzl_dock_bin_get_left_edge (self));

  return NULL;
}

static void
dzl_dock_bin_init_buildable_iface (GtkBuildableIface *iface)
{
  iface->add_child = dzl_dock_bin_add_child;
  iface->get_internal_child = dzl_dock_bin_get_internal_child;
}

static void
dzl_dock_bin_present_child (DzlDockItem *item,
                            DzlDockItem *widget)
{
  DzlDockBin *self = (DzlDockBin *)item;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (widget));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  for (i = 0; i < G_N_ELEMENTS (priv->children); i++)
    {
      DzlDockBinChild *child = &priv->children [i];

      if (DZL_IS_DOCK_BIN_EDGE (child->widget) &&
          gtk_widget_is_ancestor (GTK_WIDGET (widget), child->widget))
        {
          set_visible (self, child->type, TRUE);
          return;
        }
    }
}

static gboolean
dzl_dock_bin_get_child_visible (DzlDockItem *item,
                                DzlDockItem *child)
{
  DzlDockBin *self = (DzlDockBin *)item;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);
  GtkWidget *ancestor;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (item));

  ancestor = gtk_widget_get_ancestor (GTK_WIDGET (child), DZL_TYPE_DOCK_BIN_EDGE);

  if (ancestor == NULL)
    return FALSE;

  if ((ancestor == priv->children [0].widget) ||
      (ancestor == priv->children [1].widget) ||
      (ancestor == priv->children [2].widget) ||
      (ancestor == priv->children [3].widget))
    return dzl_dock_revealer_get_child_revealed (DZL_DOCK_REVEALER (ancestor));

  return FALSE;
}

static void
dzl_dock_bin_set_child_visible (DzlDockItem *item,
                                DzlDockItem *child,
                                gboolean     child_visible)
{
  DzlDockBin *self = (DzlDockBin *)item;
  GtkWidget *ancestor;

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (item));

  ancestor = gtk_widget_get_ancestor (GTK_WIDGET (child), DZL_TYPE_DOCK_BIN_EDGE);

  if (ancestor != NULL)
    dzl_dock_revealer_set_reveal_child (DZL_DOCK_REVEALER (ancestor), child_visible);
}

static gboolean
dzl_dock_bin_minimize (DzlDockItem     *item,
                       DzlDockItem     *child,
                       GtkPositionType *position)
{
  DzlDockBin *self = (DzlDockBin *)item;
  DzlDockBinPrivate *priv = dzl_dock_bin_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_BIN (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));
  g_warn_if_fail (position != NULL);

  for (guint i = 0; i < LAST_DZL_DOCK_BIN_CHILD; i++)
    {
      const DzlDockBinChild *info = &priv->children [i];

      if (info->widget != NULL && gtk_widget_is_ancestor (GTK_WIDGET (child), info->widget))
        {
          switch (info->type)
            {
            case DZL_DOCK_BIN_CHILD_LEFT:
            case DZL_DOCK_BIN_CHILD_CENTER:
            case LAST_DZL_DOCK_BIN_CHILD:
            default:
              *position = GTK_POS_LEFT;
              break;

            case DZL_DOCK_BIN_CHILD_RIGHT:
              *position = GTK_POS_RIGHT;
              break;

            case DZL_DOCK_BIN_CHILD_TOP:
              *position = GTK_POS_TOP;
              break;

            case DZL_DOCK_BIN_CHILD_BOTTOM:
              *position = GTK_POS_BOTTOM;
              break;
            }

          break;
        }
    }

  return FALSE;
}

static void
dzl_dock_bin_init_dock_item_iface (DzlDockItemInterface *iface)
{
  iface->present_child = dzl_dock_bin_present_child;
  iface->get_child_visible = dzl_dock_bin_get_child_visible;
  iface->set_child_visible = dzl_dock_bin_set_child_visible;
  iface->minimize = dzl_dock_bin_minimize;
}
