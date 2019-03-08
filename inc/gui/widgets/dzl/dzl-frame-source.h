/* dzl-frame-source.h
 *
 * Copyright (C) 2010-2016 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DZL_FRAME_SOURCE_H
#define DZL_FRAME_SOURCE_H

#include <glib.h>

G_BEGIN_DECLS

guint dzl_frame_source_add      (guint       frames_per_sec,
                                 GSourceFunc callback,
                                 gpointer    user_data);
guint dzl_frame_source_add_full (gint           priority,
                                 guint          frames_per_sec,
                                 GSourceFunc    callback,
                                 gpointer       user_data,
                                 GDestroyNotify notify);

G_END_DECLS

#endif /* DZL_FRAME_SOURCE_H */
