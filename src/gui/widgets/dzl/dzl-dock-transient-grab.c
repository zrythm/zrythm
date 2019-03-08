/* dzl-dock-transient-grab.c
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

#define G_LOG_DOMAIN "dzl-dock-transient-grab"

#include "gui/widgets/dzl/dzl-dock-transient-grab.h"
#include "gui/widgets/dzl/dzl-macros.h"

struct _DzlDockTransientGrab
{
  GObject parent_instance;

  GPtrArray *items;
  GHashTable *hidden;

  guint timeout;

  guint acquired : 1;
};

G_DEFINE_TYPE (DzlDockTransientGrab, dzl_dock_transient_grab, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_TIMEOUT,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
dzl_dock_transient_grab_weak_notify (gpointer  data,
                                     GObject  *where_object_was)
{
  DzlDockTransientGrab *self = data;

  g_assert (DZL_IS_DOCK_TRANSIENT_GRAB (self));

  g_ptr_array_remove (self->items, where_object_was);
}

static void
dzl_dock_transient_grab_finalize (GObject *object)
{
  DzlDockTransientGrab *self = (DzlDockTransientGrab *)object;
  guint i;

  for (i = 0; i < self->items->len; i++)
    g_object_weak_unref (g_ptr_array_index (self->items, i),
                         dzl_dock_transient_grab_weak_notify,
                         self);

  g_clear_pointer (&self->items, g_ptr_array_unref);
  g_clear_pointer (&self->hidden, g_hash_table_unref);

  G_OBJECT_CLASS (dzl_dock_transient_grab_parent_class)->finalize (object);
}

static void
dzl_dock_transient_grab_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DzlDockTransientGrab *self = DZL_DOCK_TRANSIENT_GRAB (object);

  switch (prop_id)
    {
    case PROP_TIMEOUT:
      g_value_set_uint (value, dzl_dock_transient_grab_get_timeout (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_transient_grab_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DzlDockTransientGrab *self = DZL_DOCK_TRANSIENT_GRAB (object);

  switch (prop_id)
    {
    case PROP_TIMEOUT:
      dzl_dock_transient_grab_set_timeout (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_transient_grab_class_init (DzlDockTransientGrabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dzl_dock_transient_grab_finalize;
  object_class->get_property = dzl_dock_transient_grab_get_property;
  object_class->set_property = dzl_dock_transient_grab_set_property;

  properties [PROP_TIMEOUT] =
    g_param_spec_uint ("timeout",
                       "Timeout",
                       "Timeout",
                       0,
                       G_MAXUINT,
                       0,
                       (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dzl_dock_transient_grab_init (DzlDockTransientGrab *self)
{
  self->items = g_ptr_array_new ();
  self->hidden = g_hash_table_new (NULL, NULL);
}

DzlDockTransientGrab *
dzl_dock_transient_grab_new (void)
{
  return g_object_new (DZL_TYPE_DOCK_TRANSIENT_GRAB, NULL);
}

guint
dzl_dock_transient_grab_get_timeout (DzlDockTransientGrab *self)
{
  g_return_val_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self), 0);

  return self->timeout;
}

void
dzl_dock_transient_grab_set_timeout (DzlDockTransientGrab *self,
                                     guint                 timeout)
{
  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));

  if (timeout != self->timeout)
    {
      self->timeout = timeout;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_TIMEOUT]);
    }
}

gboolean
dzl_dock_transient_grab_contains (DzlDockTransientGrab *self,
                                  DzlDockItem          *item)
{
  guint i;

  g_return_val_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self), FALSE);
  g_return_val_if_fail (DZL_IS_DOCK_ITEM (item), FALSE);

  for (i = 0; i < self->items->len; i++)
    if (g_ptr_array_index (self->items, i) == item)
      return TRUE;

  return FALSE;
}

void
dzl_dock_transient_grab_add_item (DzlDockTransientGrab *self,
                                  DzlDockItem          *item)
{
  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (DZL_IS_DOCK_ITEM (item));

  g_ptr_array_add (self->items, item);

  g_object_weak_ref (G_OBJECT (item),
                     dzl_dock_transient_grab_weak_notify,
                     self);
}

static void
dzl_dock_transient_grab_remove_index (DzlDockTransientGrab *self,
                                      guint                 index)
{
  DzlDockItem *item;

  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (index < self->items->len);

  item = g_ptr_array_index (self->items, index);
  g_object_weak_unref (G_OBJECT (item),
                       dzl_dock_transient_grab_weak_notify,
                       self);
  g_ptr_array_remove_index (self->items, index);
  g_hash_table_remove (self->hidden, item);
}

void
dzl_dock_transient_grab_remove_item (DzlDockTransientGrab *self,
                                     DzlDockItem          *item)
{
  guint i;

  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (DZL_IS_DOCK_ITEM (item));

  for (i = 0; i < self->items->len; i++)
    {
      DzlDockItem *iter = g_ptr_array_index (self->items, i);

      if (item == iter)
        {
          dzl_dock_transient_grab_remove_index (self, i);
          return;
        }
    }
}

void
dzl_dock_transient_grab_acquire (DzlDockTransientGrab *self)
{
  guint i;

  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (self->acquired == FALSE);

  self->acquired = TRUE;

  for (i = self->items->len; i > 1; i--)
    {
      DzlDockItem *parent = g_ptr_array_index (self->items, i - 1);
      DzlDockItem *child = g_ptr_array_index (self->items, i - 2);

      if (!dzl_dock_item_get_child_visible (parent, child))
        {
          dzl_dock_item_set_child_visible (parent, child, TRUE);
          g_hash_table_insert (self->hidden, child, NULL);
        }
    }
}

void
dzl_dock_transient_grab_release (DzlDockTransientGrab *self)
{
  g_autoptr(GPtrArray) matches = NULL;

  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (self->acquired == TRUE);
  g_return_if_fail (self->items != NULL);

  g_object_ref (self);

  matches = g_ptr_array_new_full (self->items->len, g_object_unref);

  /* Collect our matches and increment their reference count so that
   * we can ensure they stick around while we notify our matches in
   * the followup iteration.
   */
  for (guint i = 0; i < self->items->len; i++)
    {
      DzlDockItem *item = g_ptr_array_index (self->items, i);

      if (g_hash_table_contains (self->hidden, item))
        g_ptr_array_add (matches, g_object_ref (item));
    }

  /*
   * Notify in reverse order to increase chances we don't collide with
   * any state changing as we propagate.
   */
  for (guint i = matches->len; i > 0; i--)
    {
      DzlDockItem *item = g_ptr_array_index (matches, i - 1);
      DzlDockItem *parent = dzl_dock_item_get_parent (item);

      if (parent != NULL)
        dzl_dock_item_set_child_visible (parent, item, FALSE);
    }

  g_object_unref (self);
}

gboolean
dzl_dock_transient_grab_is_descendant (DzlDockTransientGrab *self,
                                       GtkWidget            *widget)
{
  g_return_val_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self), FALSE);

  if (self->items->len > 0)
    {
      GtkWidget *item = g_ptr_array_index (self->items, 0);
      GtkWidget *ancestor;

      ancestor = gtk_widget_get_ancestor (widget, DZL_TYPE_DOCK_ITEM);

      return (item == ancestor);
    }

  return FALSE;
}

void
dzl_dock_transient_grab_steal_common_ancestors (DzlDockTransientGrab *self,
                                                DzlDockTransientGrab *other)
{
  guint i;

  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));
  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (other));

  for (i = other->items->len; i > 0; i--)
    {
      DzlDockItem *item = g_ptr_array_index (other->items, i - 1);

      if (dzl_dock_transient_grab_contains (self, item))
        {
          /*
           * Since we are stealing the common ancestors, we don't want the
           * previous grab to hide them when releasing, so clear the items
           * from the hash of children it wants to hide.
           */
          g_hash_table_remove (other->hidden, item);

          dzl_dock_transient_grab_add_item (self, item);
          dzl_dock_transient_grab_remove_index (other, i - 1);
        }
    }
}

void
dzl_dock_transient_grab_cancel (DzlDockTransientGrab *self)
{
  g_return_if_fail (DZL_IS_DOCK_TRANSIENT_GRAB (self));

  dzl_clear_source (&self->timeout);
  while (self->items->len)
    dzl_dock_transient_grab_remove_index (self, self->items->len - 1);
  g_hash_table_remove_all (self->hidden);
}
