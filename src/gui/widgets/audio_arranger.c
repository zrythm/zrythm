/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "gui/widgets/region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
/*void*/
/*audio_arranger_widget_set_allocation (*/
  /*ArrangerWidget * self,*/
  /*GtkWidget *          widget,*/
  /*GdkRectangle *       allocation)*/
/*{*/
/*}*/

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
/*ArrangerCursor*/
/*audio_arranger_widget_get_cursor (*/
  /*ArrangerWidget * self,*/
  /*UiOverlayAction action,*/
  /*Tool            tool)*/
/*{*/

/*}*/

/*void*/
/*audio_arranger_widget_setup (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*// set the size*/
  /*[>int ww, hh;<]*/
  /*RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);*/
  /*gtk_widget_set_size_request (*/
    /*GTK_WIDGET (self),*/
    /*(int) rw_prv->total_px,*/
    /*-1);*/
/*}*/

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
/*void*/
/*audio_arranger_widget_select (*/
  /*ArrangerWidget * self,*/
  /*double               offset_x,*/
  /*double               offset_y,*/
  /*int                  delete)*/
/*{*/
/*}*/

/**
 * Readd children.
 */
/*void*/
/*audio_arranger_widget_refresh_children (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*[> remove all children except bg <]*/
  /*GList *children, *iter;*/

  /*children =*/
    /*gtk_container_get_children (*/
      /*GTK_CONTAINER (self));*/
  /*for (iter = children;*/
       /*iter != NULL;*/
       /*iter = g_list_next (iter))*/
    /*{*/
      /*GtkWidget * widget = GTK_WIDGET (iter->data);*/
      /*if (widget != (GtkWidget *) ar_prv->bg &&*/
          /*widget != (GtkWidget *) ar_prv->playhead)*/
        /*{*/
          /*g_object_ref (widget);*/
          /*gtk_container_remove (*/
            /*GTK_CONTAINER (self),*/
            /*widget);*/
        /*}*/
    /*}*/
  /*g_list_free (children);*/

  /*if (CLIP_EDITOR->region &&*/
      /*CLIP_EDITOR->region->type == REGION_TYPE_AUDIO)*/
    /*{*/
      /*[>AudioRegion * mr =<]*/
        /*[>(AudioRegion *) CLIP_EDITOR->region;<]*/
    /*}*/
/*}*/

/*static void*/
/*audio_arranger_widget_class_init (*/
  /*ArrangerWidgetClass * klass)*/
/*{*/
/*}*/

/*static void*/
/*audio_arranger_widget_init (*/
  /*ArrangerWidget *self)*/
/*{*/
/*}*/

