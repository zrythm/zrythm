#define LV2_EXTERNAL_UI_URI "http://lv2plug.in/ns/extensions/ui#external"
#define LV2_EXTERNAL_UI_URI__KX__Host "http://kxstudio.sf.net/ns/lv2ext/external-ui#Host"

/* API/ABI */

#ifdef __cplusplus
extern "C" {
#endif
struct lv2_external_ui {
  void (* run)(struct lv2_external_ui * _this_);
  void (* show)(struct lv2_external_ui * _this_);
  void (* hide)(struct lv2_external_ui * _this_);
	void *self;
};

struct lv2_external_ui_host
{
  void (* ui_closed)(void* controller);
  const char * plugin_human_id;
};
#ifdef __cplusplus
}
#endif
