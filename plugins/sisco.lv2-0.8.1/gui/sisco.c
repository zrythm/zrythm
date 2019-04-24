/* simple scope -- example pipe raw audio data to UI
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

///////////////////////
#define LVGL_RESIZEABLE
#define WITH_TRIGGER
#define WITH_RESAMPLING
#define WITH_MARKERS
#define WITH_AMP_LABEL
#undef  DEBUG_WAVERENDER
#undef  WITH_TIME_ADJ
///////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "../src/uris.h"

#define RTK_URI SCO_URI "#"
#define RTK_GUI "ui"

#ifdef WITH_RESAMPLING
#include "./zita-resampler/resampler.h"
using namespace LV2S;
#endif

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

enum TriggerState {
  TS_DISABLED = 0,
  TS_INITIALIZING,
  TS_WAITMANUAL,
  TS_PREBUFFER,
  TS_TRIGGERED,
  TS_COLLECT,
  TS_END,
  TS_DELAY,
};

/* drawing area size */

#ifdef LVGL_RESIZEABLE
#define DAWIDTH  (ui->w_width)
#define DFLTAMPL (ui->w_amplitude)
#else
#define DAWIDTH (640)
#define DFLTAMPL (200) // default amplitude pixels [-1..+1]
#endif
#define DAHEIGHT (ui->w_height)
#define CHNYPOS(CHN) (robtk_cbtn_get_active(ui->btn_align) ? rintf(.5 * (DAHEIGHT - DFLTAMPL)) : (CHN) * ui->w_chnoff )

#define ANHEIGHT (56)  // annotation footer
#define ANLINE1  (12)
#define ANLINE2  (28)
#define ANLINE3  (44)

#ifdef WITH_AMP_LABEL
#define ANWIDTH  (6 + 10 * ui->n_channels)  // annotation right-side
#define ANLINEL  (10)  // max annotation line length right-side
#else
#define ANWIDTH  (10)  // annotation right-side
#define ANLINEL  (10)  // max annotation line length right-side
#endif

//#define ANRTEXT  (DAWIDTH + ANWIDTH - 20)  // annotation right-aligned text
#define ANRTEXT  (DAWIDTH - 2)  // annotation right-aligned text

/* trigger buffer - 2 * sizeof max audio-buffer size
 * times two becasue with trigger pos at 100% it must be able
 * to hold the prev buffer and the next buffer without
 * overwriting the any data.
 */
#ifdef WITH_RESAMPLING
#define MAX_UPSAMPLING (32)
#define TRBUFSZ  (16384 * MAX_UPSAMPLING)
#else
#define TRBUFSZ  (16384)
#endif

typedef struct {
  float *data_min;
  float *data_max;
  float *data_rms;

  uint32_t idx;
  uint32_t sub;
  uint32_t bufsiz;
  pthread_mutex_t lock;
} ScoChan;

#ifdef WITH_MARKERS
typedef struct {
  uint32_t xpos;
  uint32_t chn;
  float ymin, ymax; // derived
} MarkerX;
#endif

typedef struct {
  LV2_Atom_Forge forge;
  LV2_URID_Map*  map;
  ScoLV2URIs     uris;

  LV2UI_Write_Function write;
  LV2UI_Controller controller;

  RobWidget *hbox, *ctable;

  RobTkSep *sep[3];
  RobWidget *darea;
  RobTkCBtn *btn_pause;
  RobTkCBtn *btn_latch;
  RobTkCBtn *btn_align;
  RobTkLbl  *lbl_amp, *lbl_off_x, *lbl_off_y;
  RobTkCBtn *btn_chn[MAX_CHANNELS];
  RobTkCBtn *btn_mem[MAX_CHANNELS];
  RobTkDial *spb_amp[MAX_CHANNELS];
  RobTkSelect *sel_speed;
  RobTkDial *spb_yoff[MAX_CHANNELS], *spb_xoff[MAX_CHANNELS];
  bool visible[MAX_CHANNELS];

  cairo_surface_t *gridnlabels;
  PangoFontDescription *font[4];

  ScoChan  chn[MAX_CHANNELS];
  ScoChan  mem[MAX_CHANNELS];
  float    xoff[MAX_CHANNELS];
  float    yoff[MAX_CHANNELS];
  float    gain[MAX_CHANNELS];
  bool     hold[MAX_CHANNELS];
  float    grid_spacing;
  uint32_t stride;
  uint32_t stride_vis;
  uint32_t n_channels;
  bool     paused;
  bool     update_ann;
  float    rate;
  uint32_t cur_period;
  bool     error;

#ifdef DEBUG_WAVERENDER
  bool     solidwave;
  RobTkCBtn *btn_solidwave;
#endif
#ifdef WITH_TIME_ADJ
  RobTkSpin     *spb_speed_adj;
#endif

  uint32_t  w_height;
  uint32_t  w_chnoff;

#ifdef WITH_TRIGGER
  RobTkSelect   *sel_trigger_mode;
  RobTkSelect   *sel_trigger_type;
  RobTkPBtn     *btn_trigger_man;
  RobTkSpin     *spb_trigger_lvl;
  RobTkSpin     *spb_trigger_pos;
  RobTkSpin     *spb_trigger_hld;
  RobTkLbl      *lbl_tpos, *lbl_tlvl, *lbl_thld;

  uint32_t trigger_cfg_pos;
  float    trigger_cfg_lvl;
  uint32_t trigger_cfg_channel;

  uint32_t trigger_cfg_mode;
  uint32_t trigger_cfg_type;

  enum TriggerState trigger_state;
  enum TriggerState trigger_state_n;

  ScoChan  trigger_buf[MAX_CHANNELS];
  float    trigger_prev;
  uint32_t trigger_offset;
  uint32_t trigger_delay;
  bool     trigger_collect_ok;
  bool     trigger_manual;
#endif

#ifdef WITH_RESAMPLING
  Resampler *src[MAX_CHANNELS];
  float src_fact;
  float src_fact_vis;
  float src_buf[MAX_CHANNELS][TRBUFSZ]; // TODO dyn alloc
#endif

#ifdef WITH_MARKERS
  MarkerX mrk[2];
  RobTkLbl      *lbl_mlbl0, *lbl_mlbl1, *lbl_mpos0, *lbl_mpos1;
  RobTkLbl      *lbl_mchn0, *lbl_mchn1;
  RobTkDial     *spb_marker_x0, *spb_marker_x1;
  RobTkSpin     *spb_marker_c0, *spb_marker_c1;
  int           dragging_marker;

  RobTkMBtn *btn_ann[MAX_CHANNELS];
  RobWidget *hbx_btn[MAX_CHANNELS];
  int        cann[MAX_CHANNELS];
#endif

#ifdef LVGL_RESIZEABLE
  uint32_t w_width;
  uint32_t w_amplitude;
#endif
} SiScoUI;

static const float color_grd[4] = {0.9, 0.9, 0.0, 0.3};
static const float color_zro[4] = {0.4, 0.4, 0.6, 0.3};
static const float color_trg[4] = {0.1, 0.1, 0.9, 0.9};
static const float color_lvl[4] = {0.5, 0.5, 1.0, 1.0};
static const float color_tbg[4] = {0.0, 0.0, 0.0, 0.7};
static const float color_mrk[4] = {1.0, 1.0, 0.9, 0.9};

static const float color_blk[4] = {0.0, 0.0, 0.0, 1.0};
static const float color_gry[4] = {0.5, 0.5, 0.5, 1.0};
static const float color_wht[4] = {1.0, 1.0, 1.0, 1.0};


static const float color_chn[MAX_CHANNELS][4] = {
  {0.0, 1.0, 0.0, 1.0},
  {1.0, 0.0, 0.0, 1.0},
  {0.0, 0.0, 1.0, 1.0},
  {1.0, 0.0, 1.0, 1.0}
};

static const float color_ann[MAX_CHANNELS][4] = {
  {0.3, 1.0, 0.3, 1.0},
  {1.0, 0.3, 0.3, 1.0},
  {0.3, 0.3, 1.0, 1.0},
  {1.0, 0.3, 1.0, 1.0}
};

static float db_to_coefficient(float v) {
  return pow(10, .05 * v);
}
static float coefficient_to_dB(float v) {
  return 20.0f * log10f (fabsf(v));
}

/* Prototypes */
#ifdef WITH_MARKERS
static void marker_control_sensitivity(SiScoUI* ui, bool en);
#endif


#ifdef WITH_RESAMPLING
/******************************************************************************
 * Setup re-sampling (upsample)
 */
static void setup_src(SiScoUI* ui, float oversample) {
  const int hlen = 16; // 8..96
  const float frel = 1.0; // 1.0 - 2.6 / (float) hlen;
  uint32_t bsiz = 8192;
  float *scratch = (float*) calloc(bsiz, sizeof(float));
  float *resampl = (float*) malloc(bsiz * oversample * sizeof(float));

  ui->src_fact = oversample;

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    if (ui->src[c] != 0) {
      delete ui->src[c];
      ui->src[c] = 0;
    }
    if (oversample <= 1) continue;

    ui->src[c] = new Resampler();
    ui->src[c]->setup(ui->rate, ui->rate * oversample, 1, hlen, frel);

    /* q/d initialize */
    ui->src[c]->inp_count = bsiz;
    ui->src[c]->inp_data = scratch;
    ui->src[c]->out_count = bsiz * oversample;
    ui->src[c]->out_data = resampl;
    ui->src[c]->process ();
  }

  free(scratch);
  free(resampl);
}
#endif


/******************************************************************************
 * Allocate Data structures
 */


static void zero_sco_chan(ScoChan *sc) {
  sc->idx = 0;
  sc->sub = 0;
  memset(sc->data_min, 0, sizeof(float) * sc->bufsiz);
  memset(sc->data_max, 0, sizeof(float) * sc->bufsiz);
  memset(sc->data_rms, 0, sizeof(float) * sc->bufsiz);
}

static void alloc_sco_chan(ScoChan *sc) {
  sc->data_min = (float*) malloc(sizeof(float) * sc->bufsiz);
  sc->data_max = (float*) malloc(sizeof(float) * sc->bufsiz);
  sc->data_rms = (float*) malloc(sizeof(float) * sc->bufsiz);
  zero_sco_chan(sc);
  pthread_mutex_init(&sc->lock, NULL);
}

static void free_sco_chan(ScoChan *sc) {
  pthread_mutex_destroy(&sc->lock);
  free(sc->data_min);
  free(sc->data_max);
  free(sc->data_rms);
}

static void realloc_sco_chan(ScoChan *sc, uint32_t size) {
  free(sc->data_min);
  free(sc->data_max);
  free(sc->data_rms);
  sc->bufsiz = size;
  sc->data_min = (float*) malloc(sizeof(float) * sc->bufsiz);
  sc->data_max = (float*) malloc(sizeof(float) * sc->bufsiz);
  sc->data_rms = (float*) malloc(sizeof(float) * sc->bufsiz);
  zero_sco_chan(sc);
}


#ifdef WITH_TRIGGER
static inline void setup_trigger(SiScoUI* ui) {
  ui->trigger_state_n = TS_INITIALIZING;
}
static inline void next_tigger_state(SiScoUI* ui, enum TriggerState next) {
  if (ui->trigger_state_n == TS_DISABLED || ui->trigger_state == TS_DISABLED) return;
  ui->trigger_state_n = next;
}
#endif


/******************************************************************************
 * Communication with DSP backend -- send/receive settings
 */


/** send current settings to backend */
static void ui_state(LV2UI_Handle handle)
{
  SiScoUI* ui = (SiScoUI*)handle;
  uint8_t obj_buf[4096];
  struct channelstate cs[MAX_CHANNELS];
  const int32_t grid = robtk_select_get_item(ui->sel_speed);
  int32_t misc = 0;

  if (robtk_cbtn_get_active(ui->btn_latch)) {
    misc |= 1;
  }
  if (robtk_cbtn_get_active(ui->btn_align)) {
    misc |= 2;
  }

#ifdef WITH_TRIGGER
  struct triggerstate ts;
  ts.mode = robtk_select_get_item(ui->sel_trigger_mode);
  ts.type = robtk_select_get_item(ui->sel_trigger_type);
  ts.xpos = robtk_spin_get_value(ui->spb_trigger_pos);
  ts.hold = robtk_spin_get_value(ui->spb_trigger_hld);
  ts.level= robtk_spin_get_value(ui->spb_trigger_lvl);
#endif

#ifdef WITH_MARKERS
  struct cursorstate ms;
  ms.xpos[0] = robtk_dial_get_value(ui->spb_marker_x0);
  ms.xpos[1] = robtk_dial_get_value(ui->spb_marker_x1);
  ms.chn[0] = robtk_spin_get_value(ui->spb_marker_c0);
  ms.chn[1] = robtk_spin_get_value(ui->spb_marker_c1);
#endif

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    uint32_t opts = 0;
    if (robtk_cbtn_get_active(ui->btn_chn[c])) opts |= 1;
#ifdef WITH_MARKERS
    opts |= (robtk_mbtn_get_active(ui->btn_ann[c])) << 1;
#endif
    cs[c].gain = db_to_coefficient(robtk_dial_get_value(ui->spb_amp[c]));
    if (robtk_dial_get_state(ui->spb_amp[c]) == 1) cs[c].gain *= -1;
    cs[c].xoff = robtk_dial_get_value(ui->spb_xoff[c]);
    cs[c].yoff = robtk_dial_get_value(ui->spb_yoff[c]);
    cs[c].opts = opts;
  }

  lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 1024);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_frame_time(&ui->forge, 0);
  LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_state);


  lv2_atom_forge_property_head(&ui->forge, ui->uris.ui_state_grid, 0);
  lv2_atom_forge_int(&ui->forge, grid);

  lv2_atom_forge_property_head(&ui->forge, ui->uris.ui_state_misc, 0);
  lv2_atom_forge_int(&ui->forge, misc);

#ifdef WITH_TRIGGER
  lv2_atom_forge_property_head(&ui->forge, ui->uris.ui_state_trig, 0);
  lv2_atom_forge_vector(&ui->forge, sizeof(float), ui->uris.atom_Float,
      sizeof(struct triggerstate) / sizeof(float), &ts);
#endif
#ifdef WITH_MARKERS
  lv2_atom_forge_property_head(&ui->forge, ui->uris.ui_state_curs, 0);
  lv2_atom_forge_vector(&ui->forge, sizeof(int32_t), ui->uris.atom_Int,
      sizeof(struct cursorstate) / sizeof(int32_t), &ms);
#endif
  lv2_atom_forge_property_head(&ui->forge, ui->uris.ui_state_chn, 0);
  lv2_atom_forge_vector(&ui->forge, sizeof(float), ui->uris.atom_Float,
      ui->n_channels * sizeof(struct channelstate) / sizeof(float), cs);

  lv2_atom_forge_pop(&ui->forge, &frame);
  ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

/** notfiy backend that UI is closed */
static void ui_disable(LV2UI_Handle handle)
{
  SiScoUI* ui = (SiScoUI*)handle;
  ui_state(handle);

  uint8_t obj_buf[64];
  lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_frame_time(&ui->forge, 0);
  LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_off);
  lv2_atom_forge_pop(&ui->forge, &frame);
  ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

/** notify backend that UI is active:
 * request state and enable data-transmission */
static void ui_enable(LV2UI_Handle handle)
{
  SiScoUI* ui = (SiScoUI*)handle;
  uint8_t obj_buf[64];
  lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_frame_time(&ui->forge, 0);
  LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_on);
  lv2_atom_forge_pop(&ui->forge, &frame);
  ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

static void apply_state_chn(SiScoUI* ui, LV2_Atom_Vector* vof) {
  if (vof->atom.type != ui->uris.atom_Float) {
    return;
  }
  struct channelstate *cs = (struct channelstate *) LV2_ATOM_BODY(&vof->atom);
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    uint32_t opts = cs[c].opts;
    if (cs[c].gain < 0) {
      robtk_dial_set_state(ui->spb_amp[c], 1);
      robtk_dial_set_value(ui->spb_amp[c], coefficient_to_dB(-cs[c].gain));
    } else {
      robtk_dial_set_state(ui->spb_amp[c], 0);
      robtk_dial_set_value(ui->spb_amp[c], coefficient_to_dB(cs[c].gain));
    }
    robtk_dial_set_value(ui->spb_xoff[c], cs[c].xoff);
    robtk_dial_set_value(ui->spb_yoff[c], cs[c].yoff);
    robtk_cbtn_set_active(ui->btn_chn[c], (opts & 1) ? true: false);
#ifdef WITH_MARKERS
    robtk_mbtn_set_active(ui->btn_ann[c], (opts>>1)&0x3);
#endif
  }
}

#ifdef WITH_TRIGGER
static void apply_state_trig(SiScoUI* ui, LV2_Atom_Vector* vof) {
  if (vof->atom.type != ui->uris.atom_Float) {
    return;
  }
  struct triggerstate *ts = (struct triggerstate *) LV2_ATOM_BODY(&vof->atom);
  robtk_spin_set_value(ui->spb_trigger_lvl, ts->level);
  robtk_spin_set_value(ui->spb_trigger_pos, ts->xpos);
  robtk_spin_set_value(ui->spb_trigger_hld, ts->hold);
  robtk_select_set_item(ui->sel_trigger_type, ts->type);
  robtk_select_set_item(ui->sel_trigger_mode, ts->mode);
}
#endif

#ifdef WITH_MARKERS
static void apply_state_curs(SiScoUI* ui, LV2_Atom_Vector* vof) {
  if (vof->atom.type != ui->uris.atom_Int) {
    return;
  }
  struct cursorstate *cs = (struct cursorstate *) LV2_ATOM_BODY(&vof->atom);
  robtk_dial_set_value(ui->spb_marker_x0, cs->xpos[0]);
  robtk_dial_set_value(ui->spb_marker_x1, cs->xpos[1]);
  robtk_spin_set_value(ui->spb_marker_c0, cs->chn[0]);
  robtk_spin_set_value(ui->spb_marker_c1, cs->chn[1]);
}
#endif

/******************************************************************************
 * WIDGET CALLBACKS
 */

static bool cfg_changed (RobWidget *widget, void* data)
{
  ui_state(data);
  return TRUE;
}

#ifdef DEBUG_WAVERENDER
static bool solidwave_btn_callback (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  ui->solidwave = robtk_cbtn_get_active(ui->btn_solidwave);
  queue_draw(ui->darea);
  return TRUE;
}
#endif

static bool latch_btn_callback (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  bool latched = robtk_cbtn_get_active(ui->btn_latch);
  for (uint32_t c = 1; c < ui->n_channels; ++c) {
    robtk_dial_set_sensitive(ui->spb_amp[c], !latched);
  }
  ui_state(data);
  return TRUE;
}

static bool align_btn_callback (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  bool aligned = robtk_cbtn_get_active(ui->btn_align);
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    robtk_dial_set_sensitive(ui->spb_yoff[c], !aligned);
  }
  ui_state(data);
  ui->update_ann = true;
  queue_draw(ui->darea);
  return TRUE;
}

#ifdef WITH_MARKERS
static bool mrk_changed (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  if (ui->paused
#ifdef WITH_TRIGGER
      || (ui->trigger_state == TS_END && ui->trigger_cfg_mode == 1)
#endif
      ) {
    queue_draw(ui->darea);
  }
  ui_state(ui);
  return TRUE;
}

static RobWidget* mouse_down(RobWidget* handle, RobTkBtnEvent *ev) {
  SiScoUI* ui = (SiScoUI*) GET_HANDLE(handle);
  if (!ui->paused
#ifdef WITH_TRIGGER
      && !(ui->trigger_state == TS_END && ui->trigger_cfg_mode == 1)
#endif
      ) return NULL;

  if (ev->button == 1) {
    robtk_dial_set_value(ui->spb_marker_x0, ev->x);
    ui->dragging_marker = 1;
  } else if (ev->button == 3) {
    robtk_dial_set_value(ui->spb_marker_x1, ev->x);
    ui->dragging_marker = 2;
  } else {
    ui->dragging_marker = 0;
    return NULL;
  }
  return handle;
}

static RobWidget* mouse_up(RobWidget* handle, RobTkBtnEvent *ev) {
  SiScoUI* ui = (SiScoUI*) GET_HANDLE(handle);
  ui->dragging_marker = 0;
  return NULL;
}

static RobWidget* mouse_move(RobWidget* handle, RobTkBtnEvent *ev) {
  SiScoUI* ui = (SiScoUI*) GET_HANDLE(handle);
  if (!ui->paused
#ifdef WITH_TRIGGER
      && !(ui->trigger_state == TS_END && ui->trigger_cfg_mode == 1)
#endif
      ) return NULL;

  switch(ui->dragging_marker) {
    case 1:
      robtk_dial_set_value(ui->spb_marker_x0, ev->x);
      break;
    case 2:
      robtk_dial_set_value(ui->spb_marker_x1, ev->x);
      break;
    default:
      return NULL;
  }
  return handle;
}
#endif

#ifdef WITH_TRIGGER
static bool trigger_btn_callback (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  if (ui->trigger_cfg_mode == 1) {
    ui->trigger_manual = true;
  }
  return TRUE;
}

static bool trigger_sel_callback (RobWidget *widget, void* data)
{
  SiScoUI* ui = (SiScoUI*) data;
  ui->trigger_cfg_mode = robtk_select_get_item(ui->sel_trigger_mode);
  // set widget sensitivity depending on mode
  robtk_pbtn_set_sensitive(ui->btn_trigger_man, ui->trigger_cfg_mode == 1);
  robtk_spin_set_sensitive(ui->spb_trigger_lvl, true);
  ui->trigger_manual = false;

  switch(ui->trigger_cfg_mode) {
    default:
    case 0:
      robtk_cbtn_set_sensitive(ui->btn_pause, true);
      robtk_spin_set_sensitive(ui->spb_trigger_hld, false);
      robtk_spin_set_sensitive(ui->spb_trigger_lvl, false);
      robtk_spin_set_sensitive(ui->spb_trigger_pos, false);
      ui->trigger_state_n = TS_DISABLED;
      ui->update_ann = true;
      ui->stride_vis = ui->stride;
#ifdef WITH_RESAMPLING
      ui->src_fact_vis = ui->src_fact;
#endif
      break;
    case 1:
      robtk_cbtn_set_active(ui->btn_pause, false);
      robtk_cbtn_set_sensitive(ui->btn_pause, false);
      robtk_spin_set_sensitive(ui->spb_trigger_hld, false);
      robtk_spin_set_sensitive(ui->spb_trigger_lvl, true);
      robtk_spin_set_sensitive(ui->spb_trigger_pos, true);
      setup_trigger(ui);
      break;
    case 2:
      robtk_cbtn_set_sensitive(ui->btn_pause, true);
      robtk_spin_set_sensitive(ui->spb_trigger_hld, true);
      robtk_spin_set_sensitive(ui->spb_trigger_lvl, true);
      robtk_spin_set_sensitive(ui->spb_trigger_pos, true);
      setup_trigger(ui);
      break;
  }
#ifdef WITH_MARKERS
  marker_control_sensitivity(ui, false);
#endif

  ui_state(data);
  queue_draw(ui->darea);
  return TRUE;
}
#endif


/******************************************************************************
 * Data Preprocessing and triggering logic
 */


/** parse raw audio data from and prepare for later drawing */
static int process_channel(SiScoUI *ui, ScoChan *chn,
    const size_t n_elem, float const *data,
    uint32_t *idx_start, uint32_t *idx_end)
{
  int overflow = 0;
  *idx_start = chn->idx;
  for (uint32_t i = 0; i < n_elem; ++i) {
    if (data[i] < chn->data_min[chn->idx]) { chn->data_min[chn->idx] = data[i]; }
    if (data[i] > chn->data_max[chn->idx]) { chn->data_max[chn->idx] = data[i]; }
    chn->data_rms[chn->idx] += data[i] * data[i];
    if (++chn->sub >= ui->stride) {
      chn->sub = 0;
      chn->idx = (chn->idx + 1) % chn->bufsiz;
      if (chn->idx == 0) {
	++overflow;
      }
      chn->data_min[chn->idx] =  1.0;
      chn->data_max[chn->idx] = -1.0;
      chn->data_rms[chn->idx] = 0;
    }
  }
  *idx_end = chn->idx;
  return overflow;
}


#ifdef WITH_TRIGGER
static int process_trigger(SiScoUI* ui, uint32_t channel, size_t *n_samples_p, float const *audiobuffer)
{
  size_t n_samples = *n_samples_p;

  if (ui->trigger_state == TS_DISABLED) {
    return 0;
  }

  else if (ui->trigger_state == TS_INITIALIZING) {
#ifdef WITH_MARKERS
    marker_control_sensitivity(ui, ui->paused);
#endif
    if (ui->trigger_cfg_mode == 1) {
      next_tigger_state(ui, TS_WAITMANUAL);
    } else {
      next_tigger_state(ui, TS_PREBUFFER);
    }
    ui->trigger_collect_ok = false;
    zero_sco_chan(&ui->trigger_buf[channel]);
    if (ui->trigger_cfg_mode == 1) {
      zero_sco_chan(&ui->chn[channel]);
    }
    ui->trigger_prev = ui->trigger_cfg_lvl;

    if (channel + 1 == ui->n_channels) {
      queue_draw(ui->darea);
    }
    return -1;
  }

  else if (ui->trigger_state == TS_WAITMANUAL) {
    if (ui->trigger_manual) {
      robtk_pbtn_set_sensitive(ui->btn_trigger_man, false);
      robtk_spin_set_sensitive(ui->spb_trigger_lvl, false);
      ui->trigger_manual = false;
      next_tigger_state(ui, TS_PREBUFFER);
    }
    return -1;
  }

  else if (ui->trigger_state == TS_PREBUFFER) {
    uint32_t idx_start, idx_end;
    idx_start = idx_end = 0;

    int overflow = process_channel(ui, &ui->trigger_buf[channel], n_samples, audiobuffer, &idx_start, &idx_end);
    size_t trigger_scan_start;

    if (channel != ui->trigger_cfg_channel) {
      return -1;
    }

    if (ui->trigger_collect_ok) {
      trigger_scan_start = 0;
    } else if (overflow > 0 || idx_end >= ui->trigger_cfg_pos) {
      ui->trigger_collect_ok = true;
      uint32_t voff = (idx_end + TRBUFSZ - ui->trigger_cfg_pos) % TRBUFSZ;
      assert(n_samples >= voff * ui->stride);
      trigger_scan_start = n_samples - voff * ui->stride;
    } else {
      /* no scan yet, keep buffering */
      return -1;
    }

    const float trigger_lvl = ui->trigger_cfg_lvl;
    if (ui->trigger_cfg_type == 0) {
      // RISING EDGE
      for (uint32_t i = trigger_scan_start; i < n_samples; ++i) {
	if (ui->trigger_prev < trigger_lvl && audiobuffer[i] >= trigger_lvl) {
	  next_tigger_state(ui, TS_TRIGGERED);
	  ui->trigger_offset = idx_start + i / ui->stride;
	  break;
	}
	ui->trigger_prev = audiobuffer[i];
      }
    } else {
      // FALLING EDGE
      for (uint32_t i = trigger_scan_start; i < n_samples; ++i) {
	if (ui->trigger_prev > trigger_lvl && audiobuffer[i] <= trigger_lvl) {
	  next_tigger_state(ui, TS_TRIGGERED);
	  ui->trigger_offset = idx_start + i / ui->stride;
	  break;
	}
	ui->trigger_prev = audiobuffer[i];
      }
    }
    return -1;
  }

  else if (ui->trigger_state == TS_TRIGGERED) {
    ScoChan *chn = &ui->chn[channel];
    ScoChan *tbf = &ui->trigger_buf[channel];
    zero_sco_chan(chn);
    const uint32_t pos = ui->trigger_cfg_pos;

    const uint32_t ofx = ui->trigger_offset % TRBUFSZ;
    const uint32_t exs = (tbf->idx + TRBUFSZ - ofx) % TRBUFSZ;

    // when  i == pos;  then (i+off)%DW == ui->trigger_offset
    const uint32_t off = (ui->trigger_offset + TRBUFSZ - pos) % TRBUFSZ;
    const uint32_t ncp = MIN(DAWIDTH, ui->trigger_cfg_pos + exs + 1);

    for (uint32_t i=0; i < ncp; ++i) {
      chn->data_min[i] = tbf->data_min[(i+off)%TRBUFSZ];
      chn->data_max[i] = tbf->data_max[(i+off)%TRBUFSZ];
      chn->data_rms[i] = tbf->data_rms[(i+off)%TRBUFSZ];
    }
    chn->idx = (ncp + DAWIDTH - 1)%DAWIDTH;
    chn->sub = tbf->sub;

    if (channel + 1 == ui->n_channels) {
      if (ui->stride_vis != ui->stride
#ifdef WITH_RESAMPLING
	  || ui->src_fact_vis != ui->src_fact
#endif
	 ) {
	ui->update_ann = true;
	ui->stride_vis = ui->stride;
#ifdef WITH_RESAMPLING
	ui->src_fact_vis = ui->src_fact;
#endif
      }
      queue_draw(ui->darea);
    }

    if (ncp == DAWIDTH) {
      next_tigger_state(ui, TS_END);
      return -1;
    } else {
      next_tigger_state(ui, TS_COLLECT);
      const size_t max_remain = MIN(n_samples, (DAWIDTH - chn->idx - 1) * ui->stride);
      *n_samples_p = max_remain;
      return 0;
    }
  }

  else if (ui->trigger_state == TS_COLLECT) {
    // limit-audio-data
    ScoChan *chn = &ui->chn[channel];
    const size_t max_remain = MIN(n_samples, (DAWIDTH - chn->idx - 1) * ui->stride);
    if (max_remain < n_samples) {
      next_tigger_state(ui, TS_END);
      queue_draw(ui->darea);
    }
    *n_samples_p = max_remain;
    return 0;
  }

  else if (ui->trigger_state == TS_END) {
    if (ui->trigger_cfg_mode == 2) {
      float holdoff = robtk_spin_get_value(ui->spb_trigger_hld);
      if (holdoff > 0) {
	next_tigger_state(ui, TS_DELAY);
	ui->trigger_delay = ceilf(holdoff * ui->rate / ui->cur_period);
      } else {
	next_tigger_state(ui, TS_INITIALIZING);
      }
    } else if (ui->trigger_cfg_mode == 1) {
      robtk_pbtn_set_sensitive(ui->btn_trigger_man, true);
      robtk_spin_set_sensitive(ui->spb_trigger_lvl, true);
#ifdef WITH_MARKERS
      marker_control_sensitivity(ui, true);
#endif
      if (ui->trigger_manual) {
	robtk_pbtn_set_sensitive(ui->btn_trigger_man, false);
	robtk_spin_set_sensitive(ui->spb_trigger_lvl, false);
	next_tigger_state(ui, TS_INITIALIZING);
      }
    }
    return -1;
  }

  else if (ui->trigger_state == TS_DELAY) {
    if (ui->trigger_delay == 0) {
      next_tigger_state(ui, TS_INITIALIZING);
    }
    return -1;
  }

  else {
    fprintf(stderr, "INVALID Trigger state!\n");
    return -1;
  }

}
#endif

/******************************************************************************
 * Pango / Cairo Rendering, Expose
 */

static void render_text(
    cairo_t* cr,
    const char *txt,
    PangoFontDescription *font,
    const float x, const float y,
    const float ang, const int align,
    const float * const col)
{
  int tw, th;
  cairo_save(cr);

  PangoLayout * pl = pango_cairo_create_layout(cr);
  pango_layout_set_font_description(pl, font);
  pango_layout_set_text(pl, txt, -1);
  pango_layout_get_pixel_size(pl, &tw, &th);
  cairo_translate (cr, x, y);
  if (ang != 0) { cairo_rotate (cr, ang); }
  switch(abs(align)) {
    case 1:
      cairo_translate (cr, -tw, -th/2.0);
      break;
    case 2:
      cairo_translate (cr, -tw/2.0 - 0.5, -th/2.0);
      break;
    case 3:
      cairo_translate (cr, -0.5, -th/2.0);
      break;
    case 4:
      cairo_translate (cr, -tw, -th);
      break;
    case 5:
      cairo_translate (cr, -tw/2.0 - 0.5, -th);
      break;
    case 6:
      cairo_translate (cr, -0.5, -th);
      break;
    case 7:
      cairo_translate (cr, -tw, 0);
      break;
    case 8:
      cairo_translate (cr, -tw/2.0 - 0.5, 0);
      break;
    case 9:
      cairo_translate (cr, -0.5, 0);
      break;
    default:
      break;
  }
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  if (align < 0) {
    CairoSetSouerceRGBA(color_tbg);
    cairo_rectangle (cr, 0, 0, tw, th);
    cairo_fill (cr);
  }
  cairo_set_source_rgba (cr, col[0], col[1], col[2], col[3]);
  pango_cairo_layout_path(cr, pl);
  pango_cairo_show_layout(cr, pl);
  g_object_unref(pl);
  cairo_restore(cr);
  cairo_new_path (cr);
}


static void dial_annotation_val(RobTkDial * d, cairo_t *cr, void *data) {
  SiScoUI* ui = (SiScoUI*) (data);
  char txt[16];
  if (d->click_state == 1) {
    snprintf(txt, 16, "%+5.1f\n dB \u00D8", d->cur);
  } else {
    snprintf(txt, 16, "%+5.1f\n dB", d->cur);
  }

  int tw, th;
  cairo_save(cr);
  PangoLayout * pl = pango_cairo_create_layout(cr);
  pango_layout_set_font_description(pl, ui->font[3]);
  pango_layout_set_text(pl, txt, -1);
  pango_layout_get_pixel_size(pl, &tw, &th);
  cairo_translate (cr, d->w_width - 3, d->w_height / 2.0);
  cairo_translate (cr, -tw - 0.5, -th / 2.0);
  cairo_set_source_rgba (cr, .0, .0, .0, .5);
  rounded_rectangle(cr, -1, -1, tw+3, th+1, 3);
  cairo_fill(cr);
  CairoSetSouerceRGBA(c_wht);
  pango_cairo_layout_path(cr, pl);
  pango_cairo_show_layout(cr, pl);
  g_object_unref(pl);
  cairo_restore(cr);
  cairo_new_path(cr);
}


/** called when backend notifies the UI about sample-rate (SR changes)
 */
static void calc_gridspacing(SiScoUI* ui) {
  /* base-grid: 1 sample / pixel
   * grid-spacing = SR / X; with X so that gs in [40-80] px
   */
  const uint32_t base = ceil(ui->rate / 10000.0) * 200;
  const float grid_spacing = ui->rate / base;
  assert(grid_spacing > 0);
  //fprintf(stderr, "GRID-SPACING: %.1f px\n", grid_spacing);
  ui->grid_spacing = grid_spacing;

  // TODO update ui->sel_speed -
  // remove elements not available w/ current sample-rate * MAX_UPSAMPLING
}

static uint32_t calc_stride(SiScoUI* ui) {
  const float us = robtk_select_get_value(ui->sel_speed);
  float stride = ui->rate * us / (1000000.0 * ui->grid_spacing);
  assert (stride > 0);

#ifdef WITH_TIME_ADJ
  const float adj = robtk_spin_get_value(ui->spb_speed_adj);
  stride +=  stride * .5 * adj;
#endif
  // TODO non-int upsampling?! -- as long a samples are integer and SRC quality is appropriate
#ifdef WITH_RESAMPLING
  int upsample = 1;
  if (stride < 1) {
    upsample = MIN(MAX_UPSAMPLING, rintf(1.0 / stride));
    stride *= upsample;
  }
  if (upsample != ui->src_fact) {
    setup_src(ui, upsample);
  }
#endif
  return MAX(1, rintf(stride));
}

static void update_annotations(SiScoUI* ui) {
  cairo_t *cr;
  ui->update_ann = false;
  if (!ui->gridnlabels) {
    ui->gridnlabels = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
	ANWIDTH + DAWIDTH, ANHEIGHT + DAHEIGHT);
  }
  cr = cairo_create(ui->gridnlabels);
  CairoSetSouerceRGBA(color_blk);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_rectangle (cr, 0, 0, ANWIDTH + DAWIDTH, ANHEIGHT + DAHEIGHT);
  cairo_fill (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* version info */
  render_text(cr, "x42 Scope", ui->font[2],
      DAWIDTH + ANWIDTH - 8, DAHEIGHT + ANHEIGHT * .5,
      1.5 * M_PI, 5, c_g30);

  render_text(cr, VERSION, ui->font[2],
      DAWIDTH + ANWIDTH, DAHEIGHT + ANHEIGHT * .5,
      1.5 * M_PI, 5, color_gry);

  cairo_set_line_width(cr, 1.0);
  CairoSetSouerceRGBA(color_grd);

  int32_t gl = ceil(DAWIDTH / ui->grid_spacing / 2.0);

  /* x-grid */
  for (int32_t i = -gl; i <= gl; ++i) {
    const uint32_t xp = DAWIDTH / 2.0 + ui->grid_spacing * i;
    if (xp > DAWIDTH) continue;
    cairo_move_to(cr, xp - .5, 0);
    cairo_line_to(cr, xp - .5, DAHEIGHT - .5);
    cairo_stroke(cr);
  }

  /* y-grid */
  int32_t gy = ceil(DAHEIGHT / 50.0 / 2.0);
  for (int32_t i = -gy; i <= gy; ++i) {
    const int yp = DAHEIGHT / 2.0 + 50.0 * i;
    if (yp < 0 || yp > (int)DAHEIGHT) continue;
    cairo_move_to(cr, 0, yp - .5);
    cairo_line_to(cr, DAWIDTH, yp - .5);
    cairo_stroke(cr);
  }

  /* x ticks */
  const float y0 = rint(DAHEIGHT / 2.0);
  for (int32_t i = -gl * 5; i <= gl * 5; ++i) {
    if (abs(i)%5 == 0) continue;
    uint32_t xp = DAWIDTH / 2.0 + i * ui->grid_spacing / 5.0;
    if (xp > DAWIDTH) continue;
    cairo_move_to(cr, xp - .5, y0 - 3.0);
    cairo_line_to(cr, xp - .5, y0 + 2.5);
    cairo_stroke(cr);
  }

  /* y ticks */
  const float x0 = rint(DAWIDTH / 2.0);
  for (int32_t i = -gy * 5; i <= gy * 5; ++i) {
    if (abs(i)%5 == 0) continue;
    const int yp = DAHEIGHT / 2.0 + 50.0 * i / 5.0;
    if (yp < 0 || yp > (int)DAHEIGHT) continue;
    cairo_move_to(cr, x0-3.0, yp - .5);
    cairo_line_to(cr, x0+2.5, yp - .5);
    cairo_stroke(cr);
  }

  /* border */
  CairoSetSouerceRGBA(color_gry);
  cairo_move_to(cr, 0, DAHEIGHT - .5);
  cairo_line_to(cr, ANWIDTH + DAWIDTH, DAHEIGHT - .5);
  cairo_stroke(cr);

  cairo_move_to(cr, DAWIDTH -.5 , 0);
  cairo_line_to(cr, DAWIDTH -.5 , DAHEIGHT);
  cairo_stroke(cr);

  /* bottom annotation (grid spacing text)*/
  const float gs_us = (ui->grid_spacing * 1000000.0 * ui->stride_vis / ui->rate
#ifdef WITH_RESAMPLING
      / ui->src_fact_vis
#endif
      );
  char tmp[128];
  if (gs_us >= 900000.0) {
    snprintf(tmp, 128, "Grd: %6.2f  s/grid  (%6.2f Hz)", gs_us / 1000000.0, 1000000.0 / gs_us);
  } else if (gs_us >= 1500.0) {
    snprintf(tmp, 128, "Grd: %6.2f ms/grid  (%6.2f Hz)", gs_us / 1000.0, 1000000.0 / gs_us);
  } else if (gs_us >= 900.0) {
    snprintf(tmp, 128, "Grd: %6.2f ms/grid  (%5.1f kHz)", gs_us / 1000.0, 1000.0 / gs_us);
  } else {
    snprintf(tmp, 128, "Grd: %6.2f \u00b5s/grid  (%5.1f kHz)", gs_us, 1000.0 / gs_us);
  }
  render_text(cr, tmp, ui->font[0],
      16, DAHEIGHT + ANLINE2,
      0, 3, color_wht);

  const float ts_us = gs_us * DAWIDTH / ui->grid_spacing;
  if (ts_us >= 800000.0) {
    snprintf(tmp, 128, "Screen width: %.2f s", ts_us / 1000000.0);
  } else if (ts_us >= 900.0) {
    snprintf(tmp, 128, "Screen width: %.2f ms (%.1f Hz)", ts_us / 1000.0, 1000000.0 / ts_us);
  } else {
    snprintf(tmp, 128, "Screen width: %.2f ms (%4.1f kHz)", ts_us / 1000.0, 1000.0 / ts_us);
  }
  render_text(cr, tmp, ui->font[0],
      16, DAHEIGHT + ANLINE3,
      0, 3, color_wht);

  const float er_us = (ui->stride_vis * 1000000.0 / ui->rate
#ifdef WITH_RESAMPLING
      / ui->src_fact_vis
#endif
      );
  if (er_us >= 900000.0) {
    snprintf(tmp, 128, "Res: %6.2f  s/pixel (%6.2f Hz)", er_us / 1000000.0, 1000000.0 / er_us);
  } else if (er_us >= 900.0) {
    snprintf(tmp, 128, "Res: %6.2f ms/pixel (%6.2f Hz)", er_us / 1000.0, 1000000.0 / er_us);
  } else {
    snprintf(tmp, 128, "Res: %6.2f \u00b5s/pixel (%5.1f kHz)", er_us, 1000.0 / er_us);
  }
  render_text(cr, tmp, ui->font[0],
      16, DAHEIGHT + ANLINE1,
      0, 3, color_wht);

  char offs1[127] = "";
  char offs2[127] = "";

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    if (!ui->visible[c]) continue;
    if (c > 3) break; // XXX TODO > 4 channel ?
    char *ostr = c >= ceilf(ui->n_channels / 2.0) ? offs2 : offs1;
    float xo_us = ui->xoff[c] * er_us;
    if (fabsf(xo_us) >= 900000.0) {
      snprintf(ostr + strlen(ostr), 127 - strlen(ostr), "  c%d: \u0394x=%+6.1f s  \u0394y=%+4.0fpx", c,
	  ui->xoff[c] * er_us / 1000000., ui->yoff[c]);
    } else if (fabsf(xo_us) >= 900.0) {
      snprintf(ostr + strlen(ostr), 127 - strlen(ostr), "  c%d: \u0394x=%+6.1f ms \u0394y=%+4.0fpx", c,
	  ui->xoff[c] * er_us / 1000., ui->yoff[c]);
    } else {
      snprintf(ostr + strlen(ostr), 127 - strlen(ostr), "  c%d: \u0394x=%+6.1f \u00b5s \u0394y=%+4.0fpx", c,
	  ui->xoff[c] * er_us, ui->yoff[c]);
    }
  }

  render_text(cr, offs1, ui->font[0],
      ANRTEXT, DAHEIGHT + ANLINE1,
      0, 1, color_wht);
  render_text(cr, offs2, ui->font[0],
      ANRTEXT, DAHEIGHT + ANLINE2,
      0, 1, color_wht);

  /* limit to right border */
  cairo_rectangle (cr, 0, 0, ANWIDTH + DAWIDTH + .5, DAHEIGHT);
  cairo_clip(cr);

  /* y-scale for each channel in right border */
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    if (!ui->visible[c]) continue;
    const float yoff = ui->yoff[c];
    const float gain = ui->gain[c];
    const float gainL = MIN(1.0, fabsf(gain));
    const float gainU = fabsf(gain);
    float y0 = yoff + CHNYPOS(c) + DFLTAMPL * .5;

    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
    // TODO use fixed alpha pattern & scale
    cairo_pattern_t *cpat = cairo_pattern_create_linear(0, 0, 0, gainU * DFLTAMPL);

    cairo_pattern_add_color_stop_rgba(cpat, 0.0, color_chn[c][0], color_chn[c][1], color_chn[c][2], .1);
    cairo_pattern_add_color_stop_rgba(cpat, 0.2, color_chn[c][0], color_chn[c][1], color_chn[c][2], .2);
    cairo_pattern_add_color_stop_rgba(cpat, 0.5, color_chn[c][0], color_chn[c][1], color_chn[c][2], .4);
    cairo_pattern_add_color_stop_rgba(cpat, 0.8, color_chn[c][0], color_chn[c][1], color_chn[c][2], .2);
    cairo_pattern_add_color_stop_rgba(cpat, 1.0, color_chn[c][0], color_chn[c][1], color_chn[c][2], .1);

    cairo_matrix_t m;
#ifdef WITH_AMP_LABEL
    const int a0 = DAWIDTH + ANWIDTH - 10 * (c+1);
    const int a1 = 10;
#else
    const int a0 = DAWIDTH;
    const int a1 = ANWIDTH;
#endif

    cairo_matrix_init_translate (&m, 0, -y0 + DFLTAMPL * gainU * .5);
    cairo_pattern_set_matrix (cpat, &m);
    cairo_set_source (cr, cpat);
    cairo_rectangle (cr, a0, yoff + CHNYPOS(c) - DFLTAMPL * .5 * (gainU - 1.0),
	a1, DFLTAMPL * gainU + 1.0);

    cairo_fill(cr);
    cairo_pattern_destroy(cpat);

    cairo_set_source_rgba (cr, color_chn[c][0], color_chn[c][1], color_chn[c][2], 1.0);
    int max_points = ceilf(gainL * 5.0) * 2;
    for (int32_t i = -max_points; i <= max_points; ++i) {
      float yp = rintf(y0 + gainU * DFLTAMPL * i * .5 / max_points) - .5;
      int ll;

      if (abs(i) == max_points || i==0) ll = ANLINEL * 3 / 4;
      else if (abs(i) == max_points / 2) ll = ANLINEL * 2 / 4;
      else ll = ANLINEL * 1 / 4;

      cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
      cairo_move_to(cr, DAWIDTH, yp);
      cairo_line_to(cr, DAWIDTH + ll + .5,  yp);
      cairo_stroke (cr);

#ifdef WITH_AMP_LABEL
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      /* add numeric indicators */
      if (gainL < .25 ) {
	continue;
      } else if (gainL < 0.5
	  && !(abs(i) == max_points || i==0)) {
	continue;
      } else if (gainU < 2.5
	  && !(abs(i) == max_points || i==0 || abs(i) == max_points / 2)) {
	continue;
      }

      snprintf(tmp, 128, "%+3.1f", ((gain < 0) ? i : -i) / (float) max_points);
      render_text(cr, tmp, ui->font[2],
	  DAWIDTH + ANWIDTH - c * 10,
	  yp, 1.5 * M_PI, 5, color_ann[c]);
#endif
    }
  }

  cairo_destroy(cr);
}

static void invalidate_ann(SiScoUI* ui, int what)
{
  if (what & 1) {
    queue_draw_area(ui->darea, 0, DAHEIGHT, DAWIDTH, ANHEIGHT);
  }
  if (what & 2) {
    queue_draw_area(ui->darea, DAWIDTH, 0, ANWIDTH, DAHEIGHT);
  }
}

#ifdef WITH_MARKERS
static void marker_control_sensitivity(SiScoUI* ui, bool en) {
  robtk_dial_set_sensitive(ui->spb_marker_x0, en);
  robtk_spin_set_sensitive(ui->spb_marker_c0, en);
  robtk_dial_set_sensitive(ui->spb_marker_x1, en);
  robtk_spin_set_sensitive(ui->spb_marker_c1, en);
  for(uint32_t c = 0 ; c < ui->n_channels; ++c) {
    robtk_mbtn_set_sensitive(ui->btn_ann[c], en);
  }
}

static void update_marker_data(SiScoUI* ui, uint32_t id) {
  MarkerX *mrk = &ui->mrk[id];
  const uint32_t c = mrk->chn;
  int pos = mrk->xpos;

  assert (c >=0 && c <= ui->n_channels);
  assert (pos >=0 && pos < (int)DAWIDTH);

  ScoChan *chn = &ui->chn[c];
  if (ui->hold[c]) chn = &ui->mem[c];

  pos -= rintf(ui->xoff[c]);
  if (pos < 0 || pos >= (int)DAWIDTH || pos == (int)chn->idx) {
    mrk->ymin = NAN;
    mrk->ymax = NAN;
  } else {
    mrk->ymin = chn->data_min[pos];
    mrk->ymax = chn->data_max[pos];
  }
}

#ifdef WITH_MARKERS
static void render_marker(SiScoUI* ui, cairo_t *cr, uint32_t id) {
  if (isnan(ui->mrk[id].ymax) || isnan(ui->mrk[id].ymin)) {
    return;
  }

  char tmp[128];
  const uint32_t c = ui->mrk[id].chn;
  const float yoff = ui->yoff[c];
  const float gain = ui->gain[c];
  const float chn_y_offset = yoff + CHNYPOS(c) + DFLTAMPL * .5f - .5f;
  const float chn_y_scale = DFLTAMPL * .5f * gain;

  float ypos = chn_y_offset - (ui->mrk[id].ymin) * chn_y_scale;
  cairo_move_to(cr, ui->mrk[id].xpos - 5.5, ypos);
  cairo_line_to(cr, ui->mrk[id].xpos + 5.0, ypos);
  cairo_stroke (cr);

  if (ui->stride_vis > 1) {
    ypos = chn_y_offset - (ui->mrk[id].ymax) * chn_y_scale;
    cairo_move_to(cr, ui->mrk[id].xpos - 5.5, ypos);
    cairo_line_to(cr, ui->mrk[id].xpos + 5.0, ypos);
    cairo_stroke (cr);

    snprintf(tmp, 128, "Cursor %d (chn:%d)\nMax: %+5.2f (%.1f dBFS)\nMin: %+5.2f (%.1f dBFS)",
	id+1, c+1,
	ui->mrk[id].ymax, coefficient_to_dB(ui->mrk[id].ymax),
	ui->mrk[id].ymin, coefficient_to_dB(ui->mrk[id].ymin));
  } else {
    assert (ui->mrk[id].ymax == ui->mrk[id].ymin);
    snprintf(tmp, 128, "Cursor %d (chn:%d)\nVal: %+5.2f (%.1f dBFS)",
	id+1, c+1,
	ui->mrk[id].ymin, coefficient_to_dB(ui->mrk[id].ymin));
  }

  int txtypos;
  int txtalign;
  if (id == 0) {
    txtypos  = 10;
    txtalign = (ui->mrk[id].xpos > DAWIDTH / 2) ? 7 : 9;
  } else {
    txtypos  = DAHEIGHT - 10;
    txtalign = (ui->mrk[id].xpos > DAWIDTH / 2) ? 4 : 6;
  }
  int txtxpos = ui->mrk[id].xpos - ((ui->mrk[id].xpos > DAWIDTH / 2) ? 2 : -2);

  render_text(cr, tmp, ui->font[0],
      txtxpos, txtypos, 0, -txtalign, color_wht);
}
#endif

static void render_markers(SiScoUI* ui, cairo_t *cr) {

  ui->mrk[0].xpos = robtk_dial_get_value(ui->spb_marker_x0);
  ui->mrk[0].chn = robtk_spin_get_value(ui->spb_marker_c0) - 1;
  ui->mrk[1].xpos = robtk_dial_get_value(ui->spb_marker_x1);
  ui->mrk[1].chn = robtk_spin_get_value(ui->spb_marker_c1) - 1;

  update_marker_data(ui, 0);
  update_marker_data(ui, 1);

  cairo_set_line_width(cr, 1.0);
  CairoSetSouerceRGBA(color_mrk);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* draw lines first, annotations go on top */
  static const double dashed[] = {1.0};
  cairo_set_dash(cr, dashed, 1, 0);

  cairo_move_to(cr, ui->mrk[0].xpos - .5, 0);
  cairo_line_to(cr, ui->mrk[0].xpos - .5, DAHEIGHT);
  cairo_stroke (cr);

  cairo_set_dash(cr, dashed, 1, 1);

  cairo_move_to(cr, ui->mrk[1].xpos - .5, 0);
  cairo_line_to(cr, ui->mrk[1].xpos - .5, DAHEIGHT);
  cairo_stroke (cr);
  cairo_set_dash(cr, NULL, 0, 0);

  /* render annotations */
  render_marker(ui, cr, 0);
  render_marker(ui, cr, 1);

  const float dt_us = ((float)ui->mrk[1].xpos - (float)ui->mrk[0].xpos) * 1000000.0 * ui->stride_vis / ui->rate
#ifdef WITH_RESAMPLING
      / ui->src_fact_vis
#endif
      ;
  char tmp[256];
  if (fabs(dt_us) >= 900000.0) {
    snprintf(tmp, 128, "Cursor \u0394t=  %.2f s (%.1f Hz)", dt_us / 1000000.0, 1000000.0 / dt_us);
  } else if (fabs(dt_us) >= 900.0) {
    snprintf(tmp, 128, "Cursor \u0394t=  %.1f ms (%.1f Hz)", dt_us / 1000.0, 1000000.0 / dt_us);
  } else {
    snprintf(tmp, 128, "Cursor \u0394t=  %.1f \u00b5s (%.1f kHz)", dt_us, 1000.0 / dt_us);
  }
  render_text(cr, tmp, ui->font[0],
      ANRTEXT, DAHEIGHT + ANLINE3,
      0, 1, color_wht);

  /* P-P & RMS */
  if (ui->mrk[0].xpos != ui->mrk[1].xpos) {
    uint32_t mstart = MIN(ui->mrk[0].xpos, ui->mrk[1].xpos);
    uint32_t mend   = MAX(ui->mrk[0].xpos, ui->mrk[1].xpos);

    cairo_rectangle_t box_rect[MAX_CHANNELS];
    int ybox_cnt = 0;

    for(uint32_t c = 0 ; c < ui->n_channels; ++c) {
      if (!ui->visible[c]) continue;
      if (!ui->cann[c]) continue;
      ScoChan *chn = &ui->chn[c];
      if (ui->hold[c]) chn = &ui->mem[c];
      float d_rms = 0, d_min = 1.0, d_max = -1.0;
      uint32_t d_cnt = 0;

      uint32_t mmstart = MIN(DAWIDTH-1, MAX(0, mstart - ui->xoff[c]));
      uint32_t mmend   = MIN(DAWIDTH-1, MAX(0, mend - ui->xoff[c]));

      for(uint32_t s = mmstart ; s < mmend; ++s) {
	if (s == chn->idx) continue;
	if (chn->data_min[s] < d_min) { d_min = chn->data_min[s]; }
	if (chn->data_max[s] > d_max) { d_max = chn->data_max[s]; }
	d_rms += chn->data_rms[s];
	d_cnt += ui->stride_vis;
      }
      if (d_cnt > 0) {

#if 0 // TODO display # of samples div oversampling (d_cnt / d_src)
	float d_src = 1;
#ifdef WITH_RESAMPLING
	d_src = ui->src_fact_vis;
#endif
#endif
	/* warn if acq point is between start & end */
	char xtra[8] = "";
	if (chn->idx > mstart && chn->idx < mend) {
	  snprintf(xtra, 8, " (*)");
	}
	d_rms = sqrt(2.0 * d_rms / d_cnt);
	float d_abs = MAX(fabsf(d_max), fabsf(d_min));
	snprintf(tmp, 256, "Channel %d%s\nRMS: %5.3f (%.1f dBFS)\nP-P: %5.3f (%+5.2f,%+5.2f)\nAbs: %5.3f (%.1f dBFS)",
	    c + 1, xtra,
	    d_rms, coefficient_to_dB(d_rms),
	    d_max - d_min, d_max, d_min,
	    d_abs, coefficient_to_dB(d_abs));
      } else {
	snprintf(tmp, 256, "Channel %d\nno data available\n", c);
      }

      float txtypos = rint(ui->yoff[c] + CHNYPOS(c) + DFLTAMPL * .5f);
      float txtxpos;
      int txtalign;
      switch (ui->cann[c]) {
	case 3:
	  txtxpos = DAWIDTH - 5;
	  txtalign = 1;
	  break;
	case 2:
	  txtxpos = 5;
	  txtalign = 3;
	  break;
	default:
	case 1:
	  txtxpos = rint(mstart + (mend - mstart) * .5f);
	  txtxpos = MIN(MAX(txtxpos, 85), DAWIDTH - 85);
	  txtalign = 2;
	  break;
      }

      /* highlight area corresponding to the data */
      cairo_save(cr);
      static const double dashed[] = {2.5};
      cairo_set_dash(cr, dashed, 1, 0);
      cairo_move_to(cr, mstart-0.5, txtypos -.5);
      cairo_line_to(cr, mend-0.5, txtypos -.5);

      CairoSetSouerceRGBA(color_blk);
      cairo_set_line_width(cr, 2.25);
      cairo_stroke_preserve (cr);
      cairo_set_line_width(cr, 1.5);
      CairoSetSouerceRGBA(color_ann[c]);
      cairo_stroke(cr);
      cairo_set_dash(cr, NULL, 0, 0);

      /* mark acq point */
      if (chn->idx > mstart && chn->idx < mend) {
	cairo_move_to(cr, chn->idx+4.5, txtypos-5.0);
	cairo_line_to(cr, chn->idx-1.5, txtypos+4.5);

	CairoSetSouerceRGBA(color_blk);
	cairo_set_line_width(cr, 2.0);
	cairo_stroke_preserve (cr);
	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA(color_wht);
	cairo_stroke(cr);

	cairo_move_to(cr, chn->idx+1.5, txtypos-5.0);
	cairo_line_to(cr, chn->idx-4.5, txtypos+4.5);

	CairoSetSouerceRGBA(color_blk);
	cairo_set_line_width(cr, 2.0);
	cairo_stroke_preserve (cr);
	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA(color_wht);
	cairo_stroke (cr);
      }

      /* arrows */
      if (mend - mstart > 13) {
	cairo_set_line_width(cr, 1.75);
	cairo_move_to(cr, mstart-0.5, txtypos);
	cairo_line_to(cr, mstart+7.5, txtypos-3.5);
	cairo_line_to(cr, mstart+7.5, txtypos+3.5);
	cairo_line_to(cr, mstart-0.5, txtypos);

	CairoSetSouerceRGBA(color_blk);
	cairo_stroke_preserve (cr);
	CairoSetSouerceRGBA(color_ann[c]);
	cairo_fill (cr);

	cairo_move_to(cr, mend-0.5, txtypos);
	cairo_line_to(cr, mend-7.5, txtypos-3.5);
	cairo_line_to(cr, mend-7.5, txtypos+3.5);
	cairo_line_to(cr, mend-0.5, txtypos);

	CairoSetSouerceRGBA(color_blk);
	cairo_stroke_preserve (cr);
	CairoSetSouerceRGBA(color_ann[c]);
	cairo_fill (cr);
      }
      cairo_restore(cr);
      txtypos = MIN(MAX(txtypos, 80), DAHEIGHT - 80);

      int tx_w, tx_h;
      get_text_geometry(tmp, ui->font[0], &tx_w, &tx_h);

      switch (ui->cann[c]) {
	case 3:
	  box_rect[ybox_cnt].x = txtxpos - tx_w;
	  break;
	case 2:
	  box_rect[ybox_cnt].x = 5;
	  break;
	default:
	case 1:
	  box_rect[ybox_cnt].x = txtxpos - tx_w * .5;
	  break;
      }
      box_rect[ybox_cnt].y = txtypos;
      box_rect[ybox_cnt].width = tx_w;
      box_rect[ybox_cnt].height = tx_h;

      /* somewhat messy, pragmatic approach to
       * prevent overlap of the info boxes.*/
      uint32_t giveup = DAHEIGHT * 4;
      for (int y = 0; y < ybox_cnt; ++y) {
	if (--giveup == 0) {
	  break;
	}
	if (rect_intersect(&box_rect[y], &box_rect[ybox_cnt])) {
	  int dir = ((box_rect[y].y > DAHEIGHT / 2) ^ (giveup > DAHEIGHT * 2)) ? 1 : -1;
	  txtypos += dir;
	  txtypos = MIN(MAX(txtypos, 80), DAHEIGHT - 80);
	  box_rect[ybox_cnt].y = txtypos;
	  y=-1; continue;
	}
      }
      ybox_cnt++;

      render_text(cr, tmp, ui->font[0],
	  txtxpos, txtypos, 0, -txtalign, color_ann[c]);
    }
  }
}
#endif

/* gdk drawing area draw callback
 * -- this runs in gtk's main thread */
static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev)
{
  SiScoUI* ui = (SiScoUI*) GET_HANDLE(handle);

  if (ui->update_ann) {
    update_annotations(ui);
  }

  /* limit cairo-drawing to widget */
  cairo_rectangle (cr, 0, 0, ANWIDTH + DAWIDTH, ANHEIGHT + DAHEIGHT);
  cairo_clip(cr);

  /* limit cairo-drawing to exposed area */
  cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
  cairo_clip(cr);

  cairo_set_source_surface(cr, ui->gridnlabels, 0, 0);
  cairo_paint (cr);

  if (ui->error) {
    cairo_set_source_rgba (cr, 0.7, 0, 0, .5);
    cairo_rectangle (cr, 0, 0, ANWIDTH + DAWIDTH, ANHEIGHT + DAHEIGHT);
    cairo_fill(cr);
    render_text(cr, "UI is not available.\n\n\n", ui->font[1], DAWIDTH/2, DAHEIGHT/2, 0, 2, color_wht);
    render_text(cr, "The LV2-host does not provide sufficient buffers.", ui->font[1], DAWIDTH/2, DAHEIGHT/2, 0, 2, color_wht);
    render_text(cr, "\n\nPlease ask the author of the LV2-host to support", ui->font[1], DAWIDTH/2, DAHEIGHT/2, 0, 2, color_wht);
    render_text(cr, "\n\n\n\nhttp://lv2plug.in/ns/ext/resize-port#minimumSize", ui->font[1], DAWIDTH/2, DAHEIGHT/2, 0, 2, color_wht);
    return TRUE;
  }

#ifdef WITH_TRIGGER
  if (!ui->paused) // NB. trigger-state shares space w/Marker
  switch(ui->trigger_state) {
    case TS_END:
#ifdef WITH_MARKERS
      if (ui->trigger_cfg_mode != 1)
#endif
      render_text(cr, "Acquisition complete", ui->font[1],
	  ANRTEXT, DAHEIGHT + ANLINE3,
	  0, 1, color_wht);
      break;
    case TS_PREBUFFER:
    case TS_WAITMANUAL:
      render_text(cr, "Waiting for trigger", ui->font[1],
	  ANRTEXT, DAHEIGHT + ANLINE3,
	  0, 1, color_wht);
      break;
    case TS_TRIGGERED:
    case TS_COLLECT:
      render_text(cr, "Triggered", ui->font[1],
	  ANRTEXT, DAHEIGHT + ANLINE3,
	  0, 1, color_wht);
      break;
    case TS_DELAY:
      render_text(cr, "Hold-off", ui->font[1],
	  ANRTEXT, DAHEIGHT + ANLINE3,
	  0, 1, color_wht);
      break;
    default:
      break;
  }
#endif

  cairo_save(cr);
  /* limit cairo-drawing to scope-area */
  cairo_rectangle (cr, 0, 0, DAWIDTH, DAHEIGHT);
  cairo_clip(cr);

  cairo_set_line_width(cr, 1.0);

  for(uint32_t c = 0 ; c < ui->n_channels; ++c) {
    if (!ui->visible[c]) continue;
    const float gain = ui->gain[c];
    const float yoff = ui->yoff[c];
    const float x_offset = rintf(ui->xoff[c]);
    ScoChan *chn = &ui->chn[c];
    if (ui->hold[c]) chn = &ui->mem[c];

    uint32_t start = MAX(MIN(DAWIDTH, ev->x - x_offset), 0);
    uint32_t end   = MAX(MIN(DAWIDTH, ev->x + ev->width - x_offset), 0);

#ifdef WITH_TRIGGER
    if (ui->trigger_cfg_mode > 0) {
      end = MIN(end, ui->chn[c].idx);
    }
#endif

    /* drawing area Y-position of given sample-value
     * note: cairo-pixel at 0 spans -.5 .. +.5, hence (DFLTAMPL / 2.0 -.5)
     * also the cairo Y-axis points upwards
     */
    const float chn_y_offset = yoff + CHNYPOS(c) + DFLTAMPL * .5f - .5f;
    const float chn_y_scale = DFLTAMPL * .5f * gain;
#define CYPOS(VAL) ( chn_y_offset - (VAL) * chn_y_scale )

    cairo_save(cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);

    const float margin = (gain > 0) ? 1.0 : -1.0;
    cairo_rectangle (cr, 0, yoff + CHNYPOS(c) - DFLTAMPL * .5 * (gain - 1.0) - margin,
	DAWIDTH, DFLTAMPL * gain + 2.0 * margin);
    cairo_clip(cr);

    CairoSetSouerceRGBA(color_chn[c]);

    pthread_mutex_lock(&chn->lock);

    float prev_min = 0;
    float prev_max = 0;
#ifdef DEBUG_WAVERENDER
    const float so = ui->solidwave ? -.5 : +.5;
#else
    const float so = -.5;
#endif

    if (start == chn->idx) {
      start++;
    }

    if (start < end && start < DAWIDTH) {
      uint32_t spos = start;
      if (start > 0 && chn->idx + 1 != start) {
	spos = start - 1;
      }
      cairo_move_to(cr, spos - .5 + x_offset, CYPOS(chn->data_max[spos]));
      prev_min = chn->data_min[spos];
      prev_max = chn->data_max[spos];
    }

    for (uint32_t i = start ; i < end; ++i) {
      if (i == chn->idx) {
	prev_min = prev_max = 0;
	cairo_move_to(cr, i + x_offset, CYPOS(0));
	continue;
      }
      // keep in mind:
      // * CYPOS is inverted
      // * lines w/ thickness 1 is from  [x-.5 .. x+.5]
      // TODO, combine paths, remember last move_to
      if (chn->data_min[i] == chn->data_max[i]) {
	cairo_line_to(cr, i -.5  + x_offset, CYPOS(chn->data_min[i]));
	cairo_stroke (cr);
	cairo_move_to(cr, i -.5  + x_offset, CYPOS(chn->data_min[i]));
      } else if (chn->data_min[i] > prev_max) {
	cairo_line_to(cr, i -.75 + x_offset, CYPOS(chn->data_min[i]));
	cairo_line_to(cr, i -.25 + x_offset, CYPOS(chn->data_max[i]));
	cairo_stroke (cr);
	cairo_move_to(cr, i -.25 + x_offset, CYPOS(chn->data_max[i]));
      } else if (chn->data_max[i] < prev_min) {
	cairo_line_to(cr, i -.75 + x_offset, CYPOS(chn->data_max[i]));
	cairo_line_to(cr, i -.25 + x_offset, CYPOS(chn->data_min[i]));
	cairo_stroke (cr);
	cairo_move_to(cr, i -.25 + x_offset, CYPOS(chn->data_min[i]));
      } else if (chn->data_min[i] > prev_min) {
	// could go up+right  -- same as chn->data_min[i] > prev_max
	cairo_line_to(cr, i -.5  + x_offset, CYPOS(chn->data_min[i]));
	cairo_line_to(cr, i +so  + x_offset, CYPOS(chn->data_max[i]));
	cairo_stroke (cr);
	cairo_move_to(cr, i +so  + x_offset, CYPOS(chn->data_max[i]));
      } else {
	// could go down+right  -- same chn->data_max[i] < prev_min
	cairo_line_to(cr, i -.5  + x_offset, CYPOS(chn->data_max[i]));
	cairo_line_to(cr, i +so  + x_offset, CYPOS(chn->data_min[i]));
	cairo_stroke (cr);
	cairo_move_to(cr, i +so  + x_offset, CYPOS(chn->data_min[i]));
      }

      prev_min = chn->data_min[i];
      prev_max = chn->data_max[i];
    }

    /* current position vertical-line */
    if (ui->stride >= ui->rate / 4800.0f || ui->paused || ui->hold[c]) {
      cairo_set_source_rgba (cr, color_chn[c][0], color_chn[c][1], color_chn[c][2], .5);
      cairo_move_to(cr, chn->idx - .5 + x_offset, chn_y_offset - chn_y_scale);
      cairo_line_to(cr, chn->idx - .5 + x_offset, chn_y_offset + chn_y_scale);
      cairo_stroke (cr);
    }
    pthread_mutex_unlock(&chn->lock);

#ifdef WITH_TRIGGER
    if (ui->trigger_cfg_mode > 0 && c == ui->trigger_cfg_channel) {
      CairoSetSouerceRGBA(color_trg);
      static const double dashed[] = {1.5};
      cairo_set_dash(cr, dashed, 1, 0);
      const float xoff = rintf(ui->trigger_cfg_pos + x_offset) - .5f;
      const float yval = CYPOS(ui->trigger_cfg_lvl);

      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_move_to(cr, xoff, chn_y_offset - chn_y_scale);
      cairo_line_to(cr, xoff, chn_y_offset + chn_y_scale);
      cairo_stroke (cr);
      cairo_set_dash(cr, NULL, 0, 0);

      cairo_set_line_width(cr, 1.25);
      CairoSetSouerceRGBA(color_blk);
      cairo_move_to(cr, xoff-3.5, yval-3.5);
      cairo_line_to(cr, xoff+3.5, yval+3.5);
      cairo_move_to(cr, xoff-3.5, yval+3.5);
      cairo_line_to(cr, xoff+3.5, yval-3.5);
      cairo_stroke (cr);

      cairo_set_line_width(cr, 1.0);
      CairoSetSouerceRGBA(color_lvl);
      cairo_move_to(cr, xoff-3.5, yval-3.5);
      cairo_line_to(cr, xoff+3.5, yval+3.5);
      cairo_move_to(cr, xoff-3.5, yval+3.5);
      cairo_line_to(cr, xoff+3.5, yval-3.5);
      cairo_stroke (cr);
    }
#endif
    cairo_restore(cr);

    /* zero scale-line */
    CairoSetSouerceRGBA(color_zro);
    cairo_move_to(cr, 0, yoff + CHNYPOS(c) + DFLTAMPL * .5f - .5);
    cairo_line_to(cr, DAWIDTH, yoff + CHNYPOS(c) + DFLTAMPL * .5f - .5);
    cairo_stroke (cr);
  }

  cairo_restore(cr);

#ifdef WITH_MARKERS
  if (ui->paused
#ifdef WITH_TRIGGER
      || (ui->trigger_state == TS_END && ui->trigger_cfg_mode == 1)
#endif
      ) {
    render_markers(ui, cr);
  }
#endif
  return TRUE;
}


/******************************************************************************/

/** this callback runs in the "communication" thread of the LV2-host
 * -- invoked via port_event(); please see notes there.
 *
 *  it acts as 'glue' between LV2 port_event() and GTK expose_event_callback()
 *
 *  This function processes the raw audio data:
 *  - check for trigger (if any)
 *  - limit data to be processed (ignore excess)
 *  - process data  into the display buffers
 *    (combine multiple samples into min/max)
 *  - calulates the minimal area of the display to be updated
 *    (depending on settings, offset, scale)
 */
static void update_scope_real(SiScoUI* ui, const uint32_t channel, const size_t n_elem, float const * data)
{
  uint32_t idx_start, idx_end; // display pixel start/end
  int overflow = 0; // received more audio-data than display-pixel
  size_t n_samples;
  float const * audiobuffer;
  ScoChan *chn = &ui->chn[channel];

  idx_start = idx_end = 0;
  /* if buffer is larger than display, process only end */
  if (
#ifdef WITH_TRIGGER
      ui->trigger_state == TS_DISABLED &&
#endif
      n_elem / ui->stride >= DAWIDTH
      ) {
    n_samples = DAWIDTH * ui->stride;
    audiobuffer = &data[n_elem - n_samples];
    pthread_mutex_lock(&chn->lock);
    chn->idx=0;
    chn->sub=0;
    chn->data_min[chn->idx] =  1.0;
    chn->data_max[chn->idx] = -1.0;
    chn->data_rms[chn->idx] = 0;
    pthread_mutex_unlock(&chn->lock);
  } else {
    n_samples = n_elem;
    audiobuffer = data;
  }
  assert(n_samples <= n_elem);

  pthread_mutex_lock(&chn->lock);

#ifdef WITH_TRIGGER
  if (process_trigger(ui, channel, &n_samples, audiobuffer) >= 0)
  {
#endif

  /* process this channel's audio-data for display */
  overflow = process_channel(ui, chn, n_samples, audiobuffer, &idx_start, &idx_end);

#ifdef WITH_TRIGGER
  }
#endif

  pthread_mutex_unlock(&chn->lock);

  /* signal gtk's main thread to redraw the widget after the last channel */
  if (channel + 1 == ui->n_channels) {
    if (ui->update_ann) {
      /* redraw annotations and complete widget */
      queue_draw(ui->darea);
    } else if (overflow > 1 || (overflow == 1 && idx_end == idx_start)) {
      /* redraw complete widget */
      queue_draw(ui->darea);
    } else if (idx_end > idx_start) {
      /* redraw area between start -> end pixel */
      for (uint32_t c = 0; c < ui->n_channels; ++c) {
	const float gn = fabsf(ui->gain[c]);
	queue_draw_area(ui->darea, idx_start - 2 + ui->xoff[c],
	    ui->yoff[c] + CHNYPOS(c) - DFLTAMPL * .5 * (gn - 1.0),
	    3 + idx_end - idx_start, DFLTAMPL * gn);
      }
    } else if (idx_end < idx_start) {
      /* wrap-around; redraw area between 0 -> start AND end -> right-end */
      for (uint32_t c = 0; c < ui->n_channels; ++c) {
	const float gn = fabsf(ui->gain[c]);
	queue_draw_area(ui->darea, idx_start - 2 + ui->xoff[c],
	    ui->yoff[c] + CHNYPOS(c) - DFLTAMPL * .5 * (gn - 1.0),
	    3 + DAWIDTH - idx_start, DFLTAMPL * gn);
	queue_draw_area(ui->darea, 0,
	    ui->yoff[c] + CHNYPOS(c) - DFLTAMPL * .5 * (gn - 1.0),
	    idx_end + 1 + ui->xoff[c], DFLTAMPL * gn);
      }
    }

    /* check alignment (x-runs) */
    bool ok = true;
    const uint32_t cbx = ui->chn[0].idx;
#ifdef WITH_TRIGGER
    const uint32_t tbx = ui->trigger_buf[0].idx;
#endif
    for (uint32_t c = 1; c < ui->n_channels; ++c) {
      if (ui->chn[c].idx != cbx
#ifdef WITH_TRIGGER
	  || ui->trigger_buf[c].idx != tbx
#endif
	  ) {
	ok = false;
	break;
      }
    }
    /* reset buffers on x-run */
    if (!ok) {
      fprintf(stderr, "SiSco.lv2 UI: x-run (DSP <> UI comm buffer under/overflow)\n");
      for (uint32_t c = 0; c < ui->n_channels; ++c) {
	pthread_mutex_lock(&ui->chn[c].lock);
	zero_sco_chan(&ui->chn[c]);
#ifdef WITH_TRIGGER
	zero_sco_chan(&ui->trigger_buf[c]);
#endif
	pthread_mutex_unlock(&ui->chn[c].lock);
      }
#ifdef WITH_TRIGGER
      next_tigger_state(ui, TS_INITIALIZING);
#endif
    }
  }
}

/** this callback runs in the "communication" thread of the LV2-host
 * -- invoked via port_event(); please see notes there.
 *
 *  it acts as 'glue' between LV2 port_event() and GTK expose_event_callback()
 *
 *  This is a wrapper, around above 'update_scope_real()' it allows
 *  to update the UI state in sync with the 1st channel and
 *  upsamples the data if neccesary.
 */
static void update_scope(SiScoUI* ui, const uint32_t channel, const size_t n_elem, float const * data)
{
  /* this callback runs in the "communication" thread of the LV2-host
   * usually a g_timeout() at ~25fps
   */
  if (channel > ui->n_channels) {
    return;
  }
  /* update state in sync with 1st channel */
  if (channel == 0) {
    ui->cur_period = n_elem;

    bool paused = robtk_cbtn_get_active(ui->btn_pause);
    if (paused != ui->paused) {
      ui->paused = paused;
#ifdef WITH_MARKERS
      marker_control_sensitivity(ui, paused);
#endif
      queue_draw(ui->darea);
    }

#ifdef WITH_TRIGGER
    if (ui->trigger_state != ui->trigger_state_n) {
      invalidate_ann(ui, 1);
    }

    ui->trigger_state = ui->trigger_state_n;
    if ( paused &&
	(ui->trigger_state == TS_WAITMANUAL || ui->trigger_state == TS_PREBUFFER)) {
      ui->trigger_state_n = ui->trigger_state = TS_DELAY;
      ui->trigger_delay = 0;
    }
    if (ui->trigger_delay > 0) ui->trigger_delay--;

    if (ui->trigger_state < TS_TRIGGERED || ui->trigger_state == TS_END) {
      const uint32_t p_pos = ui->trigger_cfg_pos;
      const uint32_t p_typ = ui->trigger_cfg_channel << 1 | ui->trigger_cfg_type;
      const float    p_lvl = ui->trigger_cfg_lvl;

      ui->trigger_cfg_pos = rintf(DAWIDTH * robtk_spin_get_value(ui->spb_trigger_pos) * .01f);
      ui->trigger_cfg_lvl = robtk_spin_get_value(ui->spb_trigger_lvl);

      const uint32_t type = robtk_select_get_item(ui->sel_trigger_type);
      ui->trigger_cfg_channel = type >> 1;
      ui->trigger_cfg_type = type & 1;

      if (p_typ != type || p_pos != ui->trigger_cfg_pos || p_lvl != ui->trigger_cfg_lvl) {
	if (ui->trigger_state == TS_PREBUFFER) {
	  ui->trigger_state = TS_INITIALIZING;
	  robtk_pbtn_set_sensitive(ui->btn_trigger_man, ui->trigger_cfg_mode == 1);
	}
	queue_draw(ui->darea);
      }
    }
#endif
  }

  const float oxoff = ui->xoff[channel];
  const float oyoff = ui->yoff[channel];
  const float ogain = ui->gain[channel];
  const bool  o_vis = ui->visible[channel];
  const bool  o_mem = ui->hold[channel];
#ifdef WITH_MARKERS
  const int   o_ann = ui->cann[channel];
#endif

  if (robtk_cbtn_get_active(ui->btn_align)) {
    ui->yoff[channel] = 0;
  } else {
    ui->yoff[channel] = DFLTAMPL * .005 * ui->n_channels * robtk_dial_get_value(ui->spb_yoff[channel]);
  }
  ui->xoff[channel] = DAWIDTH * .005 * robtk_dial_get_value(ui->spb_xoff[channel]);
  bool latched = robtk_cbtn_get_active(ui->btn_latch);
  ui->gain[channel] = db_to_coefficient(robtk_dial_get_value(ui->spb_amp[latched ? 0 : channel]));
  if (robtk_dial_get_state(ui->spb_amp[latched ? 0 : channel]) == 1) ui->gain[channel] *= -1;
  ui->visible[channel] = robtk_cbtn_get_active(ui->btn_chn[channel]);
  ui->hold[channel] = robtk_cbtn_get_active(ui->btn_mem[channel]);
#ifdef WITH_MARKERS
  ui->cann[channel] = robtk_mbtn_get_active(ui->btn_ann[channel]);
#endif

  if (   oxoff != ui->xoff[channel]
      || oyoff != ui->yoff[channel]
      || ogain != ui->gain[channel]
      || o_vis != ui->visible[channel]
#ifdef WITH_MARKERS
      || o_ann != ui->cann[channel]
#endif
      ) {
    ui->update_ann = true;
  }

  if (ui->hold[channel] != o_mem) {
    if (ui->hold[channel]) {
      ScoChan *cx = &ui->chn[channel];
      ScoChan *mx = &ui->mem[channel];
      memcpy(mx->data_min, cx->data_min, sizeof(float) * cx->bufsiz);
      memcpy(mx->data_max, cx->data_max, sizeof(float) * cx->bufsiz);
      memcpy(mx->data_rms, cx->data_rms, sizeof(float) * cx->bufsiz);
      mx->idx = cx->idx;
    }
    queue_draw(ui->darea);
  }

  if (ui->paused
#ifdef WITH_TRIGGER
      && ( ui->trigger_state == TS_DISABLED
	|| ui->trigger_state == TS_END
	|| ui->trigger_state == TS_DELAY)
#endif
      ) {
    if (ui->update_ann) {
      queue_draw(ui->darea);
    }
    return;
  }

  /* update time-scale in sync with 1st channel when NOT paused */
  if (channel == 0
#ifdef WITH_TRIGGER
      && !ui->paused
      && ( ui->trigger_state == TS_DISABLED
	|| ui->trigger_state == TS_INITIALIZING

	|| ui->trigger_state == TS_WAITMANUAL
	|| ui->trigger_state == TS_PREBUFFER
	)
#endif
      ) {

#ifdef WITH_RESAMPLING
    uint32_t p_srcfct = ui->src_fact;
#endif
    uint32_t p_stride = ui->stride;
    ui->stride = calc_stride(ui);

#ifdef WITH_TRIGGER
    if (ui->trigger_state == TS_DISABLED)
#endif
    {
      ui->stride_vis = ui->stride;
#ifdef WITH_RESAMPLING
      ui->src_fact_vis = ui->src_fact;
#endif
    }

    if (p_stride != ui->stride
#ifdef WITH_RESAMPLING
	|| p_srcfct != ui->src_fact
#endif
	) {
      ui->update_ann = true;
      for (uint32_t c = 0; c < ui->n_channels; ++c) {
	zero_sco_chan(&ui->mem[c]);
	zero_sco_chan(&ui->chn[c]);
	robtk_cbtn_set_active(ui->btn_mem[channel], false);
      }
#ifdef WITH_TRIGGER
    next_tigger_state(ui, TS_INITIALIZING);
#endif
    }
  }


#ifdef WITH_RESAMPLING
  if (ui->src_fact > 1) {
    ui->src[channel]->inp_count = n_elem;
    ui->src[channel]->inp_data = data;
    ui->src[channel]->out_count = n_elem * ui->src_fact;
    ui->src[channel]->out_data = ui->src_buf[channel];
    ui->src[channel]->process ();
    update_scope_real(ui, channel, n_elem * ui->src_fact, ui->src_buf[channel]);
  } else
#endif
  update_scope_real(ui, channel, n_elem, data);
}

/******************************************************************************
 * RobWidget
 */

#ifdef LVGL_RESIZEABLE
static void
size_request(RobWidget* handle, int *w, int *h) {
  SiScoUI* ui = (SiScoUI*)GET_HANDLE(handle);
  *w = 640 + ANWIDTH;
  *h = MIN(500, 200 * ui->n_channels) + ANHEIGHT;
}

static void
size_allocate(RobWidget* handle, int w, int h) {
  SiScoUI* ui = (SiScoUI*)GET_HANDLE(handle);
  if ((uint32_t)w == ui->w_width + ANWIDTH
      && (uint32_t)h == ui->w_height + ANHEIGHT) {
    robwidget_set_size(ui->darea, w, h);
    return;
  }
  ui->w_width = MIN(16384, w - ANWIDTH);
  ui->w_height = MIN(8192, h - ANHEIGHT);

  ui->w_amplitude = MAX(200, rint(ui->w_height / ui->n_channels / 2) * 2);
  ui->w_chnoff = (ui->n_channels < 2) ? 0 : (ui->w_height - DFLTAMPL) / (ui->n_channels - 1);

  robwidget_set_size(ui->darea, w, h);
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    pthread_mutex_lock(&ui->chn[c].lock);
  }
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    realloc_sco_chan(&ui->chn[c], ui->w_width);
    realloc_sco_chan(&ui->mem[c], ui->w_width);
#ifdef WITH_TRIGGER
    zero_sco_chan(&ui->trigger_buf[c]);
#endif
    robtk_dial_update_range(ui->spb_xoff[c], -100.0, 100.0, 100.0/(float)DAWIDTH);
    robtk_dial_update_range(ui->spb_yoff[c], -100.0, 100.0, 100.0/(float)DFLTAMPL);
  }

#ifdef WITH_TRIGGER
  robtk_spin_update_range(ui->spb_trigger_pos, 0.0, 100.0, 100.0/(float)DAWIDTH);
  ui->trigger_cfg_pos = 0; // 50%
  next_tigger_state(ui, TS_INITIALIZING);
#endif
#ifdef WITH_MARKERS
  robtk_dial_update_range(ui->spb_marker_x0, 0.0, DAWIDTH - 1, 1);
  robtk_dial_update_range(ui->spb_marker_x1, 0.0, DAWIDTH - 1, 1);

  robtk_dial_set_default(ui->spb_marker_x0, DAWIDTH * .25);
  robtk_dial_set_default(ui->spb_marker_x1, DAWIDTH * .75);
  robtk_dial_set_value(ui->spb_marker_x0, DAWIDTH * .25);
  robtk_dial_set_value(ui->spb_marker_x1, DAWIDTH * .75);
#endif

  cairo_surface_destroy(ui->gridnlabels);
  ui->gridnlabels = NULL;
  update_annotations(ui);
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    pthread_mutex_unlock(&ui->chn[c].lock);
  }
}

#else

static void
size_request(RobWidget* handle, int *w, int *h) {
  SiScoUI* ui = (SiScoUI*)GET_HANDLE(handle);
  *w = DAWIDTH + ANWIDTH;
  *h = DAHEIGHT + ANHEIGHT;
}

#endif

#define robtk_dial_new_narrow(min, max, step) \
  robtk_dial_new_with_size(min, max, step, \
      GSP_WIDTH, GSP_HEIGHT, GSP_CX, GSP_CY, GSP_RADIUS)


static RobWidget * toplevel(SiScoUI* ui, void * const top)
{
  /* main widget: layout */
  ui->hbox = rob_hbox_new(FALSE, 2);
  robwidget_make_toplevel(ui->hbox, top);
  ROBWIDGET_SETNAME(ui->hbox, "sisco");

  /* setup UI */
  ui->darea = robwidget_new(ui);
  robwidget_set_alignment(ui->darea, 0, 0);
  robwidget_set_expose_event(ui->darea, expose_event);
  robwidget_set_size_request(ui->darea, size_request);
#ifdef LVGL_RESIZEABLE
  robwidget_set_size_allocate(ui->darea, size_allocate);
#endif
#ifdef WITH_MARKERS
  robwidget_set_mousedown(ui->darea, mouse_down);
  robwidget_set_mousemove(ui->darea, mouse_move);
  robwidget_set_mouseup  (ui->darea, mouse_up);
#endif

  ui->ctable = rob_table_new(/*rows*/7, /*cols*/ 5, FALSE);

  /* widgets */
  ui->lbl_off_x = robtk_lbl_new("X");
  ui->lbl_off_y = robtk_lbl_new("Y ");
  ui->lbl_amp   = robtk_lbl_new("Amp.");

  robtk_lbl_set_alignment(ui->lbl_off_x, 0.5, 0.5);
  robtk_lbl_set_alignment(ui->lbl_off_y, 0.5, 0.5);
  robtk_lbl_set_alignment(ui->lbl_amp, 0, 0.5);

  ui->btn_pause = robtk_cbtn_new("Pause/Freeze", GBT_LED_LEFT, false);
  ui->btn_latch = robtk_cbtn_new("Gang Ampl.", GBT_LED_LEFT, false);
  ui->btn_align = robtk_cbtn_new("Y", GBT_LED_LEFT, false);
  robtk_cbtn_set_color_on(ui->btn_latch, .2, .2, .8);
  robtk_cbtn_set_color_off(ui->btn_latch, .1, .1, .3);
  robtk_cbtn_set_color_on(ui->btn_align, .2, .2, .8);
  robtk_cbtn_set_color_off(ui->btn_align, .1, .1, .3);

  ui->sep[0] = robtk_sep_new(TRUE);
  ui->sep[1] = robtk_sep_new(TRUE);
  ui->sep[2] = robtk_sep_new(TRUE);
  robtk_sep_set_linewidth(ui->sep[0], 0);
  robtk_sep_set_linewidth(ui->sep[1], 0);
  robtk_sep_set_linewidth(ui->sep[2], 0);

#ifdef DEBUG_WAVERENDER
  ui->btn_solidwave = robtk_cbtn_new("SolidWave (debug)", GBT_LED_LEFT, false);
  robtk_cbtn_set_active(ui->btn_solidwave, true);
#endif

#ifdef WITH_TRIGGER
  ui->spb_trigger_lvl     = robtk_spin_new(-1.0, 1.0, 0.01);

  ui->spb_trigger_pos     = robtk_spin_new(0.0, 100.0, 100.0/(float)DAWIDTH);
  ui->spb_trigger_hld     = robtk_spin_new(0.0, 5.0, 0.1);
  ui->btn_trigger_man     = robtk_pbtn_new("Trigger");

  ui->lbl_tpos = robtk_lbl_new("Xpos: ");
  ui->lbl_tlvl = robtk_lbl_new("Level: ");
  ui->lbl_thld = robtk_lbl_new("Hold [s]: ");

  ui->spb_trigger_lvl->dial->displaymode = 4;
  ui->spb_trigger_lvl->dial->dcol[2][0] = .8;
  ui->spb_trigger_lvl->dial->dcol[2][1] = .8;
  ui->spb_trigger_lvl->dial->dcol[2][2] = .8;

  robtk_lbl_set_alignment(ui->lbl_tpos, 1.0, 0.5);
  robtk_lbl_set_alignment(ui->lbl_tlvl, 1.0, 0.5);
  robtk_lbl_set_alignment(ui->lbl_thld, 1.0, 0.5);

  ui->sel_trigger_mode = robtk_select_new();
  robtk_select_add_item(ui->sel_trigger_mode, 0, "No Trigger");
  robtk_select_add_item(ui->sel_trigger_mode, 1, "Manual");
  robtk_select_add_item(ui->sel_trigger_mode, 2, "Continuous");

  ui->sel_trigger_type = robtk_select_new();
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    char tmp[64];
    snprintf(tmp, 64, "Chn %d Rise", c+1);
    robtk_select_add_item(ui->sel_trigger_type, 2*c, tmp);
    snprintf(tmp, 64, "Chn %d Fall", c+1);
    robtk_select_add_item(ui->sel_trigger_type, 2*c+1, tmp);
  }

  robtk_select_set_alignment(ui->sel_trigger_mode, 0, .5);
  robtk_select_set_alignment(ui->sel_trigger_type, 0, .5);

  robtk_pbtn_set_sensitive(ui->btn_trigger_man, false);
  robtk_spin_set_sensitive(ui->spb_trigger_hld, false);
  robtk_spin_set_sensitive(ui->spb_trigger_lvl, false);
  robtk_spin_set_sensitive(ui->spb_trigger_pos, false);

  robwidget_set_alignment(ui->btn_trigger_man->rw, 0.5, 0.5);

  robtk_spin_set_alignment(ui->spb_trigger_lvl, 0.0, 0.5);
  robtk_spin_label_width(ui->spb_trigger_lvl, -1, 0);
  robtk_spin_set_label_pos(ui->spb_trigger_lvl, 2);

  robtk_spin_set_alignment(ui->spb_trigger_pos, 0.0, 0.5);
  robtk_spin_label_width(ui->spb_trigger_pos, -1, -1);
  robtk_spin_set_label_pos(ui->spb_trigger_pos, 0);

  robtk_spin_set_alignment(ui->spb_trigger_hld, 0.0, 0.5);
  robtk_spin_label_width(ui->spb_trigger_hld, -1, 0);
  robtk_spin_set_label_pos(ui->spb_trigger_hld, 2);

  /* values */
  robtk_spin_set_default(ui->spb_trigger_pos, 50);
  robtk_spin_set_value(ui->spb_trigger_pos, 50);

  robtk_spin_set_default(ui->spb_trigger_lvl, 0);
  robtk_spin_set_value(ui->spb_trigger_lvl, 0);

  robtk_spin_set_default(ui->spb_trigger_hld, 0.5);
  robtk_spin_set_value(ui->spb_trigger_hld, 0.5);

  robtk_select_set_item(ui->sel_trigger_mode, 0);
  robtk_select_set_item(ui->sel_trigger_type, 0);
#endif

  ui->sel_speed = robtk_select_new();

  robtk_select_add_item(ui->sel_speed,      50 , " 50 \u00b5s");
  robtk_select_add_item(ui->sel_speed,     100 , "100 \u00b5s");
  robtk_select_add_item(ui->sel_speed,     200 , "200 \u00b5s");
  robtk_select_add_item(ui->sel_speed,     250 , "250 \u00b5s");
  robtk_select_add_item(ui->sel_speed,     500 , "500 \u00b5s");
  robtk_select_add_item(ui->sel_speed,    1000 , "  1 ms");
  robtk_select_add_item(ui->sel_speed,    2000 , "  2 ms");
  robtk_select_add_item(ui->sel_speed,    5000 , "  5 ms");
  robtk_select_add_item(ui->sel_speed,   10000 , " 10 ms");
  robtk_select_add_item(ui->sel_speed,   20000 , " 20 ms");
  robtk_select_add_item(ui->sel_speed,   50000 , " 50 ms");
  robtk_select_add_item(ui->sel_speed,  100000 , "100 ms");
  robtk_select_add_item(ui->sel_speed,  200000 , "200 ms");
  robtk_select_add_item(ui->sel_speed,  500000 , "500 ms");
  robtk_select_add_item(ui->sel_speed, 1000000 , "1 sec");

  robtk_select_set_item(ui->sel_speed, 10);
  robtk_select_set_default_item(ui->sel_speed, 10);

#ifdef WITH_TIME_ADJ
  ui->spb_speed_adj = robtk_spin_new(-1, 1, .02);
  robtk_spin_set_default(ui->spb_speed_adj, 0);
  robtk_spin_set_value(ui->spb_speed_adj, 0);
  robtk_spin_label_width(ui->spb_speed_adj, -1, -1);
  robtk_spin_set_label_pos(ui->spb_speed_adj, 0);
#endif

#ifdef WITH_MARKERS
  if (ui->n_channels > 1) {
    ui->lbl_mlbl0 = robtk_lbl_new("Cursor 1");
    ui->lbl_mpos0 = robtk_lbl_new("Xpos:");
    ui->lbl_mpos1 = robtk_lbl_new("Xpos:");
  } else {
    ui->lbl_mlbl0 = robtk_lbl_new("Cursor");
    ui->lbl_mpos0 = robtk_lbl_new("X1:");
    ui->lbl_mpos1 = robtk_lbl_new("X2:");
  }
  ui->lbl_mlbl1 = robtk_lbl_new("Cursor 2");
  ui->lbl_mchn0 = robtk_lbl_new("Chn:");
  ui->lbl_mchn1 = robtk_lbl_new("Chn:");

  robtk_lbl_set_alignment(ui->lbl_mchn0, 0.5, 0.5);
  robtk_lbl_set_alignment(ui->lbl_mchn1, 0.5, 0.5);

  ui->spb_marker_x0 = robtk_dial_new_narrow(0.0, DAWIDTH - 1, 1);
  ui->spb_marker_x1 = robtk_dial_new_narrow(0.0, DAWIDTH - 1, 1);

  robtk_dial_set_default(ui->spb_marker_x0, DAWIDTH * .25);
  robtk_dial_set_default(ui->spb_marker_x1, DAWIDTH * .75);
  robtk_dial_set_value(ui->spb_marker_x0, DAWIDTH * .25);
  robtk_dial_set_value(ui->spb_marker_x1, DAWIDTH * .75);

  ui->spb_marker_c0 = robtk_spin_new(1.0, MAX(2,ui->n_channels), 1);
  ui->spb_marker_c1 = robtk_spin_new(1.0, MAX(2,ui->n_channels), 1);
  robtk_spin_set_alignment(ui->spb_marker_c0, 0, 0.5);
  robtk_spin_set_alignment(ui->spb_marker_c1, 0, 0.5);
  robtk_spin_label_width(ui->spb_marker_c0, -1, 0);
  robtk_spin_label_width(ui->spb_marker_c1, -1, 0);
  robtk_spin_set_label_pos(ui->spb_marker_c0, 2);
  robtk_spin_set_label_pos(ui->spb_marker_c1, 2);
#endif

  /* LAYOUT */
  int row = 0;

#define TBLADD(WIDGET, X0, X1, Y0, Y1) \
  rob_table_attach(ui->ctable, WIDGET, X0, X1, Y0, Y1, 2, 2, RTK_EXANDF, RTK_SHRINK)

#define TBLATT(WIDGET, X0, X1, Y0, Y1, XX, XY) \
  rob_table_attach(ui->ctable, WIDGET, X0, X1, Y0, Y1, 2, 2, XX, XY)

  TBLADD(robtk_cbtn_widget(ui->btn_pause), 0, 2, row, row+1);

#ifdef WITH_TIME_ADJ
  TBLADD(robtk_spin_widget(ui->spb_speed_adj), 2, 3, row, row+1);
  TBLATT(robtk_select_widget(ui->sel_speed), 3, 5, row, row+1, RTK_EXANDF, RTK_SHRINK);
#else
  TBLATT(robtk_select_widget(ui->sel_speed), 2, 5, row, row+1, RTK_EXANDF, RTK_SHRINK);
#endif
  row++;

#ifdef DEBUG_WAVERENDER
  TBLADD(robtk_cbtn_widget(ui->btn_solidwave), 0, 4, row, row+1);
  row++;
#endif

  TBLATT(robtk_sep_widget(ui->sep[0]), 0, 5, row, row+1, RTK_EXANDF, RTK_EXANDF); row++;

  if (ui->n_channels > 1) {
    TBLADD(robtk_cbtn_widget(ui->btn_latch), 0, 2, row, row+1);
    robwidget_set_alignment(ui->btn_latch->rw, 0, .5);
    TBLADD(robtk_cbtn_widget(ui->btn_align), 3, 4, row, row+1);
    robwidget_set_alignment(ui->btn_align->rw, .5, .5);
  } else {
    TBLADD(robtk_lbl_widget(ui->lbl_off_y), 3, 4, row, row+1);
  }

  TBLADD(robtk_lbl_widget(ui->lbl_off_x), 2, 3, row, row+1);
  TBLADD(robtk_lbl_widget(ui->lbl_amp), 4, 5, row, row+1);
  row++;

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    char tmp[32];
    snprintf(tmp, 32, "C %d", c+1);
    ui->btn_chn[c] = robtk_cbtn_new(tmp, GBT_LED_LEFT, false);
    robtk_cbtn_set_color_on(ui->btn_chn[c], .2, .8, .1);
    robtk_cbtn_set_color_off(ui->btn_chn[c], .1, .3, .1);
    robtk_cbtn_set_active(ui->btn_chn[c], true);
    ui->visible[c] = true;

    ui->btn_mem[c] = robtk_cbtn_new("", GBT_LED_LEFT, false);
    robtk_cbtn_set_active(ui->btn_mem[c], false);
    ui->hold[c] = false;
#ifdef WITH_MARKERS
    const float mbtncolors[12] = {
      0.0, 0.0, 0.0,
      1.0, 1.0, 1.0,
      0.0, 0.0, 1.0,
      0.7, 0.7, 0.0,
    };
    ui->btn_ann[c] = robtk_mbtn_new(4);
    robtk_mbtn_set_active(ui->btn_ann[c], 1);
    ui->cann[c] = 1;
    robtk_mbtn_set_sensitive(ui->btn_ann[c], false);
    robtk_mbtn_set_leds_rgb(ui->btn_ann[c], mbtncolors);

    ui->hbx_btn[c]  = rob_hbox_new(FALSE, 2);
    rob_hbox_child_pack(ui->hbx_btn[c], robtk_cbtn_widget(ui->btn_mem[c]), FALSE, FALSE);
    rob_hbox_child_pack(ui->hbx_btn[c], robtk_mbtn_widget(ui->btn_ann[c]), FALSE, FALSE);
#endif

    ui->spb_yoff[c] = robtk_dial_new_narrow(-100, 100, 100.0/(float)DFLTAMPL);
    ui->spb_xoff[c] = robtk_dial_new_narrow(-100, 100, 100.0/(float)DAWIDTH);
    ui->spb_amp[c]  = robtk_dial_new_with_size(-20.0, 20.0, .01,
	65, GED_HEIGHT, GSP_CX, GED_CY, GED_RADIUS);

    robtk_dial_enable_states(ui->spb_amp[c], 1);
    robtk_dial_set_state_color(ui->spb_amp[c], 1, 1.0, .0, .0, .3);
    robtk_dial_set_default_state(ui->spb_amp[c], 0);
    robtk_dial_set_state(ui->spb_amp[c], 0);
    robtk_dial_annotation_callback(ui->spb_amp[c], dial_annotation_val, ui);
    ui->spb_amp[c]->displaymode = 7;
    ui->spb_amp[c]->dcol[2][0] = color_chn[c][0];
    ui->spb_amp[c]->dcol[2][1] = color_chn[c][1];
    ui->spb_amp[c]->dcol[2][2] = color_chn[c][2];
    ui->spb_amp[c]->dcol[3][0] = .2 + color_chn[c][0] / 2.5;
    ui->spb_amp[c]->dcol[3][1] = .2 + color_chn[c][1] / 2.5;
    ui->spb_amp[c]->dcol[3][2] = .2 + color_chn[c][2] / 2.5;

    robtk_dial_set_default(ui->spb_yoff[c], 0);
    robtk_dial_set_default(ui->spb_xoff[c], 0);
    robtk_dial_set_default(ui->spb_amp[c], 0);

    robtk_dial_set_alignment(ui->spb_amp[c], 0, .5);

    TBLADD(robtk_cbtn_widget(ui->btn_chn[c]), 0, 1, row, row+1);
#ifdef WITH_MARKERS
    TBLADD(ui->hbx_btn[c], 1, 2, row, row+1);
#else
    TBLADD(robtk_cbtn_widget(ui->btn_mem[c]), 1, 2, row, row+1);
#endif
    TBLADD(robtk_dial_widget(ui->spb_xoff[c]), 2, 3, row, row+1);
    TBLADD(robtk_dial_widget(ui->spb_yoff[c]), 3, 4, row, row+1);
    TBLADD(robtk_dial_widget(ui->spb_amp[c]), 4, 5, row, row+1);

    robtk_dial_set_callback(ui->spb_amp[c], cfg_changed, ui);
    robtk_dial_set_callback(ui->spb_yoff[c], cfg_changed, ui);
    robtk_dial_set_callback(ui->spb_xoff[c], cfg_changed, ui);
    row++;
  }

  TBLATT(robtk_sep_widget(ui->sep[2]), 0, 5, row, row+1, RTK_EXANDF, RTK_EXANDF); row++;

#ifdef WITH_MARKERS
  if (ui->n_channels > 1) {
    TBLADD(robtk_lbl_widget(ui->lbl_mlbl0), 0, 1, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mpos0), 1, 2, row, row+1);
    TBLADD(robtk_dial_widget(ui->spb_marker_x0), 2, 3, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mchn0), 3, 4, row, row+1);
    TBLADD(robtk_spin_widget(ui->spb_marker_c0), 4, 5, row, row+1);
    row++;
    TBLADD(robtk_lbl_widget(ui->lbl_mlbl1), 0, 1, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mpos1), 1, 2, row, row+1);
    TBLADD(robtk_dial_widget(ui->spb_marker_x1), 2, 3, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mchn1), 3, 4, row, row+1);
    TBLADD(robtk_spin_widget(ui->spb_marker_c1), 4, 5, row, row+1);
  } else {
    TBLADD(robtk_lbl_widget(ui->lbl_mlbl0), 0, 1, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mpos0), 1, 2, row, row+1);
    TBLADD(robtk_dial_widget(ui->spb_marker_x0), 2, 3, row, row+1);
    TBLADD(robtk_lbl_widget(ui->lbl_mpos1), 3, 4, row, row+1);
    robtk_dial_set_alignment(ui->spb_marker_x1, 0, 0.5);
    TBLADD(robtk_dial_widget(ui->spb_marker_x1), 4, 5, row, row+1);
  }
  row++;
  marker_control_sensitivity(ui, false);
  TBLATT(robtk_sep_widget(ui->sep[1]), 0, 5, row, row+1, RTK_EXANDF, RTK_EXANDF); row++;
#endif


#ifdef WITH_TRIGGER
  TBLATT(robtk_select_widget(ui->sel_trigger_mode), 0, 2, row, row+1, RTK_EXANDF, RTK_SHRINK);

  TBLADD(robtk_lbl_widget(ui->lbl_tlvl), 2, 4, row, row+1);
  TBLADD(robtk_spin_widget(ui->spb_trigger_lvl), 4, 5, row, row+1);

  row++;
  TBLATT(robtk_select_widget(ui->sel_trigger_type), 0, 2, row, row+1, RTK_EXANDF, RTK_SHRINK);

  TBLADD(robtk_lbl_widget(ui->lbl_thld), 2, 4, row, row+1);
  TBLADD(robtk_spin_widget(ui->spb_trigger_hld), 4, 5, row, row+1);

  row++;
  TBLADD(robtk_pbtn_widget(ui->btn_trigger_man), 0, 2, row, row+1);
  TBLADD(robtk_lbl_widget(ui->lbl_tpos), 2, 4, row, row+1);
  TBLADD(robtk_spin_widget(ui->spb_trigger_pos), 4, 5, row, row+1); row++;

#endif

  /* signals */
  robtk_select_set_callback(ui->sel_speed, cfg_changed, ui);
  robtk_cbtn_set_callback(ui->btn_latch, latch_btn_callback, ui);
  robtk_cbtn_set_callback(ui->btn_align, align_btn_callback, ui);

#ifdef DEBUG_WAVERENDER
  robtk_cbtn_set_callback(ui->btn_solidwave, solidwave_btn_callback, ui);
#endif

#ifdef WITH_TRIGGER
  robtk_pbtn_set_callback(ui->btn_trigger_man, trigger_btn_callback, ui);
  robtk_select_set_callback(ui->sel_trigger_mode, trigger_sel_callback, ui);
  robtk_spin_set_callback(ui->spb_trigger_lvl, cfg_changed, ui);
  robtk_spin_set_callback(ui->spb_trigger_pos, cfg_changed, ui);
#endif

#ifdef WITH_MARKERS
  robtk_dial_set_callback(ui->spb_marker_x0, mrk_changed, ui);
  robtk_spin_set_callback(ui->spb_marker_c0, mrk_changed, ui);
  robtk_dial_set_callback(ui->spb_marker_x1, mrk_changed, ui);
  robtk_spin_set_callback(ui->spb_marker_c1, mrk_changed, ui);
#endif

  /* main layout */
  rob_hbox_child_pack(ui->hbox, ui->darea, TRUE, TRUE);
  rob_hbox_child_pack(ui->hbox, ui->ctable, FALSE, FALSE);

  return ui->hbox;
}

/******************************************************************************
 * LV2
 */

static LV2UI_Handle
instantiate(
    void* const               ui_toplevel,
    const LV2UI_Descriptor*   descriptor,
    const char*               plugin_uri,
    const char*               bundle_path,
    LV2UI_Write_Function      write_function,
    LV2UI_Controller          controller,
    RobWidget**               widget,
    const LV2_Feature* const* features)
{
  SiScoUI* ui = (SiScoUI*)calloc(1, sizeof(SiScoUI));

  if (!ui) {
    fprintf(stderr, "SiSco.lv2 UI: out of memory\n");
    return NULL;
  }

  ui->map = NULL;
  *widget = NULL;

  if (!strncmp(plugin_uri, SCO_URI "#Mono", 31 + 5 )) {
    ui->n_channels = 1;
  } else if (!strncmp(plugin_uri, SCO_URI "#Stereo", 31 + 7)) {
    ui->n_channels = 2;
  } else if (!strncmp(plugin_uri, SCO_URI "#3chan", 31 + 6)) {
    ui->n_channels = 3;
  } else if (!strncmp(plugin_uri, SCO_URI "#4chan", 31 + 6)) {
    ui->n_channels = 4;
  } else {
    free(ui);
    return NULL;
  }

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
      ui->map = (LV2_URID_Map*)features[i]->data;
    }
  }

  if (!ui->map) {
    fprintf(stderr, "SiSco.lv2 UI: Host does not support urid:map\n");
    free(ui);
    return NULL;
  }

#ifdef LVGL_RESIZEABLE
  ui->w_width = 640;
  ui->w_amplitude = 200;
  ui->w_height = MIN(500, 200 * ui->n_channels);
#else
  ui->w_height = MIN(500, DFLTAMPL * ui->n_channels);
#endif
  ui->w_chnoff = (ui->n_channels < 2) ? 0 : (ui->w_height - DFLTAMPL) / (ui->n_channels - 1);

  /* initialize private data structure */
  ui->write      = write_function;
  ui->controller = controller;

  ui->stride     = 25;
  ui->paused     = false;
  ui->rate       = 48000;
  ui->error      = false;
#ifdef DEBUG_WAVERENDER
  ui->solidwave  = true;
#endif

#ifdef WITH_MARKERS
  ui->mrk[0].xpos=50;
  ui->mrk[0].chn=0;
  ui->mrk[1].xpos=490;
  ui->mrk[1].chn=0;
  ui->dragging_marker = 0;
#endif

#ifdef WITH_TRIGGER
  ui->trigger_cfg_mode = 0;
  ui->trigger_cfg_type = 0;
  ui->trigger_cfg_channel = 0;
  ui->trigger_cfg_pos = DAWIDTH * .5; // 50%
  ui->trigger_cfg_lvl = 0;

  ui->trigger_state = TS_DISABLED;
  ui->trigger_state_n = TS_DISABLED;

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    ui->trigger_buf[c].bufsiz = TRBUFSZ;
    alloc_sco_chan(&ui->trigger_buf[c]);
  }
#endif
  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    ui->chn[c].bufsiz = DAWIDTH;
    ui->mem[c].bufsiz = DAWIDTH;
    alloc_sco_chan(&ui->chn[c]);
    alloc_sco_chan(&ui->mem[c]);
  }

  map_sco_uris(ui->map, &ui->uris);
  lv2_atom_forge_init(&ui->forge, ui->map);

  *widget = toplevel(ui, ui_toplevel);

  /* On Screen Display -- annotations */
  ui->font[0] = pango_font_description_from_string("Mono 9");
  ui->font[1] = pango_font_description_from_string("Sans 10");
  ui->font[2] = pango_font_description_from_string("Sans 8px");
  ui->font[3] = pango_font_description_from_string("Mono 8");

  calc_gridspacing(ui);
  ui->stride = calc_stride(ui);
#ifdef WITH_RESAMPLING
  ui->src_fact_vis = ui->src_fact;
#endif
  update_annotations(ui);
#ifdef WITH_RESAMPLING
  setup_src(ui, 1);
#endif

  /* send message to DSP backend:
   * enable message transmission & request state
   */
  ui_enable(ui);

  return ui;
}

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
  return LVGL_LAYOUT_TO_FIT;
}

static void
cleanup(LV2UI_Handle handle)
{
  SiScoUI* ui = (SiScoUI*)handle;
  /* send message to DSP backend:
   * save state & disable message transmission
   */
  ui_disable(ui);

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
#ifdef WITH_TRIGGER
    free_sco_chan(&ui->trigger_buf[c]);
#endif
    free_sco_chan(&ui->chn[c]);
    free_sco_chan(&ui->mem[c]);
#ifdef WITH_RESAMPLING
    delete ui->src[c];
#endif
  }
  cairo_surface_destroy(ui->gridnlabels);
  pango_font_description_free(ui->font[0]);
  pango_font_description_free(ui->font[1]);
  pango_font_description_free(ui->font[2]);
  pango_font_description_free(ui->font[3]);


#ifdef WITH_TRIGGER
  robtk_spin_destroy(ui->spb_trigger_lvl);
  robtk_spin_destroy(ui->spb_trigger_pos);
  robtk_spin_destroy(ui->spb_trigger_hld);
  robtk_pbtn_destroy(ui->btn_trigger_man);
  robtk_lbl_destroy(ui->lbl_tpos);
  robtk_lbl_destroy(ui->lbl_tlvl);
  robtk_lbl_destroy(ui->lbl_thld);
  robtk_select_destroy(ui->sel_trigger_mode);
  robtk_select_destroy(ui->sel_trigger_type);
#endif
#ifdef WITH_MARKERS
  robtk_lbl_destroy(ui->lbl_mlbl0);
  robtk_lbl_destroy(ui->lbl_mlbl1);
  robtk_lbl_destroy(ui->lbl_mpos0);
  robtk_lbl_destroy(ui->lbl_mpos1);
  robtk_lbl_destroy(ui->lbl_mchn0);
  robtk_lbl_destroy(ui->lbl_mchn1);
  robtk_dial_destroy(ui->spb_marker_x0);
  robtk_dial_destroy(ui->spb_marker_x1);
  robtk_spin_destroy(ui->spb_marker_c0);
  robtk_spin_destroy(ui->spb_marker_c1);
#endif

  for (uint32_t c = 0; c < ui->n_channels; ++c) {
    robtk_cbtn_destroy(ui->btn_chn[c]);
    robtk_cbtn_destroy(ui->btn_mem[c]);
    robtk_dial_destroy(ui->spb_yoff[c]);
    robtk_dial_destroy(ui->spb_xoff[c]);
    robtk_dial_destroy(ui->spb_amp[c]);
#ifdef WITH_MARKERS
    robtk_mbtn_destroy(ui->btn_ann[c]);
    rob_box_destroy(ui->hbx_btn[c]);
#endif
  }

  robtk_sep_destroy(ui->sep[0]);
  robtk_sep_destroy(ui->sep[1]);
  robtk_sep_destroy(ui->sep[2]);

  robtk_select_destroy(ui->sel_speed);
  robtk_cbtn_destroy(ui->btn_latch);
  robtk_cbtn_destroy(ui->btn_align);
  robtk_cbtn_destroy(ui->btn_pause);

#ifdef DEBUG_WAVERENDER
  robtk_cbtn_destroy(ui->btn_solidwave);
#endif
#ifdef WITH_TIME_ADJ
  robtk_spin_destroy(ui->spb_speed_adj);
#endif

  robtk_lbl_destroy(ui->lbl_off_y);
  robtk_lbl_destroy(ui->lbl_off_x);

  rob_table_destroy(ui->ctable);
  robwidget_destroy(ui->darea);
  rob_box_destroy(ui->hbox);

  free(ui);
}

/** receive data from the DSP-backend.
 *
 * this callback runs in the "communication" thread of the LV2-host
 * jalv and ardour do this via a g_timeout() function at ~25fps
 * g_timeout is the same thread a the UI display (no locking is needed)
 * but the openGL version does not use lv2idle and requires locking.
 *
 * the atom-events from the DSP backend are written into a ringbuffer
 * in the host (in the DSP|jack realtime thread) the host then
 * empties this ringbuffer by sending port_event()s to the UI at some
 * random time later.  When CPU and DSP load are large the host-buffer
 * may overflow and some events may get lost.
 *
 * This thread does is not [usually] the 'drawing' thread (it does not
 * have X11 or gl context).
 */
static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
  if (port_index != 1) return;
  SiScoUI* ui = (SiScoUI*)handle;
  LV2_Atom* atom = (LV2_Atom*)buffer;

  /* check type of data received
   *  format == 0: [float] control-port event
   *  format > 0: message
   *  Every event message is sent as separate port-event
   */
  if (format == ui->uris.atom_eventTransfer
      && ( atom->type == ui->uris.atom_Blank || atom->type == ui->uris.atom_Object)
      )
  {
    /* cast the buffer to Atom Object */
    LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
    LV2_Atom *a0 = NULL;
    LV2_Atom *a1 = NULL;
    LV2_Atom *a2 = NULL;
    LV2_Atom *a3 = NULL;
    LV2_Atom *a4 = NULL;
    LV2_Atom *a5 = NULL;
    if (
	/* handle raw-audio data objects */
	obj->body.otype == ui->uris.rawaudio
	/* retrieve properties from object and
	 * check that there the [here] two required properties are set.. */
	&& 2 == lv2_atom_object_get(obj, ui->uris.channelid, &a0, ui->uris.audiodata, &a1, NULL)
	/* ..and non-null.. */
	&& a0
	&& a1
	/* ..and match the expected type */
	&& a0->type == ui->uris.atom_Int
	&& a1->type == ui->uris.atom_Vector
	)
    {
      /* single integer value can be directly dereferenced */
      const int32_t chn = ((LV2_Atom_Int*)a0)->body;

      /* dereference and typecast vector pointer */
      LV2_Atom_Vector* vof = (LV2_Atom_Vector*)LV2_ATOM_BODY(a1);
      /* check if atom is indeed a vector of the expected type*/
      if (vof->atom.type == ui->uris.atom_Float) {
	/* get number of elements in vector
	 * = (raw 8bit data-length - header-length) / sizeof(expected data type:float) */
	const size_t n_elem = (a1->size - sizeof(LV2_Atom_Vector_Body)) / vof->atom.size;
	/* typecast, dereference pointer to vector */
	const float *data = (float*) LV2_ATOM_BODY(&vof->atom);
	/* call function that handles the actual data */
	update_scope(ui, chn, n_elem, data);
      }
    }
    else if (
	/* handle 'state/settings' data object */
	obj->body.otype == ui->uris.ui_state
	/* retrieve properties from object and
	 * check that there the [here] three required properties are set.. */
	&& 0 < lv2_atom_object_get(obj,
	  ui->uris.ui_state_chn, &a0,
	  ui->uris.ui_state_grid, &a1,
	  ui->uris.ui_state_trig, &a2,
	  ui->uris.ui_state_misc, &a4,
	  ui->uris.ui_state_curs, &a5,
	  ui->uris.samplerate, &a3, NULL)
	)
    {
      if (a0 && a0->type == ui->uris.atom_Vector) {
	apply_state_chn(ui, (LV2_Atom_Vector*)LV2_ATOM_BODY(a0));
      }
      if (a3 && a3->type == ui->uris.atom_Float) {
	float rate = ((LV2_Atom_Float*)a3)->body;
	if (rate > 0) {
	  ui->rate = ((LV2_Atom_Float*)a3)->body;
	  ui->error = false;
	} else {
	  ui->error = true;
	  queue_draw(ui->darea);
	}
      }
      if (a1 && a1->type == ui->uris.atom_Int) {
	const int32_t grid = ((LV2_Atom_Int*)a1)->body;
	robtk_select_set_item(ui->sel_speed, grid);
      }
      if (a4 && a4->type == ui->uris.atom_Int) {
	const int32_t misc = ((LV2_Atom_Int*)a4)->body;
	robtk_cbtn_set_active(ui->btn_latch, 1 == (misc & 1));
	robtk_cbtn_set_active(ui->btn_align, 2 == (misc & 2));
      }

#ifdef WITH_TRIGGER
      if (a2 && a2->type == ui->uris.atom_Vector) {
	apply_state_trig(ui, (LV2_Atom_Vector*)LV2_ATOM_BODY(a2));
      }
#endif
#ifdef WITH_MARKERS
      if (a5 && a5->type == ui->uris.atom_Vector) {
	apply_state_curs(ui, (LV2_Atom_Vector*)LV2_ATOM_BODY(a5));
      }
#endif
      /* re-draw grid -- rate may have changed */
      calc_gridspacing(ui);
      ui->update_ann=true;
    }
  }
}

static const void*
extension_data(const char* uri)
{
  return NULL;
}

/* vi:set ts=8 sts=2 sw=2: */
