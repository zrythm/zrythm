#ifndef RTK_STYLE_H
#define RTK_STYLE_H

/* colors */
static const float c_trs[4] = {0.0, 0.0, 0.0, 0.0};
static const float c_blk[4] = {0.0, 0.0, 0.0, 1.0};
static const float c_wht[4] = {1.0, 1.0, 1.0, 1.0};

static const float c_gry[4] = {0.5, 0.5, 0.5, 1.0};
static const float c_g90[4] = {0.9, 0.9, 0.9, 1.0};
static const float c_g80[4] = {0.8, 0.8, 0.8, 1.0}; // dbm gain lines
static const float c_g60[4] = {0.6, 0.6, 0.6, 1.0}; // dpm border
static const float c_g30[4] = {0.3, 0.3, 0.3, 1.0};
static const float c_g20[4] = {0.2, 0.2, 0.2, 1.0};
static const float c_g10[4] = {0.1, 0.1, 0.1, 1.0};
static const float c_g05[4] = {0.05, 0.05, 0.05, 1.0};

static const float c_red[4] = {1.0, 0.0, 0.0, 1.0};
static const float c_rd2[4] = {0.25,0.0, 0.0, 1.0}; // hist-off
static const float c_nrd[4] = {0.9, 0.2, 0.2, 1.0}; // nordicred
static const float c_nvu[4] = {0.9, 0.1, 0.1, 1.0}; // VU red
static const float c_prd[4] = {0.8, 0.2, 0.2, 1.0}; // peak red
static const float c_ptr[4] = {0.6, 0.0, 0.0, 1.0}; // dpm numerical peak bg

static const float c_grn[4] = {0.0, 1.0, 0.0, 1.0};
static const float c_lgg[4] = {0.9, 0.95,0.9, 1.0};
static const float c_ngr[4] = {0.2, 0.9, 0.2, 1.0}; // phasedgreen
static const float c_nyl[4] = {0.9, 0.9, 0.0, 1.0}; // meanyellow
static const float c_ora[4] = {0.8, 0.5, 0.0, 1.0};

static const float c_grb[4] = {0.5, 0.5, 0.6, 1.0};  // goniometer ann
static const float c_glr[4] = {0.7, 0.7, 0.8, 1.0};  // phasemtrann
static const float c_glb[4] = {0.7, 0.7, 0.1, 1.0};  // phasemtr
static const float c_g7X[4] = {0.7, 0.7, 0.7, 0.3};  // radar line

static const float c_an0[4] = {0.6, 0.6, 0.9, 0.75}; // radar annotation
static const float c_an1[4] = {0.5, 0.5, 0.8, 0.75}; // radar annotation

static const float c_gml[4] = {0.88, 0.88, 0.15, 0.6}; // goni lines
static const float c_gmp[4] = {0.88, 0.88, 0.15, 0.7}; // goni points

static const float c_hlt[4] = {1.0, 1.0, 1.0, 0.3}; // dpm highlight
static const float c_xfb[4] = {0.0, 0.0, 0.0, 0.8}; // dpm ann bg
static const float c_scr[4] = {0.2, 0.2, 0.2, 0.8}; // screw mount

#define CairoSetSouerceRGBA(COL) \
	cairo_set_source_rgba (cr, (COL)[0], (COL)[1], (COL)[2], (COL)[3])

#define ISBRIGHT(COL) (COL[0] + COL[1] + COL[2] > 1.5)
#define SHADE_RGB(COL, X) \
	ISBRIGHT(COL) ? COL[0] / (X) : COL[0] * (X), \
	ISBRIGHT(COL) ? COL[1] / (X) : COL[1] * (X), \
	ISBRIGHT(COL) ? COL[2] / (X) : COL[2] * (X)

#endif
