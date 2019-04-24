#ifdef XTERNAL_UI

#ifdef USE_GUI_THREAD
static int idle(LV2UI_Handle handle);
#endif
static int process_gui_events(LV2UI_Handle handle);

static void x_run (struct lv2_external_ui * handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle->self;
#ifdef USE_GUI_THREAD
	idle(self);
#else
	process_gui_events(self);
#endif
	if (self->close_ui && self->ui_closed) {
		self->close_ui = FALSE;
#ifndef USE_GUI_THREAD
		ui_disable(self->ui);
		puglHideWindow(self->view);
#else
		self->ui_queue_puglXWindow = -1;
#endif
		self->ui_closed(self->controller);
	}
}

static void x_show (struct lv2_external_ui * handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle->self;
#ifndef USE_GUI_THREAD
	puglShowWindow(self->view);
	ui_enable(self->ui);
#else
	self->ui_queue_puglXWindow = 1;
#endif
}

static void x_hide (struct lv2_external_ui * handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle->self;
#ifndef USE_GUI_THREAD
	ui_disable(self->ui);
	puglHideWindow(self->view);
#else
	self->ui_queue_puglXWindow = -1;
#endif
}
#endif
