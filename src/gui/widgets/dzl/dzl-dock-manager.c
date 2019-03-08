/* dzl-dock-manager.c
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

#include "gui/widgets/dzl/dzl-dock-manager.h"
#include "gui/widgets/dzl/dzl-dock-transient-grab.h"
#include "gui/widgets/dzl/dzl-util-private.h"

typedef struct
{
  GPtrArray            *docks;
  DzlDockTransientGrab *grab;
  GHashTable           *queued_focus_by_toplevel;
  guint                 queued_handler;
  gint                  pause_count;
} DzlDockManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DzlDockManager, dzl_dock_manager, G_TYPE_OBJECT)

enum {
  REGISTER_DOCK,
  UNREGISTER_DOCK,
  N_SIGNALS
};

static guint signals [N_SIGNALS];

static void
dzl_dock_manager_do_set_focus (DzlDockManager *self,
                               GtkWidget      *focus,
                               GtkWidget      *toplevel)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);
  g_autoptr(DzlDockTransientGrab) grab = NULL;
  GtkWidget *parent;

  g_assert (DZL_IS_DOCK_MANAGER (self));
  g_assert (GTK_IS_WIDGET (focus));
  g_assert (GTK_IS_WIDGET (toplevel));

  if (priv->pause_count > 0)
    return;

  if (priv->grab != NULL)
    {
      /*
       * If the current transient grab contains the new focus widget,
       * then there is nothing for us to do now.
       */
      if (dzl_dock_transient_grab_is_descendant (priv->grab, focus))
        return;
    }

  /*
   * If their is a DzlDockItem in the hierarchy, create a new transient grab.
   */
  parent = focus;
  while (GTK_IS_WIDGET (parent))
    {
      if (DZL_IS_DOCK_ITEM (parent))
        {
          /* If we reach a DockItem that doesn't have a manager set,
           * then we are probably adding the widgetry to the window
           * and grabing focus right now would be intrusive.
           */
          if (dzl_dock_item_get_manager (DZL_DOCK_ITEM (parent)) == NULL)
            return;

          if (grab == NULL)
            grab = dzl_dock_transient_grab_new ();

          dzl_dock_transient_grab_add_item (grab, DZL_DOCK_ITEM (parent));
        }

      if (GTK_IS_POPOVER (parent))
        parent = gtk_popover_get_relative_to (GTK_POPOVER (parent));
      else
        parent = gtk_widget_get_parent (parent);
    }

  /*
   * Steal common hierarchy so that we don't hide it when breaking grabs.
   * Then release our previous grab.
   */
  if (priv->grab != NULL)
    {
      if (grab != NULL)
        dzl_dock_transient_grab_steal_common_ancestors (grab, priv->grab);
      dzl_dock_transient_grab_release (priv->grab);
      g_clear_object (&priv->grab);
    }

  g_assert (priv->grab == NULL);

  /* Start the grab process */
  if (grab != NULL)
    {
      priv->grab = g_steal_pointer (&grab);
      dzl_dock_transient_grab_acquire (priv->grab);
    }
}

static gboolean
do_delayed_focus_update (gpointer user_data)
{
  DzlDockManager *self = user_data;
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);
  g_autoptr(GHashTable) hashtable = NULL;
  GHashTableIter iter;
  GtkWidget *toplevel;
  GtkWidget *focus;

  g_assert (DZL_IS_DOCK_MANAGER (self));

  priv->queued_handler = 0;

  hashtable = g_steal_pointer (&priv->queued_focus_by_toplevel);
  g_hash_table_iter_init (&iter, hashtable);
  while (g_hash_table_iter_next (&iter, (gpointer *)&toplevel, (gpointer *)&focus))
    dzl_dock_manager_do_set_focus (self, focus, toplevel);

  return G_SOURCE_REMOVE;
}

static void
dzl_dock_manager_set_focus (DzlDockManager *self,
                            GtkWidget      *focus,
                            GtkWidget      *toplevel)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_assert (DZL_IS_DOCK_MANAGER (self));
  g_assert (GTK_IS_WINDOW (toplevel));

  if (priv->queued_focus_by_toplevel == NULL)
    priv->queued_focus_by_toplevel = g_hash_table_new (NULL, NULL);

  /*
   * Don't do anything if we get a NULL focus. Instead, wait for the focus
   * to be updated with a widget.
   */
  if (focus == NULL)
    {
      g_hash_table_remove (priv->queued_focus_by_toplevel, toplevel);
      return;
    }

  /*
   * If focus is changing, we want to delay this until the end of the main
   * loop cycle so that we don't do too much work when rapidly adding widgets
   * to the hierarchy, as they may implicitly grab focus.
   */
  g_hash_table_insert (priv->queued_focus_by_toplevel, toplevel, focus);
  dzl_clear_source (&priv->queued_handler);
  priv->queued_handler = gdk_threads_add_idle (do_delayed_focus_update, self);
}

static void
dzl_dock_manager_hierarchy_changed (DzlDockManager *self,
                                    GtkWidget      *old_toplevel,
                                    GtkWidget      *widget)
{
  GtkWidget *toplevel;

  g_assert (DZL_IS_DOCK_MANAGER (self));
  g_assert (!old_toplevel || GTK_IS_WIDGET (old_toplevel));
  g_assert (GTK_IS_WIDGET (widget));

  if (GTK_IS_WINDOW (old_toplevel))
    g_signal_handlers_disconnect_by_func (old_toplevel,
                                          G_CALLBACK (dzl_dock_manager_set_focus),
                                          self);

  toplevel = gtk_widget_get_toplevel (widget);

  if (GTK_IS_WINDOW (toplevel))
    g_signal_connect_object (toplevel,
                             "set-focus",
                             G_CALLBACK (dzl_dock_manager_set_focus),
                             self,
                             G_CONNECT_SWAPPED);
}

static void
dzl_dock_manager_watch_toplevel (DzlDockManager *self,
                                 GtkWidget      *widget)
{
  g_assert (DZL_IS_DOCK_MANAGER (self));
  g_assert (GTK_IS_WIDGET (widget));

  g_signal_connect_object (widget,
                           "hierarchy-changed",
                           G_CALLBACK (dzl_dock_manager_hierarchy_changed),
                           self,
                           G_CONNECT_SWAPPED);

  dzl_dock_manager_hierarchy_changed (self, NULL, widget);
}

static void
dzl_dock_manager_weak_notify (gpointer  data,
                              GObject  *where_the_object_was)
{
  DzlDockManager *self = data;
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_assert (DZL_IS_DOCK_MANAGER (self));

  g_ptr_array_remove (priv->docks, where_the_object_was);
}

static void
dzl_dock_manager_real_register_dock (DzlDockManager *self,
                                     DzlDock        *dock)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (DZL_IS_DOCK (dock));

  g_object_weak_ref (G_OBJECT (dock), dzl_dock_manager_weak_notify, self);
  g_ptr_array_add (priv->docks, dock);
  dzl_dock_manager_watch_toplevel (self, GTK_WIDGET (dock));
}

static void
dzl_dock_manager_real_unregister_dock (DzlDockManager *self,
                                       DzlDock        *dock)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);
  guint i;

  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (DZL_IS_DOCK (dock));

  for (i = 0; i < priv->docks->len; i++)
    {
      DzlDock *iter = g_ptr_array_index (priv->docks, i);

      if (iter == dock)
        {
          g_object_weak_unref (G_OBJECT (dock), dzl_dock_manager_weak_notify, self);
          g_ptr_array_remove_index (priv->docks, i);
          break;
        }
    }
}

static void
dzl_dock_manager_finalize (GObject *object)
{
  DzlDockManager *self = (DzlDockManager *)object;
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_clear_object (&priv->grab);

  g_clear_pointer (&priv->queued_focus_by_toplevel, g_hash_table_unref);

  dzl_clear_source (&priv->queued_handler);

  while (priv->docks->len > 0)
    {
      DzlDock *dock = g_ptr_array_index (priv->docks, priv->docks->len - 1);

      g_object_weak_unref (G_OBJECT (dock), dzl_dock_manager_weak_notify, self);
      g_ptr_array_remove_index (priv->docks, priv->docks->len - 1);
    }

  g_clear_pointer (&priv->docks, g_ptr_array_unref);

  G_OBJECT_CLASS (dzl_dock_manager_parent_class)->finalize (object);
}

static void
dzl_dock_manager_class_init (DzlDockManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dzl_dock_manager_finalize;

  klass->register_dock = dzl_dock_manager_real_register_dock;
  klass->unregister_dock = dzl_dock_manager_real_unregister_dock;

  signals [REGISTER_DOCK] =
    g_signal_new ("register-dock",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlDockManagerClass, register_dock),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, DZL_TYPE_DOCK);

  signals [UNREGISTER_DOCK] =
    g_signal_new ("unregister-dock",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DzlDockManagerClass, unregister_dock),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, DZL_TYPE_DOCK);
}

static void
dzl_dock_manager_init (DzlDockManager *self)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  priv->docks = g_ptr_array_new ();
}

DzlDockManager *
dzl_dock_manager_new (void)
{
  return g_object_new (DZL_TYPE_DOCK_MANAGER, NULL);
}

void
dzl_dock_manager_register_dock (DzlDockManager *self,
                                DzlDock        *dock)
{
  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (DZL_IS_DOCK (dock));

  g_signal_emit (self, signals [REGISTER_DOCK], 0, dock);
}

void
dzl_dock_manager_unregister_dock (DzlDockManager *self,
                                  DzlDock        *dock)
{
  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (DZL_IS_DOCK (dock));

  g_signal_emit (self, signals [UNREGISTER_DOCK], 0, dock);
}

/**
 * dzl_dock_manager_pause_grabs:
 * @self: a #DzlDockManager
 *
 * Requests that the transient grab monitoring stop until
 * dzl_dock_manager_unpause_grabs() is called.
 *
 * This might be useful while setting up UI so that you don't focus
 * something unexpectedly.
 *
 * This function may be called multiple times and after an equivalent
 * number of calls to dzl_dock_manager_unpause_grabs(), transient
 * grab monitoring will continue.
 *
 * Since: 3.26
 */
void
dzl_dock_manager_pause_grabs (DzlDockManager *self)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (priv->pause_count >= 0);

  priv->pause_count++;
}

/**
 * dzl_dock_manager_unpause_grabs:
 * @self: a #DzlDockManager
 *
 * Unpauses a previous call to dzl_dock_manager_pause_grabs().
 *
 * Once the pause count returns to zero, transient grab monitoring
 * will be restored.
 *
 * Since: 3.26
 */
void
dzl_dock_manager_unpause_grabs (DzlDockManager *self)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));
  g_return_if_fail (priv->pause_count > 0);

  priv->pause_count--;
}

void
dzl_dock_manager_release_transient_grab (DzlDockManager *self)
{
  DzlDockManagerPrivate *priv = dzl_dock_manager_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_MANAGER (self));

  if (priv->grab != NULL)
    {
      g_autoptr(DzlDockTransientGrab) grab = g_steal_pointer (&priv->grab);
      dzl_dock_transient_grab_cancel (grab);
    }

  dzl_clear_source (&priv->queued_handler);
}
