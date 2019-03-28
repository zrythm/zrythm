/* dzl-util.c
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

#include <string.h>

#include "gui/widgets/dzl/dzl-util-private.h"

static void
dzl_gtk_border_sum (GtkBorder       *one,
                    const GtkBorder *two)
{
  one->top += two->top;
  one->right += two->right;
  one->bottom += two->bottom;
  one->left += two->left;
}

void
dzl_gtk_style_context_get_borders (GtkStyleContext *style_context,
                                   GtkBorder       *borders)
{
  GtkBorder border = { 0 };
  GtkBorder padding = { 0 };
  GtkBorder margin = { 0 };
  GtkStateFlags state;

  g_return_if_fail (GTK_IS_STYLE_CONTEXT (style_context));
  g_return_if_fail (borders != NULL);

  memset (borders, 0, sizeof *borders);

  state = gtk_style_context_get_state (style_context);

  gtk_style_context_get_border (style_context, state, &border);
  gtk_style_context_get_padding (style_context, state, &padding);
  gtk_style_context_get_margin (style_context, state, &margin);

  dzl_gtk_border_sum (borders, &padding);
  dzl_gtk_border_sum (borders, &border);
  dzl_gtk_border_sum (borders, &margin);
}

static void
split_action_name (const gchar  *action_name,
                   gchar       **prefix,
                   gchar       **name)
{
  const gchar *dot;

  g_warn_if_fail (prefix != NULL);
  g_warn_if_fail (name != NULL);

  *prefix = NULL;
  *name = NULL;

  if (action_name == NULL)
    return;

  dot = strchr (action_name, '.');

  if (dot == NULL)
    *name = g_strdup (action_name);
  else
    {
      *prefix = g_strndup (action_name, dot - action_name);
      *name = g_strdup (dot + 1);
    }
}

gboolean
dzl_gtk_widget_activate_action (GtkWidget   *widget,
                                const gchar *full_action_name,
                                GVariant    *parameter)
{
  GtkWidget *toplevel;
  GApplication *app;
  GActionGroup *group = NULL;
  gchar *prefix = NULL;
  gchar *action_name = NULL;
  const gchar *dot;
  gboolean ret = FALSE;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (full_action_name, FALSE);

  dot = strchr (full_action_name, '.');

  if (dot == NULL)
    {
      prefix = NULL;
      action_name = g_strdup (full_action_name);
    }
  else
    {
      prefix = g_strndup (full_action_name, dot - full_action_name);
      action_name = g_strdup (dot + 1);
    }

  /*
   * TODO: Support non-grouped names. We need to walk
   *       through all the groups at each level to do this.
   */
  if (prefix == NULL)
    prefix = g_strdup ("win");

  app = g_application_get_default ();
  toplevel = gtk_widget_get_toplevel (widget);

  while ((group == NULL) && (widget != NULL))
    {
      group = gtk_widget_get_action_group (widget, prefix);

      if G_UNLIKELY (GTK_IS_POPOVER (widget))
        {
          GtkWidget *relative_to;

          relative_to = gtk_popover_get_relative_to (GTK_POPOVER (widget));

          if (relative_to != NULL)
            widget = relative_to;
          else
            widget = gtk_widget_get_parent (widget);
        }
      else
        {
          widget = gtk_widget_get_parent (widget);
        }
    }

  if (!group && g_str_equal (prefix, "win") && G_IS_ACTION_GROUP (toplevel))
    group = G_ACTION_GROUP (toplevel);

  if (!group && g_str_equal (prefix, "app") && G_IS_ACTION_GROUP (app))
    group = G_ACTION_GROUP (app);

  if (group && g_action_group_has_action (group, action_name))
    {
      g_action_group_activate_action (group, action_name, parameter);
      ret = TRUE;
      goto cleanup;
    }

  if (parameter && g_variant_is_floating (parameter))
    {
      parameter = g_variant_ref_sink (parameter);
      g_variant_unref (parameter);
    }

  g_warning ("Failed to locate action %s.%s", prefix, action_name);

cleanup:
  g_free (prefix);
  g_free (action_name);

  return ret;
}

static GActionGroup *
find_group_with_action (GtkWidget   *widget,
                        const gchar *prefix,
                        const gchar *name)
{
  GActionGroup *group;

  g_warn_if_fail (GTK_IS_WIDGET (widget));
  g_warn_if_fail (name != NULL);

  /*
   * GtkWidget does not provide a way to get group names,
   * so there is nothing more we can do if prefix is NULL.
   */
  if (prefix == NULL)
    return NULL;

  if (g_str_equal (prefix, "app"))
    group = G_ACTION_GROUP (g_application_get_default ());
  else
    group = gtk_widget_get_action_group (widget, prefix);

  if (group != NULL && g_action_group_has_action (group, name))
    return group;

  widget = gtk_widget_get_parent (widget);

  if (widget != NULL)
    return find_group_with_action (widget, prefix, name);

  return NULL;
}

GVariant *
dzl_gtk_widget_get_action_state (GtkWidget   *widget,
                                 const gchar *action_name)
{
  GActionGroup *group;
  gchar *prefix = NULL;
  gchar *name = NULL;
  GVariant *ret = NULL;

  split_action_name (action_name, &prefix, &name);
  if (name == NULL || prefix == NULL)
    goto cleanup;

  group = find_group_with_action (widget, prefix, name);
  if (group == NULL)
    goto cleanup;

  ret = g_action_group_get_action_state (group, name);

cleanup:
  g_free (name);
  g_free (prefix);

  return ret;
}

GActionGroup *
dzl_gtk_widget_find_group_for_action (GtkWidget   *widget,
                                      const gchar *action_name)
{
  GActionGroup *group = NULL;
  gchar *prefix = NULL;
  gchar *name = NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  if (action_name == NULL)
    return NULL;

  split_action_name (action_name, &prefix, &name);
  if (name == NULL || prefix == NULL)
    goto cleanup;

  group = find_group_with_action (widget, prefix, name);

cleanup:
  g_free (name);
  g_free (prefix);

  return group;
}

void
dzl_g_action_name_parse (const gchar  *action_name,
                         gchar       **prefix,
                         gchar       **name)
{
  split_action_name (action_name, prefix, name);
}

gboolean
dzl_g_action_name_parse_full (const gchar  *detailed_action_name,
                              gchar       **prefix,
                              gchar       **name,
                              GVariant    **target)
{
  g_autofree gchar *full_name = NULL;
  g_autoptr(GVariant) target_value = NULL;
  const gchar *dot;

  if (detailed_action_name == NULL)
    return FALSE;

  if (!g_action_parse_detailed_name (detailed_action_name, &full_name, &target_value, NULL))
    return FALSE;

  if (target_value != NULL)
    g_variant_take_ref (target_value);

  dot = strchr (full_name, '.');

  if (dot != NULL)
    {
      if (prefix != NULL)
        *prefix = g_strndup (full_name, dot - full_name);

      if (name != NULL)
        *name = g_strdup (dot + 1);
    }
  else
    {
      *prefix = NULL;
      *name = g_steal_pointer (&full_name);
    }

  if (target != NULL)
    *target = g_steal_pointer (&target_value);

  return TRUE;
}

void
dzl_gtk_allocation_subtract_border (GtkAllocation *alloc,
                                    GtkBorder     *border)
{
  g_return_if_fail (alloc != NULL);
  g_return_if_fail (border != NULL);

  alloc->x += border->left;
  alloc->y += border->top;
  alloc->width -= (border->left + border->right);
  alloc->height -= (border->top + border->bottom);
}

void
dzl_gtk_widget_add_class (GtkWidget   *widget,
                          const gchar *class_name)
{
  gtk_style_context_add_class (gtk_widget_get_style_context (widget), class_name);
}

void
dzl_gtk_widget_class_add_css_resource (GtkWidgetClass *widget_class,
                                       const gchar    *resource)
{
  GdkScreen *screen = gdk_screen_get_default ();

  g_return_if_fail (widget_class != NULL);
  g_return_if_fail (resource != NULL);

  if (screen != NULL)
    {
      g_autoptr(GtkCssProvider) provider = NULL;

      /*
       * It would be nice if our theme data could be more theme friendly.
       * However, we need to be higher than SETTINGS so that some of our
       * stuff takes effect, but that is already higher than theming.
       *
       * So really the only proper answer is to get themes to style us and
       * eventually drop all CSS. Given that is impossible... I'm not sure
       * what other options we really have. Some themes will just need to
       * !important or whatever when it matters.
       */

      provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_resource (provider, resource);
      gtk_style_context_add_provider_for_screen (screen,
                                                 GTK_STYLE_PROVIDER (provider),
                                                 GTK_STYLE_PROVIDER_PRIORITY_SETTINGS + 50);
    }

}
