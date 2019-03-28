/* dzl-gtk.c
 *
 * Copyright (C) 2015-2017 Christian Hergert <chergert@redhat.com>
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

#include "gui/widgets/dzl/dzl-animation.h"
#include "gui/widgets/dzl/dzl-gtk.h"

gboolean
dzl_gtk_widget_action (GtkWidget   *widget,
                       const gchar *prefix,
                       const gchar *action_name,
                       GVariant    *parameter)
{
  GtkWidget *toplevel;
  GApplication *app;
  GActionGroup *group = NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (prefix, FALSE);
  g_return_val_if_fail (action_name, FALSE);

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
      return TRUE;
    }

  if (parameter && g_variant_is_floating (parameter))
    {
      parameter = g_variant_ref_sink (parameter);
      g_variant_unref (parameter);
    }

  g_warning ("Failed to locate action %s.%s", prefix, action_name);

  return FALSE;
}

gboolean
dzl_gtk_widget_action_with_string (GtkWidget   *widget,
                                   const gchar *group,
                                   const gchar *name,
                                   const gchar *param)
{
  g_autoptr(GVariant) variant = NULL;
  gboolean ret;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (group != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (param == NULL)
    param = "";

  if (*param != 0)
    {
      g_autoptr(GError) error = NULL;

      variant = g_variant_parse (NULL, param, NULL, NULL, &error);

      if (variant == NULL)
        {
          g_warning ("can't parse keybinding parameters \"%s\": %s",
                     param, error->message);
          return FALSE;
        }
    }

  ret = dzl_gtk_widget_action (widget, group, name, variant);

  return ret;
}

static void
show_callback (gpointer data)
{
  g_object_set_data (data, "DZL_FADE_ANIMATION", NULL);
  g_object_unref (data);
}

static void
hide_callback (gpointer data)
{
  GtkWidget *widget = data;

  g_object_set_data (data, "DZL_FADE_ANIMATION", NULL);
  gtk_widget_hide (widget);
  gtk_widget_set_opacity (widget, 1.0);
  g_object_unref (widget);
}

void
dzl_gtk_widget_hide_with_fade (GtkWidget *widget)
{
  GdkFrameClock *frame_clock;
  DzlAnimation *anim;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (gtk_widget_get_visible (widget))
    {
      anim = g_object_get_data (G_OBJECT (widget), "DZL_FADE_ANIMATION");
      if (anim != NULL)
        dzl_animation_stop (anim);

      frame_clock = gtk_widget_get_frame_clock (widget);
      anim = dzl_object_animate_full (widget,
                                      DZL_ANIMATION_LINEAR,
                                      1000,
                                      frame_clock,
                                      hide_callback,
                                      g_object_ref (widget),
                                      "opacity", 0.0,
                                      NULL);
      g_object_set_data_full (G_OBJECT (widget), "DZL_FADE_ANIMATION",
                              g_object_ref (anim), g_object_unref);
    }
}

void
dzl_gtk_widget_show_with_fade (GtkWidget *widget)
{
  GdkFrameClock *frame_clock;
  DzlAnimation *anim;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!gtk_widget_get_visible (widget))
    {
      anim = g_object_get_data (G_OBJECT (widget), "DZL_FADE_ANIMATION");
      if (anim != NULL)
        dzl_animation_stop (anim);

      frame_clock = gtk_widget_get_frame_clock (widget);
      gtk_widget_set_opacity (widget, 0.0);
      gtk_widget_show (widget);
      anim = dzl_object_animate_full (widget,
                                      DZL_ANIMATION_LINEAR,
                                      500,
                                      frame_clock,
                                      show_callback,
                                      g_object_ref (widget),
                                      "opacity", 1.0,
                                      NULL);
      g_object_set_data_full (G_OBJECT (widget), "DZL_FADE_ANIMATION",
                              g_object_ref (anim), g_object_unref);
    }
}

static void
dzl_gtk_widget_find_child_typed_cb (GtkWidget *widget,
                                    gpointer   user_data)
{
  struct {
    gpointer ret;
    GType type;
  } *state = user_data;

  if (state->ret != NULL)
    return;

  if (g_type_is_a (G_OBJECT_TYPE (widget), state->type))
    {
      state->ret = widget;
    }
  else if (GTK_IS_CONTAINER (widget))
    {
      gtk_container_foreach (GTK_CONTAINER (widget),
                             dzl_gtk_widget_find_child_typed_cb,
                             state);
    }
}

/**
 * dzl_gtk_widget_find_child_typed:
 *
 * Tries to locate a widget in a hierarchy given it's #GType.
 *
 * There is not an efficient implementation of this method, so use it
 * only when the hierarchy of widgets is small.
 *
 * Returns: (transfer none) (type Gtk.Widget) (nullable): A widget or %NULL
 */
gpointer
dzl_gtk_widget_find_child_typed (GtkWidget *widget,
                                 GType      child_type)
{
  struct {
    gpointer ret;
    GType type;
  } state;

  g_return_val_if_fail (GTK_IS_CONTAINER (widget), NULL);
  g_return_val_if_fail (g_type_is_a (child_type, GTK_TYPE_WIDGET), NULL);

  state.ret = NULL;
  state.type = child_type;

  gtk_container_foreach (GTK_CONTAINER (widget),
                         dzl_gtk_widget_find_child_typed_cb,
                         &state);

  return state.ret;
}

/**
 * dzl_gtk_text_buffer_remove_tag:
 *
 * Like gtk_text_buffer_remove_tag() but allows specifying that the tags
 * should be removed one at a time to avoid over-damaging the views
 * displaying @buffer.
 */
void
dzl_gtk_text_buffer_remove_tag (GtkTextBuffer     *buffer,
                                GtkTextTag        *tag,
                                const GtkTextIter *start,
                                const GtkTextIter *end,
                                gboolean           minimal_damage)
{
  GtkTextIter tag_begin;
  GtkTextIter tag_end;

  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (GTK_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (!minimal_damage)
    {
      gtk_text_buffer_remove_tag (buffer, tag, start, end);
      return;
    }

  tag_begin = *start;

  if (!gtk_text_iter_starts_tag (&tag_begin, tag))
    {
      if (!gtk_text_iter_forward_to_tag_toggle (&tag_begin, tag))
        return;
    }

  while (gtk_text_iter_starts_tag (&tag_begin, tag) &&
         gtk_text_iter_compare (&tag_begin, end) < 0)
    {
      gint count = 1;

      tag_end = tag_begin;

      /*
       * We might have found the start of another tag embedded
       * inside this tag. So keep scanning forward until we have
       * reached the right number of end tags.
       */

      while (gtk_text_iter_forward_to_tag_toggle (&tag_end, tag))
        {
          if (gtk_text_iter_starts_tag (&tag_end, tag))
            count++;
          else if (gtk_text_iter_ends_tag (&tag_end, tag))
            count--;

          if (count == 0)
            break;
        }

      if (gtk_text_iter_ends_tag (&tag_end, tag))
        gtk_text_buffer_remove_tag (buffer, tag, &tag_begin, &tag_end);

      tag_begin = tag_end;

      /*
       * Move to the next start tag. It's possible to have an overlapped
       * end tag, which would be non-ideal, but possible.
       */
      if (!gtk_text_iter_starts_tag (&tag_begin, tag))
        {
          while (gtk_text_iter_forward_to_tag_toggle (&tag_begin, tag))
            {
              if (gtk_text_iter_starts_tag (&tag_begin, tag))
                break;
            }
        }
    }
}

void
dzl_gtk_widget_add_style_class (GtkWidget   *widget,
                                const gchar *class_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (class_name != NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (widget), class_name);
}

void
dzl_gtk_widget_remove_style_class (GtkWidget   *widget,
                                   const gchar *class_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (class_name != NULL);

  gtk_style_context_remove_class (gtk_widget_get_style_context (widget), class_name);
}

void
dzl_gtk_widget_action_set (GtkWidget   *widget,
                           const gchar *group,
                           const gchar *name,
                           const gchar *first_property,
                           ...)
{
  GAction *action = NULL;
  va_list args;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (group != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (first_property != NULL);

  for (; widget; widget = gtk_widget_get_parent (widget))
    {
      GActionGroup *actions = gtk_widget_get_action_group (widget, group);

      if (G_IS_ACTION_MAP (actions))
        {
          action = g_action_map_lookup_action (G_ACTION_MAP (actions), name);

          if (action != NULL)
            break;
        }
    }

  if (action == NULL)
    {
      g_warning ("Failed to locate action %s.%s", group, name);
      return;
    }

  va_start (args, first_property);
  g_object_set_valist (G_OBJECT (action), first_property, args);
  va_end (args);
}

/**
 * dzl_gtk_widget_mux_action_groups:
 * @widget: a #GtkWidget
 * @from_widget: A #GtkWidget containing the groups to copy
 * @mux_key: (nullable): a unique key to represent the muxing
 *
 * This function will find all of the actions on @from_widget in various
 * groups and add them to @widget. As this just copies the action groups
 * over, note that it does not allow for muxing items within the same
 * group.
 *
 * You should specify a key for @mux_key so that if the same mux key is
 * seen again, the previous muxings will be removed.
 */
void
dzl_gtk_widget_mux_action_groups (GtkWidget   *widget,
                                  GtkWidget   *from_widget,
                                  const gchar *mux_key)
{
  const gchar * const *old_prefixes = NULL;
  g_auto(GStrv) prefixes = NULL;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (!from_widget || GTK_IS_WIDGET (from_widget));
  g_return_if_fail (widget != from_widget);

  if (mux_key == NULL)
    mux_key = "DZL_GTK_MUX_ACTIONS";

  /*
   * First check to see if there are any old action groups that
   * were muxed for which we need to unmux.
   */

  old_prefixes = g_object_get_data (G_OBJECT (widget), mux_key);

  if (old_prefixes != NULL)
    {
      for (guint i = 0; old_prefixes [i]; i++)
        {
          if (g_str_equal (old_prefixes [i], "win") || g_str_equal (old_prefixes [i], "app"))
            continue;

          gtk_widget_insert_action_group (widget, old_prefixes [i], NULL);
        }
    }

  /*
   * Now, if there is a from_widget to mux, get all of their action
   * groups and mux them across to our target widget.
   */

  if (from_widget != NULL)
    {
      g_autofree const gchar **tmp = gtk_widget_list_action_prefixes (from_widget);

      if (tmp != NULL)
        {
          prefixes = g_strdupv ((gchar **)tmp);

          for (guint i = 0; prefixes [i]; i++)
            {
              GActionGroup *group = gtk_widget_get_action_group (from_widget, prefixes [i]);

              if (g_str_equal (prefixes [i], "win") || g_str_equal (prefixes [i], "app"))
                continue;

              if G_UNLIKELY (group == NULL)
                continue;

              gtk_widget_insert_action_group (widget, prefixes[i], group);
            }
        }
    }

  /* Store the set of muxed prefixes so that we can unmux them later. */
  g_object_set_data_full (G_OBJECT (widget),
                          mux_key,
                          g_steal_pointer (&prefixes),
                          (GDestroyNotify) g_strfreev);
}

static gboolean
list_store_iter_middle (GtkListStore      *store,
                        const GtkTreeIter *begin,
                        const GtkTreeIter *end,
                        GtkTreeIter       *middle)
{
  g_warn_if_fail (store != NULL);
  g_warn_if_fail (begin != NULL);
  g_warn_if_fail (end != NULL);
  g_warn_if_fail (middle != NULL);
  g_warn_if_fail (middle->stamp == begin->stamp);
  g_warn_if_fail (middle->stamp == end->stamp);

  /*
   * middle MUST ALREADY BE VALID as it saves us some copying
   * as well as just makes things easier when binary searching.
   */

  middle->user_data = g_sequence_range_get_midpoint (begin->user_data, end->user_data);

  if (g_sequence_iter_is_end (middle->user_data))
    {
      middle->stamp = 0;
      return FALSE;
    }

  return TRUE;
}

static inline gboolean
list_store_iter_equal (const GtkTreeIter *a,
                       const GtkTreeIter *b)
{
  return a->user_data == b->user_data;
}

/**
 * dzl_gtk_list_store_insert_sorted:
 * @store: A #GtkListStore
 * @iter: (out): A location for a #GtkTextIter
 * @key: A key to compare to when binary searching
 * @compare_column: the column containing the data to compare
 * @compare_func: (scope call) (closure compare_data): A callback to compare
 * @compare_data: data for @compare_func
 *
 * This function will binary search the contents of @store looking for the
 * location to insert a new row.
 *
 * @compare_column must be the index of a column that is a %G_TYPE_POINTER,
 * %G_TYPE_BOXED or %G_TYPE_OBJECT based column.
 *
 * @compare_func will be called with @key as the first parameter and the
 * value from the #GtkListStore row as the second parameter. The third and
 * final parameter is @compare_data.
 *
 * Since: 3.26
 */
void
dzl_gtk_list_store_insert_sorted (GtkListStore     *store,
                                  GtkTreeIter      *iter,
                                  gconstpointer     key,
                                  guint             compare_column,
                                  GCompareDataFunc  compare_func,
                                  gpointer          compare_data)
{
  GValue value = G_VALUE_INIT;
  gpointer (*get_func) (const GValue *) = NULL;
  GtkTreeModel *model = (GtkTreeModel *)store;
  GtkTreeIter begin;
  GtkTreeIter end;
  GtkTreeIter middle;
  guint n_children;
  gint cmpval = 0;
  GType type;

  g_return_if_fail (GTK_IS_LIST_STORE (store));
  g_return_if_fail (GTK_IS_LIST_STORE (model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (compare_column < gtk_tree_model_get_n_columns (GTK_TREE_MODEL (store)));
  g_return_if_fail (compare_func != NULL);

  type = gtk_tree_model_get_column_type (GTK_TREE_MODEL (store), compare_column);

  if (g_type_is_a (type, G_TYPE_POINTER))
    get_func = g_value_get_pointer;
  else if (g_type_is_a (type, G_TYPE_BOXED))
    get_func = g_value_get_boxed;
  else if (g_type_is_a (type, G_TYPE_OBJECT))
    get_func = g_value_get_object;
  else
    {
      g_warning ("%s() only supports pointer, boxed, or object columns",
                 G_STRFUNC);
      gtk_list_store_append (store, iter);
      return;
    }

  /* Try to get the first iter instead of calling n_children to
   * avoid walking the GSequence all the way to the right. If this
   * matches, we know there are some children.
   */
  if (!gtk_tree_model_get_iter_first (model, &begin))
    {
      gtk_list_store_append (store, iter);
      return;
    }

  n_children = gtk_tree_model_iter_n_children (model, NULL);
  if (!gtk_tree_model_iter_nth_child (model, &end, NULL, n_children - 1))
    g_warn_if_reached ();

  middle = begin;

  while (list_store_iter_middle (store, &begin, &end, &middle))
    {
      gtk_tree_model_get_value (model, &middle, compare_column, &value);
      cmpval = compare_func (key, get_func (&value), compare_data);
      g_value_unset (&value);

      if (cmpval == 0 || list_store_iter_equal (&begin, &end))
        break;

      if (cmpval < 0)
        {
          end = middle;

          if (!list_store_iter_equal (&begin, &end) &&
              !gtk_tree_model_iter_previous (model, &end))
            break;
        }
      else if (cmpval > 0)
        {
          begin = middle;

          if (!list_store_iter_equal (&begin, &end) &&
              !gtk_tree_model_iter_next (model, &begin))
            break;
        }
      else
        g_warn_if_reached ();
    }

  if (cmpval < 0)
    gtk_list_store_insert_before (store, iter, &middle);
  else
    gtk_list_store_insert_after (store, iter, &middle);
}

static GtkWidget *
get_parent_or_relative (GtkWidget *widget)
{
  GtkWidget *parent = NULL;

  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_IS_POPOVER (widget))
    parent = gtk_popover_get_relative_to (GTK_POPOVER (widget));
  else if (GTK_IS_WINDOW (widget))
    parent = (GtkWidget *)gtk_window_get_transient_for (GTK_WINDOW (widget));
  else if (GTK_IS_MENU (widget))
    parent = gtk_menu_get_attach_widget (GTK_MENU (widget));

  if (parent == NULL)
    parent = gtk_widget_get_parent (widget);

  return parent;
}

/**
 * dzl_gtk_widget_is_ancestor_or_relative:
 * @widget: a #GtkWidget
 * @ancestor: a #GtkWidget that might be an ancestor
 *
 * This function is like gtk_widget_is_ancestor() except that it checks
 * various relative widgets that are not in the direct hierarchy of
 * widgets. That includes #GtkMenu:attach-widget,
 * #GtkPopover:relative-to, and #GtkWindow:transient-for.
 *
 * Returns: %TRUE if @ancestor is an ancestor or relative for @widget.
 *
 * Since: 3.26
 */
gboolean
dzl_gtk_widget_is_ancestor_or_relative (GtkWidget *widget,
                                        GtkWidget *ancestor)
{
  g_return_val_if_fail (!widget || GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (!ancestor || GTK_IS_WIDGET (ancestor), FALSE);

  if (widget == NULL || ancestor == NULL)
    return FALSE;

  do
    {
      if (widget == ancestor)
        return TRUE;
    }
  while (NULL != (widget = get_parent_or_relative (widget)));

  return FALSE;
}

/**
 * dzl_gtk_widget_get_relative:
 * @widget: a #GtkWidget
 * @relative_type: the type of widget to locate
 *
 * This is similar to gtk_widget_get_ancestor(), but looks for relatives
 * via properties such as #GtkPopover:relative-to and others.
 *
 * Returns: (transfer none) (nullable): A #GtkWidget or %NULL.
 */
GtkWidget *
dzl_gtk_widget_get_relative (GtkWidget *widget,
                             GType      relative_type)
{
  g_return_val_if_fail (!widget || GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (g_type_is_a (relative_type, GTK_TYPE_WIDGET), NULL);

  if (widget == NULL)
    return FALSE;

  do
    {
      if (g_type_is_a (G_OBJECT_TYPE (widget), relative_type))
        return widget;
    }
  while (NULL != (widget = get_parent_or_relative (widget)));

  return NULL;
}
