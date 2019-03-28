/* dzl-child-property-action.c
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
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

#include "gui/widgets/dzl/dzl-child-property-action.h"
#include "gui/widgets/dzl/dzl-util-private.h"

struct _DzlChildPropertyAction
{
  GObject       parent_instance;

  GtkContainer *container;
  GtkWidget    *child;

  const gchar  *child_property_name;
  const gchar  *name;
};

enum {
  PROP_0,
  PROP_CHILD,
  PROP_CHILD_PROPERTY_NAME,
  PROP_CONTAINER,
  N_PROPS,

  PROP_ENABLED,
  PROP_NAME,
  PROP_PARAMETER_TYPE,
  PROP_STATE,
  PROP_STATE_TYPE,
};

static void action_iface_init (GActionInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DzlChildPropertyAction, dzl_child_property_action, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, action_iface_init))

static GParamSpec *properties [N_PROPS];

static const gchar *
dzl_child_property_action_get_name (GAction *action)
{
  return DZL_CHILD_PROPERTY_ACTION (action)->name;
}

static const GVariantType *
dzl_child_property_action_get_state_type (GAction *action)
{
  DzlChildPropertyAction *self = DZL_CHILD_PROPERTY_ACTION (action);

  if (self->container != NULL &&
      self->child != NULL &&
      self->child_property_name != NULL)
    {
      GParamSpec *pspec;

      pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (self->container),
                                                       self->child_property_name);

      if (pspec != NULL)
        {
          if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
            return G_VARIANT_TYPE ("b");
          else if (G_IS_PARAM_SPEC_INT (pspec))
            return G_VARIANT_TYPE ("i");
          else if (G_IS_PARAM_SPEC_UINT (pspec))
            return G_VARIANT_TYPE ("u");
          else if (G_IS_PARAM_SPEC_STRING (pspec))
            return G_VARIANT_TYPE ("s");
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec))
            return G_VARIANT_TYPE ("d");
          else if (G_IS_PARAM_SPEC_FLOAT (pspec))
            return G_VARIANT_TYPE ("d");
        }
    }

  g_warning ("Failed to discover state type for child property %s",
             self->child_property_name);

  return NULL;
}

static const GVariantType *
dzl_child_property_action_get_parameter_type (GAction *action)
{
  const GVariantType *state_type = g_action_get_state_type (action);

  if (g_variant_type_equal (state_type, G_VARIANT_TYPE ("b")))
    return NULL;

  return state_type;
}


static GVariant *
dzl_child_property_action_get_state_hint (GAction *action)
{
  return NULL;
}

static gboolean
dzl_child_property_action_get_enabled (GAction *action)
{
  return TRUE;
}

static GVariant *
dzl_child_property_action_get_state (GAction *action)
{
  DzlChildPropertyAction *self = DZL_CHILD_PROPERTY_ACTION (action);

  g_warn_if_fail (DZL_IS_CHILD_PROPERTY_ACTION (self));

  if (self->container != NULL &&
      self->child != NULL &&
      self->child_property_name != NULL)
    {
      GParamSpec *pspec;

      pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (self->container),
                                                       self->child_property_name);

      if (pspec != NULL)
        {
          g_auto(GValue) value = G_VALUE_INIT;
          GVariant *ret = NULL;

          g_value_init (&value, pspec->value_type);
          gtk_container_child_get_property (self->container,
                                            self->child,
                                            self->child_property_name,
                                            &value);

          if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
            ret = g_variant_new_boolean (g_value_get_boolean (&value));
          else if (G_IS_PARAM_SPEC_INT (pspec))
            ret = g_variant_new_int32 (g_value_get_int (&value));
          else if (G_IS_PARAM_SPEC_UINT (pspec))
            ret = g_variant_new_uint32 (g_value_get_uint (&value));
          else if (G_IS_PARAM_SPEC_STRING (pspec))
            ret = g_variant_new_string (g_value_get_string (&value));
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec))
            ret = g_variant_new_double (g_value_get_double (&value));
          else if (G_IS_PARAM_SPEC_FLOAT (pspec))
            ret = g_variant_new_double (g_value_get_double (&value));

          if (ret)
            return g_variant_ref_sink (ret);
        }
    }

  g_warning ("Failed to determine default state");

  return NULL;
}

static void
dzl_child_property_action_change_state (GAction  *action,
                                        GVariant *state)
{
  DzlChildPropertyAction *self = DZL_CHILD_PROPERTY_ACTION (action);

  if (self->container != NULL &&
      self->child != NULL &&
      self->child_property_name != NULL)
    {
      GParamSpec *pspec;

      pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (self->container),
                                                       self->child_property_name);

      if (pspec != NULL)
        {
          g_auto(GValue) value = G_VALUE_INIT;

          g_value_init (&value, pspec->value_type);

          if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
            {
              if (!g_variant_is_of_type (state, G_VARIANT_TYPE_BOOLEAN))
                {
                  g_warning ("Expected 'b', got %s", g_variant_get_type_string (state));
                  return;
                }

              g_value_set_boolean (&value, g_variant_get_boolean (state));
            }
          else if (G_IS_PARAM_SPEC_INT (pspec))
            {
              if (!g_variant_is_of_type (state, G_VARIANT_TYPE_INT32))
                {
                  g_warning ("Expected 'i', got %s", g_variant_get_type_string (state));
                  return;
                }

              g_value_set_int (&value, g_variant_get_int32 (state));
            }
          else if (G_IS_PARAM_SPEC_UINT (pspec))
            {
              if (!g_variant_is_of_type (state, G_VARIANT_TYPE_UINT32))
                {
                  g_warning ("Expected 'u', got %s", g_variant_get_type_string (state));
                  return;
                }

              g_value_set_uint (&value, g_variant_get_uint32 (state));
            }
          else if (G_IS_PARAM_SPEC_STRING (pspec))
            {
              if (!g_variant_is_of_type (state, G_VARIANT_TYPE_STRING))
                {
                  g_warning ("Expected 's', got %s", g_variant_get_type_string (state));
                  return;
                }

              g_value_set_string (&value, g_variant_get_string (state, NULL));
            }
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec) || G_IS_PARAM_SPEC_FLOAT (pspec))
            {
              if (!g_variant_is_of_type (state, G_VARIANT_TYPE_STRING))
                {
                  g_warning ("Expected 'd', got %s", g_variant_get_type_string (state));
                  return;
                }

              if (G_IS_PARAM_SPEC_DOUBLE (pspec))
                g_value_set_double (&value, g_variant_get_double (state));
              else
                g_value_set_float (&value, g_variant_get_double (state));
            }
          else
            {
              g_warning ("I don't know how to handle %s property types.",
                         g_type_name (pspec->value_type));
              return;
            }

          gtk_container_child_set_property (self->container,
                                            self->child,
                                            self->child_property_name,
                                            &value);
          g_object_notify (G_OBJECT (self), "state");

          return;
        }
    }

  g_warning ("Attempt to change state on incapable child property action");
}

static void
dzl_child_property_action_activate (GAction  *action,
                                    GVariant *parameter)
{
  DzlChildPropertyAction *self = (DzlChildPropertyAction *)action;

  g_warn_if_fail (DZL_IS_CHILD_PROPERTY_ACTION (self));

  if (self->container != NULL &&
      self->child != NULL &&
      self->child_property_name != NULL)
    {
      GParamSpec *pspec;

      pspec = gtk_container_class_find_child_property (G_OBJECT_GET_CLASS (self->container),
                                                       self->child_property_name);

      if (pspec != NULL)
        {
          g_auto(GValue) value = G_VALUE_INIT;

          if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
            {
              g_value_init (&value, G_TYPE_BOOLEAN);

              if (parameter != NULL)
                g_value_set_boolean (&value, g_variant_get_boolean (parameter));
              else
                {
                  g_auto(GValue) previous = G_VALUE_INIT;

                  g_value_init (&previous, G_TYPE_BOOLEAN);
                  gtk_container_child_get_property (self->container,
                                                    self->child,
                                                    self->child_property_name,
                                                    &previous);
                  g_value_set_boolean (&value, !g_value_get_boolean (&previous));
                }
            }
          else if (G_IS_PARAM_SPEC_INT (pspec) && parameter != NULL)
            {
              g_value_init (&value, G_TYPE_INT);
              g_value_set_int (&value, g_variant_get_int32 (parameter));
            }
          else if (G_IS_PARAM_SPEC_UINT (pspec) && parameter != NULL)
            {
              g_value_init (&value, G_TYPE_UINT);
              g_value_set_uint (&value, g_variant_get_uint32 (parameter));
            }
          else if (G_IS_PARAM_SPEC_STRING (pspec) && parameter != NULL)
            {
              g_value_init (&value, G_TYPE_STRING);
              g_value_set_string (&value, g_variant_get_string (parameter, NULL));
            }
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec) || G_IS_PARAM_SPEC_FLOAT (pspec))
            {
              if (parameter != NULL)
                {
                  g_value_init (&value, G_TYPE_DOUBLE);
                  g_value_set_double (&value, g_variant_get_double (parameter));
                }
            }
          else
            {
              g_warning ("Failed to transform state type");
              return;
            }

          gtk_container_child_set_property (self->container, self->child, pspec->name, &value);

          return;
        }
    }

  g_warning ("I don't know how to activate %s", self->name);
}

static void
action_iface_init (GActionInterface *iface)
{
  iface->get_name = dzl_child_property_action_get_name;
  iface->get_parameter_type = dzl_child_property_action_get_parameter_type;
  iface->get_state_type = dzl_child_property_action_get_state_type;
  iface->get_state_hint = dzl_child_property_action_get_state_hint;
  iface->get_enabled = dzl_child_property_action_get_enabled;
  iface->get_state = dzl_child_property_action_get_state;
  iface->change_state = dzl_child_property_action_change_state;
  iface->activate = dzl_child_property_action_activate;
}

static void
child_notify_cb (DzlChildPropertyAction *self,
                 GParamSpec             *pspec,
                 GtkWidget              *child)
{
  g_warn_if_fail (DZL_IS_CHILD_PROPERTY_ACTION (self));
  g_warn_if_fail (pspec != NULL);
  g_warn_if_fail (GTK_IS_WIDGET (child));

  g_object_notify (G_OBJECT (self), "state");
}

static void
dzl_child_property_action_dispose (GObject *object)
{
  DzlChildPropertyAction *self = (DzlChildPropertyAction *)object;

  dzl_clear_weak_pointer (&self->container);
  dzl_clear_weak_pointer (&self->child);

  G_OBJECT_CLASS (dzl_child_property_action_parent_class)->dispose (object);
}

static void
dzl_child_property_action_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  DzlChildPropertyAction *self = DZL_CHILD_PROPERTY_ACTION (object);

  switch (prop_id)
    {
    case PROP_CONTAINER:
      g_value_set_object (value, self->container);
      break;

    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    case PROP_CHILD_PROPERTY_NAME:
      g_value_set_static_string (value, self->child_property_name);
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, dzl_child_property_action_get_enabled (G_ACTION (self)));
      break;

    case PROP_PARAMETER_TYPE:
      g_value_set_boxed (value, dzl_child_property_action_get_parameter_type (G_ACTION (self)));
      break;

    case PROP_STATE:
      g_value_take_variant (value, dzl_child_property_action_get_state (G_ACTION (self)));
      break;

    case PROP_STATE_TYPE:
      g_value_set_boxed (value, dzl_child_property_action_get_state_type (G_ACTION (self)));
      break;

    case PROP_NAME:
      g_value_set_static_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_child_property_action_class_init (DzlChildPropertyActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dzl_child_property_action_dispose;
  object_class->get_property = dzl_child_property_action_get_property;

  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  g_object_class_override_property (object_class, PROP_NAME, "name");
  g_object_class_override_property (object_class, PROP_PARAMETER_TYPE, "parameter-type");
  g_object_class_override_property (object_class, PROP_STATE, "state");
  g_object_class_override_property (object_class, PROP_STATE_TYPE, "state-type");

  properties [PROP_CHILD] =
    g_param_spec_object ("child",
                         "Child",
                         "The child widget",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CHILD_PROPERTY_NAME] =
    g_param_spec_string ("child-property-name",
                         "Child Property Name",
                         "The name of the child property",
                         NULL,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_CONTAINER] =
    g_param_spec_object ("container",
                         "Container",
                         "The container widget",
                         GTK_TYPE_CONTAINER,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
dzl_child_property_action_init (DzlChildPropertyAction *self)
{
}

/**
 * dzl_child_property_action_new:
 * @name: the name of the action
 * @container: the container of the widget
 * @child: the widget for the child property
 * @child_property_name: the name of the child property
 *
 * This creates a new #GAction that will change when the underlying child
 * property of @container changes for @child.
 *
 * Returns: (transfer full): A new #DzlChildPropertyAction.
 */
GAction *
dzl_child_property_action_new (const gchar  *name,
                               GtkContainer *container,
                               GtkWidget    *child,
                               const gchar  *child_property_name)
{
  g_autoptr(DzlChildPropertyAction) self = NULL;
  g_autofree gchar *signal_detail = NULL;

  g_return_val_if_fail (GTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (child_property_name != NULL, NULL);

  self = g_object_new (DZL_TYPE_CHILD_PROPERTY_ACTION, NULL);
  self->name = g_intern_string (name);
  self->child_property_name = g_intern_string (child_property_name);
  dzl_set_weak_pointer (&self->container, container);
  dzl_set_weak_pointer (&self->child, child);

  signal_detail = g_strdup_printf ("child-notify::%s", child_property_name);

  g_signal_connect_object (child,
                           signal_detail,
                           G_CALLBACK (child_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  return G_ACTION (g_steal_pointer (&self));
}
