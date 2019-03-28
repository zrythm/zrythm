/* dzl-dock-item.c
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

#include "gui/widgets/dzl/dzl-dock-item.h"
#include "gui/widgets/dzl/dzl-dock-manager.h"
#include "gui/widgets/dzl/dzl-dock-widget.h"

G_DEFINE_INTERFACE (DzlDockItem, dzl_dock_item, GTK_TYPE_WIDGET)

/*
 * The DzlDockItem is an interface that acts more like a mixin.
 * This is mostly out of wanting to preserve widget inheritance
 * without having to duplicate all sorts of plumbing.
 */

enum {
  MANAGER_SET,
  PRESENTED,
  N_SIGNALS
};

static guint signals [N_SIGNALS];

static void
dzl_dock_item_real_set_manager (DzlDockItem    *self,
                                DzlDockManager *manager)
{
  DzlDockManager *old_manager;

  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (!manager || DZL_IS_DOCK_MANAGER (manager));

  if (NULL != (old_manager = dzl_dock_item_get_manager (self)))
    {
      if (DZL_IS_DOCK (self))
        dzl_dock_manager_unregister_dock (old_manager, DZL_DOCK (self));
    }

  if (manager != NULL)
    {
      g_object_set_data_full (G_OBJECT (self),
                              "DZL_DOCK_MANAGER",
                              g_object_ref (manager),
                              g_object_unref);
      if (DZL_IS_DOCK (self))
        dzl_dock_manager_register_dock (manager, DZL_DOCK (self));
    }
  else
    g_object_set_data (G_OBJECT (self), "DZL_DOCK_MANAGER", NULL);

  g_signal_emit (self, signals [MANAGER_SET], 0, old_manager);
}

static DzlDockManager *
dzl_dock_item_real_get_manager (DzlDockItem *self)
{
  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));

  return g_object_get_data (G_OBJECT (self), "DZL_DOCK_MANAGER");
}

static void
dzl_dock_item_real_update_visibility (DzlDockItem *self)
{
}

static void
dzl_dock_item_propagate_manager (DzlDockItem *self)
{
  DzlDockManager *manager;
  GPtrArray *ar;
  guint i;

  g_return_if_fail (DZL_IS_DOCK_ITEM (self));

  if (!GTK_IS_CONTAINER (self))
    return;

  if (NULL == (manager = dzl_dock_item_get_manager (self)))
    return;

  if (NULL == (ar = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS")))
    return;

  for (i = 0; i < ar->len; i++)
    {
      DzlDockItem *item = g_ptr_array_index (ar, i);

      dzl_dock_item_set_manager (item, manager);
    }
}

static void
dzl_dock_item_real_manager_set (DzlDockItem    *self,
                                DzlDockManager *manager)
{
  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (!manager || DZL_IS_DOCK_MANAGER (manager));

  dzl_dock_item_propagate_manager (self);
}

static void
dzl_dock_item_real_release (DzlDockItem *self,
                            DzlDockItem *child)
{
  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  g_warning ("%s does not know how to release child %s",
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (child));
}

static gboolean
dzl_dock_item_real_can_minimize (DzlDockItem *self,
                                 DzlDockItem *descendant)
{
  return FALSE;
}

static gboolean
dzl_dock_item_real_get_can_close (DzlDockItem *self)
{
  return FALSE;
}

static void
dzl_dock_item_default_init (DzlDockItemInterface *iface)
{
  iface->get_manager = dzl_dock_item_real_get_manager;
  iface->set_manager = dzl_dock_item_real_set_manager;
  iface->manager_set = dzl_dock_item_real_manager_set;
  iface->update_visibility = dzl_dock_item_real_update_visibility;
  iface->release = dzl_dock_item_real_release;
  iface->get_can_close = dzl_dock_item_real_get_can_close;
  iface->can_minimize = dzl_dock_item_real_can_minimize;

  signals [MANAGER_SET] =
    g_signal_new ("manager-set",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlDockItemInterface, manager_set),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, DZL_TYPE_DOCK_MANAGER);
  g_signal_set_va_marshaller (signals [MANAGER_SET],
                              G_TYPE_FROM_INTERFACE (iface),
                              g_cclosure_marshal_VOID__OBJECTv);

  signals [PRESENTED] =
    g_signal_new ("presented",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlDockItemInterface, presented),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  g_signal_set_va_marshaller (signals [PRESENTED],
                              G_TYPE_FROM_INTERFACE (iface),
                              g_cclosure_marshal_VOID__VOIDv);
}

/**
 * dzl_dock_item_get_manager:
 * @self: A #DzlDockItem
 *
 * Gets the dock manager for this dock item.
 *
 * Returns: (nullable) (transfer none): A #DzlDockmanager.
 */
DzlDockManager *
dzl_dock_item_get_manager (DzlDockItem *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), NULL);

  return DZL_DOCK_ITEM_GET_IFACE (self)->get_manager (self);
}

/**
 * dzl_dock_item_set_manager:
 * @self: A #DzlDockItem
 * @manager: (nullable): A #DzlDockManager
 *
 * Sets the dock manager for this #DzlDockItem.
 */
void
dzl_dock_item_set_manager (DzlDockItem    *self,
                           DzlDockManager *manager)
{
  g_return_if_fail (DZL_IS_DOCK_ITEM (self));
  g_return_if_fail (!manager || DZL_IS_DOCK_MANAGER (manager));

  DZL_DOCK_ITEM_GET_IFACE (self)->set_manager (self, manager);
}

void
dzl_dock_item_update_visibility (DzlDockItem *self)
{
  GtkWidget *parent;

  g_return_if_fail (DZL_IS_DOCK_ITEM (self));

  DZL_DOCK_ITEM_GET_IFACE (self)->update_visibility (self);

  for (parent = gtk_widget_get_parent (GTK_WIDGET (self));
       parent != NULL;
       parent = gtk_widget_get_parent (parent))
    {
      if (DZL_IS_DOCK_ITEM (parent))
        DZL_DOCK_ITEM_GET_IFACE (parent)->update_visibility (DZL_DOCK_ITEM (parent));
    }
}

static void
dzl_dock_item_child_weak_notify (gpointer  data,
                                 GObject  *where_object_was)
{
  DzlDockItem *self = data;
  GPtrArray *descendants;

  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));

  descendants = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS");

  if (descendants != NULL)
    g_ptr_array_remove (descendants, where_object_was);

  dzl_dock_item_update_visibility (self);
}

static void
dzl_dock_item_destroy (DzlDockItem *self)
{
  GPtrArray *descendants;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));

  descendants = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS");

  if (descendants != NULL)
    {
      for (i = 0; i < descendants->len; i++)
        {
          DzlDockItem *child = g_ptr_array_index (descendants, i);

          g_object_weak_unref (G_OBJECT (child),
                               dzl_dock_item_child_weak_notify,
                               self);
        }

      g_object_set_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS", NULL);
      g_ptr_array_unref (descendants);
    }
}

static void
dzl_dock_item_track_child (DzlDockItem *self,
                           DzlDockItem *child)
{
  GPtrArray *descendants;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  descendants = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS");

  if (descendants == NULL)
    {
      descendants = g_ptr_array_new ();
      g_object_set_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS", descendants);
      g_signal_connect (self,
                        "destroy",
                        G_CALLBACK (dzl_dock_item_destroy),
                        NULL);
    }

  for (i = 0; i < descendants->len; i++)
    {
      DzlDockItem *item = g_ptr_array_index (descendants, i);

      if (item == child)
        return;
    }

  g_object_weak_ref (G_OBJECT (child),
                     dzl_dock_item_child_weak_notify,
                     self);

  g_ptr_array_add (descendants, child);

  dzl_dock_item_update_visibility (child);
}

gboolean
dzl_dock_item_adopt (DzlDockItem *self,
                     DzlDockItem *child)
{
  DzlDockManager *manager;
  DzlDockManager *child_manager;

  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (child), FALSE);

  manager = dzl_dock_item_get_manager (self);
  child_manager = dzl_dock_item_get_manager (child);

  if ((child_manager != NULL) && (manager != NULL) && (child_manager != manager))
    return FALSE;

  if (manager != NULL)
    dzl_dock_item_set_manager (child, manager);

  dzl_dock_item_track_child (self, child);

  return TRUE;
}

void
dzl_dock_item_present_child (DzlDockItem *self,
                             DzlDockItem *child)
{
  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

#if 0
  g_print ("present_child (%s, %s)\n",
           G_OBJECT_TYPE_NAME (self),
           G_OBJECT_TYPE_NAME (child));
#endif

  if (DZL_DOCK_ITEM_GET_IFACE (self)->present_child)
    DZL_DOCK_ITEM_GET_IFACE (self)->present_child (self, child);
}

/**
 * dzl_dock_item_present:
 * @self: A #DzlDockItem
 *
 * This widget will walk the widget hierarchy to ensure that the
 * dock item is visible to the user.
 */
void
dzl_dock_item_present (DzlDockItem *self)
{
  GtkWidget *parent;

  g_return_if_fail (DZL_IS_DOCK_ITEM (self));

  for (parent = gtk_widget_get_parent (GTK_WIDGET (self));
       parent != NULL;
       parent = gtk_widget_get_parent (parent))
    {
      if (DZL_IS_DOCK_ITEM (parent))
        {
          DzlDockManager *manager;

          dzl_dock_item_present_child (DZL_DOCK_ITEM (parent), self);
          dzl_dock_item_present (DZL_DOCK_ITEM (parent));

          /* gtk_widget_grab_focus() results in a transient grab,
           * we want a real grab (that doesn't release the parent)
           * when we are focused during this code path.
           */
          if ((manager = dzl_dock_item_get_manager (self)))
            dzl_dock_manager_release_transient_grab (manager);

          return;
        }
    }
}

gboolean
dzl_dock_item_has_widgets (DzlDockItem *self)
{
  GPtrArray *ar;

  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);

  if (DZL_IS_DOCK_WIDGET (self))
    return TRUE;

  ar = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS");

  if (ar != NULL)
    {
      guint i;

      for (i = 0; i < ar->len; i++)
        {
          DzlDockItem *child = g_ptr_array_index (ar, i);

          if (dzl_dock_item_has_widgets (child))
            return TRUE;
        }
    }

  return FALSE;
}

static void
dzl_dock_item_printf_internal (DzlDockItem *self,
                               GString     *str,
                               guint        depth)
{
  GPtrArray *ar;
  guint i;

  g_warn_if_fail (DZL_IS_DOCK_ITEM (self));
  g_warn_if_fail (str != NULL);


  for (i = 0; i < depth; i++)
    g_string_append_c (str, ' ');

  g_string_append_printf (str, "%s\n", G_OBJECT_TYPE_NAME (self));

  ++depth;

  ar = g_object_get_data (G_OBJECT (self), "DZL_DOCK_ITEM_DESCENDANTS");

  if (ar != NULL)
    {
      for (i = 0; i < ar->len; i++)
        dzl_dock_item_printf_internal (g_ptr_array_index (ar, i), str, depth);
    }
}

void
_dzl_dock_item_printf (DzlDockItem *self)
{
  GString *str;

  g_return_if_fail (DZL_IS_DOCK_ITEM (self));

  str = g_string_new (NULL);
  dzl_dock_item_printf_internal (self, str, 0);
  g_printerr ("%s", str->str);
  g_string_free (str, TRUE);
}

/**
 * dzl_dock_item_get_parent:
 *
 * Gets the parent #DzlDockItem, or %NULL.
 *
 * Returns: (transfer none) (nullable): A #DzlDockItem or %NULL.
 */
DzlDockItem *
dzl_dock_item_get_parent (DzlDockItem *self)
{
  GtkWidget *parent;

  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), NULL);

  for (parent = gtk_widget_get_parent (GTK_WIDGET (self));
       parent != NULL;
       parent = gtk_widget_get_parent (parent))
    {
      if (DZL_IS_DOCK_ITEM (parent))
        return DZL_DOCK_ITEM (parent);
    }

  return NULL;
}

gboolean
dzl_dock_item_get_child_visible (DzlDockItem *self,
                                 DzlDockItem *child)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (child), FALSE);

  if (DZL_DOCK_ITEM_GET_IFACE (self)->get_child_visible)
    return DZL_DOCK_ITEM_GET_IFACE (self)->get_child_visible (self, child);

  return TRUE;
}

void
dzl_dock_item_set_child_visible (DzlDockItem *self,
                                 DzlDockItem *child,
                                 gboolean     child_visible)
{
  g_return_if_fail (DZL_IS_DOCK_ITEM (self));
  g_return_if_fail (DZL_IS_DOCK_ITEM (child));

  if (DZL_DOCK_ITEM_GET_IFACE (self)->set_child_visible)
    DZL_DOCK_ITEM_GET_IFACE (self)->set_child_visible (self, child, child_visible);
}

/**
 * dzl_dock_item_get_can_close:
 * @self: a #DzlDockItem
 *
 * If this dock item can be closed by the user, this virtual function should be
 * implemented by the panel and return %TRUE.
 *
 * Returns: %TRUE if the dock item can be closed by the user, otherwise %FALSE.
 */
gboolean
dzl_dock_item_get_can_close (DzlDockItem *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);

  if (DZL_DOCK_ITEM_GET_IFACE (self)->get_can_close)
    return DZL_DOCK_ITEM_GET_IFACE (self)->get_can_close (self);

  return FALSE;
}

/**
 * dzl_dock_item_close:
 * @self: a #DzlDockItem
 *
 * This function will request that the dock item close itself.
 *
 * Returns: %TRUE if the dock item was closed
 */
gboolean
dzl_dock_item_close (DzlDockItem *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);

  if (dzl_dock_item_get_can_close (self))
    {
      if (DZL_DOCK_ITEM_GET_IFACE (self)->close)
        return DZL_DOCK_ITEM_GET_IFACE (self)->close (self);

      gtk_widget_destroy (GTK_WIDGET (self));

      return TRUE;
    }

  return FALSE;
}

/**
 * dzl_dock_item_minimize:
 * @self: a #DzlDockItem
 * @child: A #DzlDockItem that is a child of @self
 * @position: (inout): A location for a #GtkPositionType
 *
 * This requests that @self minimize @child if it knows how.
 *
 * If not, it should suggest the gravity for @child if it knows how to
 * determine that. For example, a #DzlDockBin might know if the widget was part
 * of the right panel and therefore may set @position to %GTK_POS_RIGHT.
 *
 * Returns: %TRUE if @child was minimized. Otherwise %FALSE and @position
 *   may be updated to a suggested position.
 */
gboolean
dzl_dock_item_minimize (DzlDockItem     *self,
                        DzlDockItem     *child,
                        GtkPositionType *position)
{
  GtkPositionType unused = GTK_POS_LEFT;

  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (child), FALSE);
  g_return_val_if_fail (self != child, FALSE);

  if (position == NULL)
    position = &unused;

  if (DZL_DOCK_ITEM_GET_IFACE (self)->minimize)
    return DZL_DOCK_ITEM_GET_IFACE (self)->minimize (self, child, position);

  return FALSE;
}

/**
 * dzl_dock_item_get_title:
 * @self: A #DzlDockItem
 *
 * Gets the title for the #DzlDockItem.
 *
 * Generally, you want to use a #DzlDockWidget which has a "title" property you
 * can set. But this can be helpful for integration of various container
 * widgets.
 *
 * Returns: (transfer full) (nullable): A newly allocated string or %NULL.
 */
gchar *
dzl_dock_item_get_title (DzlDockItem *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), NULL);

  if (DZL_DOCK_ITEM_GET_IFACE (self)->get_title)
    return DZL_DOCK_ITEM_GET_IFACE (self)->get_title (self);

  return NULL;
}

/**
 * dzl_dock_item_get_icon_name:
 * @self: A #DzlDockItem
 *
 * Gets the icon_name for the #DzlDockItem.
 *
 * Generally, you want to use a #DzlDockWidget which has a "icon-name" property
 * you can set. But this can be helpful for integration of various container
 * widgets.
 *
 * Returns: (transfer full) (nullable): A newly allocated string or %NULL.
 */
gchar *
dzl_dock_item_get_icon_name (DzlDockItem *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), NULL);

  if (DZL_DOCK_ITEM_GET_IFACE (self)->get_icon_name)
    return DZL_DOCK_ITEM_GET_IFACE (self)->get_icon_name (self);

  return NULL;
}

/**
 * dzl_dock_item_release:
 * @self: A #DzlDockItem
 *
 * This virtual method should remove @child from @self if the
 * dock item knows how to do so. For example, the #DzlDockStack
 * will remove @child from it's internal #GtkStack.
 *
 * After the virtual function has been executed, child tracking
 * will be removed so that #DzlDockItem implementations do not
 * need to implement themselves.
 */
void
dzl_dock_item_release (DzlDockItem     *self,
                       DzlDockItem     *child)
{
  g_return_if_fail (DZL_IS_DOCK_ITEM (self));
  g_return_if_fail (self == dzl_dock_item_get_parent (child));

  DZL_DOCK_ITEM_GET_IFACE (self)->release (self, child);

  g_object_weak_unref (G_OBJECT (child),
                       dzl_dock_item_child_weak_notify,
                       self);
  dzl_dock_item_child_weak_notify (self, (GObject *)child);
}

/**
 * dzl_dock_item_get_can_minimize: (virtual can_minimize)
 * @self: a #DzlDockItem
 *
 * Returns: %TRUE if the widget can be minimized.
 */
gboolean
dzl_dock_item_get_can_minimize (DzlDockItem *self)
{
  DzlDockItem *parent;

  g_return_val_if_fail (DZL_IS_DOCK_ITEM (self), FALSE);

  parent = dzl_dock_item_get_parent (self);

  while (parent != NULL)
    {
      if (DZL_DOCK_ITEM_GET_IFACE (parent)->can_minimize (parent, self))
        return TRUE;
      parent = dzl_dock_item_get_parent (parent);
    }

  return FALSE;
}

/**
 * dzl_dock_item_emit_presented:
 * @self: a #DzlDockItem
 *
 * Emits the #DzlDockItem::presented signal.
 *
 * Containers should emit this when their descendant has been presented as the
 * current visible child. This allows dock items to do lazy initialization of
 * content once the widgetry is visible to the user.
 *
 * Currently, this is best effort, as there are a number of situations that
 * make covering all cases problematic.
 *
 * Since: 3.30
 */
void
dzl_dock_item_emit_presented (DzlDockItem *self)
{
  g_return_if_fail (DZL_IS_DOCK_ITEM (self));

  g_signal_emit (self, signals [PRESENTED], 0);
}
