/* robwidget - gtk2 & GL wrapper
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* GTK PROXY FUNCTIONS */

static gboolean robtk_expose_event(GtkWidget *w, GdkEventExpose *ev, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	cairo_t* cr = gdk_cairo_create(GDK_DRAWABLE(w->window));
	cairo_rectangle_t ea;
	ea.x = ev->area.x; ea.width = ev->area.width;
	ea.y = ev->area.y; ea.height = ev->area.height;

	self->expose_event(self, cr, &ea);
	cairo_destroy (cr);

	return TRUE;
}
static gboolean robtk_mousedown(GtkWidget *w, GdkEventButton *ev, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	RobTkBtnEvent event;
	event.x = ev->x;
	event.y = ev->y;
	event.state = ev->state;
	event.direction = ROBTK_SCROLL_ZERO;
	event.button = ev->button;
	if (self->mousedown(self, &event)) return TRUE;
	return FALSE;
}

static gboolean robtk_mouseup(GtkWidget *w, GdkEventButton *ev, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	RobTkBtnEvent event;
	event.x = ev->x;
	event.y = ev->y;
	event.state = ev->state;
	event.direction = ROBTK_SCROLL_ZERO;
	event.button = ev->button;
	if (self->mouseup(self, &event)) return TRUE;
	return FALSE;
}

static gboolean robtk_mousemove(GtkWidget *w, GdkEventMotion *ev, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	RobTkBtnEvent event;
	event.x = ev->x;
	event.y = ev->y;
	event.state = ev->state;
	event.button = -1;
	event.direction = ROBTK_SCROLL_ZERO;
	if (self->mousemove(self, &event)) return TRUE;
	return FALSE;
}

static gboolean robtk_mousescroll(GtkWidget *w, GdkEventScroll *ev, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	RobTkBtnEvent event;
	event.x = ev->x;
	event.y = ev->y;
	event.state = 0;
	event.button = -1;
	switch (ev->direction) {
		case GDK_SCROLL_UP:
			event.direction = ROBTK_SCROLL_UP;
			break;
		case GDK_SCROLL_DOWN:
			event.direction = ROBTK_SCROLL_DOWN;
			break;
		case GDK_SCROLL_LEFT:
			event.direction = ROBTK_SCROLL_LEFT;
			break;
		case GDK_SCROLL_RIGHT:
			event.direction = ROBTK_SCROLL_RIGHT;
			break;
		default:
			event.direction = ROBTK_SCROLL_ZERO;
			break;
	}
	if (self->mousescroll(self, &event)) return TRUE;
	return FALSE;
}

static void robtk_enter_notify(GtkWidget *w, GdkEvent *event, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	if (self->enter_notify) {
		self->enter_notify(self);
	}
}

static void robtk_leave_notify(GtkWidget *w, GdkEvent *event, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	if (self->leave_notify) {
		self->leave_notify(self);
	}
}

static void robtk_size_request(GtkWidget *w, GtkRequisition *r, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	int x,y;
	x = r->width;
	y = r->height;
	self->size_request(self, &x, &y);
	r->width = x;
	r->height = y;
}

static void robtk_size_allocate(GtkWidget *w, GdkRectangle *r, gpointer handle) {
	RobWidget* self = (RobWidget*)handle;
	self->size_allocate(self, r->width, r->height);
	//if (self->c) { gtk_widget_set_size_request(self->c, r->width, r->height); }
}

/***********************************************************************/

#define GET_HANDLE(HDL) (((RobWidget*)HDL)->self)

#define robwidget_set_expose_event(RW, EVT) { \
	(RW)->expose_event=EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "expose_event", G_CALLBACK (robtk_expose_event), (RW)); \
}

#define robwidget_set_size_request(RW, EVT) { \
	int w,h; \
	EVT(RW, &w, &h); \
	(RW)->size_request = EVT; \
	gtk_drawing_area_size(GTK_DRAWING_AREA(RW->m0), w, h); \
	g_signal_connect (G_OBJECT ((RW)->c), "size-request", G_CALLBACK (robtk_size_request), (RW)); \
}

#define robwidget_set_size_allocate(RW, EVT) { \
	(RW)->size_allocate = EVT; \
	g_signal_connect (G_OBJECT ((RW)->c), "size-allocate", G_CALLBACK (robtk_size_allocate), (RW)); \
}

#define robwidget_set_size_limit(RW, EVT)   { (RW)->size_limit = EVT; }
#define robwidget_set_size_default(RW, EVT) { (RW)->size_default = EVT; }

#define robwidget_set_mousedown(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_BUTTON_PRESS_MASK); \
	(RW)->mousedown=EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "button-press-event", G_CALLBACK (robtk_mousedown), (RW)); \
}

#define robwidget_set_mouseup(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK); \
	(RW)->mouseup=EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "button-release-event", G_CALLBACK (robtk_mouseup), (RW)); \
}

#define robwidget_set_mousemove(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_MASK); \
	(RW)->mousemove=EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "motion-notify-event", G_CALLBACK (robtk_mousemove), (RW)); \
}

#define robwidget_set_mousescroll(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_SCROLL_MASK); \
	(RW)->mousescroll=EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "scroll-event", G_CALLBACK (robtk_mousescroll), (RW)); \
}

#define robwidget_set_enter_notify(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_ENTER_NOTIFY_MASK); \
	(RW)->enter_notify = EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "enter-notify-event", G_CALLBACK (robtk_enter_notify), (RW)); \
}

#define robwidget_set_leave_notify(RW, EVT) { \
	gtk_widget_add_events((RW)->m0, GDK_LEAVE_NOTIFY_MASK); \
	(RW)->leave_notify = EVT; \
	g_signal_connect (G_OBJECT ((RW)->m0), "leave-notify-event", G_CALLBACK (robtk_leave_notify), (RW)); \
}

/*************************************************************/
static RobWidget * robwidget_new(void *handle) {
	RobWidget * rw = (RobWidget *) calloc(1, sizeof(RobWidget));
	rw->self = handle;
	rw->m0 = gtk_drawing_area_new();
	rw->c = gtk_alignment_new(0, .5, 0, 0);
	rw->widget_scale = 1.0;
	gtk_container_add(GTK_CONTAINER(rw->c), rw->m0);
	gtk_widget_set_redraw_on_allocate(rw->m0, TRUE);
	return rw;
}

static void robwidget_make_toplevel(RobWidget *rw, void * const handle) {
  ;
}

static void robwidget_set_area(RobWidget *rw, int x, int y, int w, int h) {
	;
}


static void robwidget_destroy(RobWidget *rw) {
	if (rw->m0) gtk_widget_destroy(rw->m0);
	if (rw->c) gtk_widget_destroy(rw->c);
	free(rw);
}

static void robwidget_set_size(RobWidget *rw, int w, int h) {
	gtk_widget_set_size_request(rw->m0, w, h);
}

static void robwidget_set_alignment(RobWidget *rw, float xalign, float yalign) {
	gtk_alignment_set(GTK_ALIGNMENT(rw->c), xalign, yalign, 0, 0);
	rw->xalign = xalign; rw->yalign = yalign;
}

static void robwidget_resize_toplevel(RobWidget *rw, int w, int h) {
	GtkWidget *tlw = gtk_widget_get_toplevel(rw->c);
	if (tlw) {
		gtk_window_resize (GTK_WINDOW(tlw), w, h);
	}
}

static void robwidget_show(RobWidget *rw, bool resize_window) {
	gtk_widget_show_all(rw->c);
}

static void robwidget_hide(RobWidget *rw, bool resize_window) {
	if (!resize_window) {
		gtk_widget_hide(rw->c);
		return;
	}
#ifdef USE_GTK_RESIZE_HACK
	gint ww,wh;
	GtkWidget *tlw = gtk_widget_get_toplevel(rw->c);
	if (tlw) {
		gtk_window_get_size(GTK_WINDOW(tlw), &ww, &wh);
	}
	gtk_widget_hide(rw->c);
	if (tlw) {
		gtk_window_resize (GTK_WINDOW(tlw), ww, 100);
	}
#else
	gtk_widget_hide(rw->c);
#endif
}

/* HOST PROVIDED FUNCTIONS */
static void queue_draw(RobWidget *h) {
	if (h->m0) gtk_widget_queue_draw(h->m0);
	else gtk_widget_queue_draw(h->c); // cb_preferences's ui->box
}

static void queue_draw_area(RobWidget *rw, int x, int y, int w, int h) {
	GdkRectangle rect;
	if (!rw->m0->window) return;
	rect.x=x; rect.y=y; rect.width=w; rect.height=h;
	GdkRegion *region =  gdk_region_rectangle (&rect);
	gdk_window_invalidate_region (rw->m0->window, region, true);
	gdk_region_destroy(region);
}

static void queue_tiny_area(RobWidget *rw, float x, float y, float w, float h) {
	queue_draw_area(rw, x, y, w, h);
}

static RobWidget* rob_hbox_new(gboolean homogeneous, gint padding) {
	RobWidget * rw = (RobWidget *) calloc(1, sizeof(RobWidget));
	rw->c = gtk_hbox_new(homogeneous, padding);
	return rw;
}

static RobWidget* rob_vbox_new(gboolean homogeneous, gint padding) {
	RobWidget * rw = (RobWidget *) calloc(1, sizeof(RobWidget));
	rw->c = gtk_vbox_new(homogeneous, padding);
	return rw;
}

static RobWidget* rob_table_new(guint rows, guint cols, gboolean homogeneous) {
	RobWidget * rw = (RobWidget *) calloc(1, sizeof(RobWidget));
	rw->c = gtk_table_new(rows, cols, homogeneous);
	return rw;
}

#define RTK_SHRINK GTK_SHRINK
#define RTK_EXPAND GTK_EXPAND
#define RTK_FILL   GTK_FILL
#define RTK_EXANDF (GTK_EXPAND|GTK_FILL)

#define rob_hbox_child_pack(BX,CLD,EXP,FILL) gtk_box_pack_start(GTK_BOX((BX)->c), (CLD)->c, EXP, FILL, 0)
#define rob_vbox_child_pack(BX,CLD,EXP,FILL) gtk_box_pack_start(GTK_BOX((BX)->c), (CLD)->c, EXP, FILL, 0)

#define rob_table_attach(RW, CL, A1, A2, A3, A4, A5, A6, A7, A8) \
	gtk_table_attach(GTK_TABLE((RW)->c), (CL)->c, A1, A2, A3, A4 \
			, (GtkAttachOptions)(A7) \
			, (GtkAttachOptions)(A8) \
			, A5, A6)

#define rob_table_attach_defaults(RW, CL, A1, A2, A3, A4) \
	gtk_table_attach_defaults(GTK_TABLE((RW)->c), (CL)->c, A1, A2, A3, A4)


#define rob_table_destroy(RW) { \
	gtk_widget_destroy((RW)->c); \
	free(RW); \
}

#define rob_box_destroy(RW) { \
	gtk_widget_destroy((RW)->c); \
	free(RW); \
}
