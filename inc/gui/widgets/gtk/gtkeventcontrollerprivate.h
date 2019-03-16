/* GTK - The GIMP Toolkit
 * Copyright (C) 2012, One Laptop Per Child.
 * Copyright (C) 2014, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */
#ifndef __GTK_EVENT_CONTROLLER_PRIVATE_H__
#define __GTK_EVENT_CONTROLLER_PRIVATE_H__

//#include "gtkeventcontroller.h"
#include <gtk/gtk.h>

struct _GtkEventController
{
  GObject parent_instance;
};

struct _GtkEventControllerClass
{
  GObjectClass parent_class;

  gboolean (* handle_event) (GtkEventController *controller,
                             const GdkEvent     *event);
  void     (* reset)        (GtkEventController *controller);

  /*<private>*/

  /* Tells whether the event is filtered out, %TRUE makes
   * the event unseen by the handle_event vfunc.
   */
  gboolean (* filter_event) (GtkEventController *controller,
                             const GdkEvent     *event);
  gpointer padding[10];
};

void         gtk_event_controller_set_event_mask (GtkEventController *controller,
						  GdkEventMask        event_mask);
GdkEventMask gtk_event_controller_get_event_mask (GtkEventController *controller);

#endif /* __GTK_EVENT_CONTROLLER_PRIVATE_H__ */
