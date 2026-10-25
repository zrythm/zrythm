/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Triple Synth"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn triple_synth -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __triple_synth_H__
#define  __triple_synth_H__

/************************************************************************
 IMPORTANT NOTE : this file contains two clearly delimited sections :
 the ARCHITECTURE section (in two parts) and the USER section.
 The ARCHITECTURE section is licensed under the Zrythm license
 (LicenseRef-ZrythmLicense). The USER section is replaced by
 Faust-generated code, whose license follows the corresponding .dsp file.
 *************************************************************************/

/******************* BEGIN ARCHITECTURE SECTION (part 1/2) ****************/

// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense


#include "plugins/faust/faust_base.h"

// The generated code references the base classes unqualified
using zrythm::plugins::faust::Meta;
using zrythm::plugins::faust::dsp;
using zrythm::plugins::faust::Soundfile;
using zrythm::plugins::faust::UI;

/******************** END ARCHITECTURE SECTION (part 1/2) *****************/

/************************** BEGIN USER SECTION ****************************/

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>
#ifndef FAUST_INT_WRAP
#define FAUST_INT_WRAP
inline int faust_wrap_add(int a, int b) { return int((unsigned int)a + (unsigned int)b); }
inline int faust_wrap_sub(int a, int b) { return int((unsigned int)a - (unsigned int)b); }
inline int faust_wrap_mul(int a, int b) { return int((unsigned int)a * (unsigned int)b); }
#endif


#ifndef FAUSTCLASS 
#define FAUSTCLASS triple_synth
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

class triple_synthSIG0 {
	
  private:
	
	int iVec1[2];
	int iRec1[2];
	int fSampleRate;
	
  public:
	
	int getNumInputstriple_synthSIG0() {
		return 0;
	}
	int getNumOutputstriple_synthSIG0() {
		return 1;
	}
	
	void instanceInittriple_synthSIG0(int sample_rate) {
		fSampleRate = sample_rate;
		for (int l2 = 0; l2 < 2; l2 = faust_wrap_add(l2, 1)) {
			iVec1[l2] = 0;
		}
		for (int l3 = 0; l3 < 2; l3 = faust_wrap_add(l3, 1)) {
			iRec1[l3] = 0;
		}
	}
	
	void filltriple_synthSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = faust_wrap_add(i1, 1)) {
			iVec1[0] = 1;
			iRec1[0] = (faust_wrap_add(iVec1[1], iRec1[1])) % 65536;
			table[i1] = std::sin(9.58738e-05f * static_cast<float>(iRec1[0]));
			iVec1[1] = iVec1[0];
			iRec1[1] = iRec1[0];
		}
	}

};

static triple_synthSIG0* newtriple_synthSIG0() { return (triple_synthSIG0*)new triple_synthSIG0(); }
static void deletetriple_synthSIG0(triple_synthSIG0* dsp) { delete dsp; }

static float ftbl0triple_synthSIG0[65536];
static float triple_synth_faustpower2_f(float value) {
	return value * value;
}

class triple_synth : public dsp {
	
 private:
	
	int iVec0[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec0[2];
	FAUSTFLOAT fHslider1;
	float fRec3[2];
	float fConst3;
	float fRec2[2];
	float fConst4;
	float fConst5;
	float fConst6;
	float fConst7;
	float fConst8;
	float fConst9;
	float fConst10;
	FAUSTFLOAT fHslider2;
	float fRec7[2];
	float fConst11;
	FAUSTFLOAT fHslider3;
	float fRec8[2];
	FAUSTFLOAT fHslider4;
	float fRec10[2];
	FAUSTFLOAT fButton0;
	float fVec2[2];
	int iRec11[2];
	FAUSTFLOAT fHslider5;
	float fRec12[2];
	float fRec13[2];
	FAUSTFLOAT fHslider6;
	float fRec14[2];
	FAUSTFLOAT fHslider7;
	float fRec15[2];
	FAUSTFLOAT fHslider8;
	float fRec16[2];
	FAUSTFLOAT fHslider9;
	float fRec18[2];
	float fRec17[2];
	FAUSTFLOAT fEntry0;
	FAUSTFLOAT fEntry1;
	float fRec19[2];
	FAUSTFLOAT fEntry2;
	float fConst12;
	float fRec20[2];
	float fRec21[2];
	float fVec3[2];
	int IOTA0;
	float fVec4[4096];
	float fConst13;
	float fConst14;
	FAUSTFLOAT fHslider10;
	float fRec22[2];
	float fRec23[2];
	float fRec24[2];
	float fVec5[2];
	float fVec6[4096];
	float fRec25[2];
	float fRec26[2];
	float fRec27[2];
	float fConst15;
	float fConst16;
	float fRec28[2];
	float fRec29[2];
	float fRec30[2];
	FAUSTFLOAT fEntry3;
	float fRec9[3];
	float fVec7[2];
	float fConst17;
	float fConst18;
	float fRec6[2];
	float fRec5[3];
	float fRec4[3];
	float fRec35[2];
	float fRec36[2];
	float fVec8[2];
	float fVec9[4096];
	float fRec37[2];
	float fRec38[2];
	float fRec34[3];
	float fVec10[2];
	float fRec33[2];
	float fRec32[3];
	float fRec31[3];
	
 public:
	triple_synth() {
	}
	
	triple_synth(const triple_synth&) = default;
	
	virtual ~triple_synth() = default;
	
	triple_synth& operator=(const triple_synth&) = default;
	
	void metadata(Meta* m) { 
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn triple_synth -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("description", "Synth with 3 detuned oscillators");
		m->declare("envelopes.lib/adsr:author", "Yann Orlarey and Andrey Bundin");
		m->declare("envelopes.lib/author", "GRAME");
		m->declare("envelopes.lib/copyright", "GRAME");
		m->declare("envelopes.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("envelopes.lib/name", "Faust Envelope Library");
		m->declare("envelopes.lib/version", "1.3.0");
		m->declare("filename", "triple_synth.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass0_highpass1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/pole:author", "Julius O. Smith III");
		m->declare("filters.lib/pole:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/pole:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/resonlp:author", "Julius O. Smith III");
		m->declare("filters.lib/resonlp:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/resonlp:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/version", "1.9.0");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Triple Synth");
		m->declare("nvoices", "16");
		m->declare("options", "[midi:on]");
		m->declare("oscillators.lib/lf_sawpos:author", "Bart Brouns, revised by Stéphane Letz");
		m->declare("oscillators.lib/lf_sawpos:licence", "LicenseRef-STK-4.3");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/saw2ptr:author", "Julius O. Smith III");
		m->declare("oscillators.lib/saw2ptr:license", "LicenseRef-STK-4.3");
		m->declare("oscillators.lib/sawN:author", "Julius O. Smith III");
		m->declare("oscillators.lib/sawN:license", "LicenseRef-STK-4.3");
		m->declare("oscillators.lib/version", "1.8.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.4.0");
		m->declare("signals.lib/name", "Faust Routing Library");
		m->declare("signals.lib/version", "1.7.0");
		m->declare("version", "1.0");
		m->declare("zrythm-utils.lib/copyright", "© 2022 Alexandros Theodotou");
		m->declare("zrythm-utils.lib/license", "AGPL-3.0-or-later");
		m->declare("zrythm-utils.lib/name", "Zrythm utils");
		m->declare("zrythm-utils.lib/version", "1.0");
	}

	virtual int getNumInputs() {
		return 0;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
		triple_synthSIG0* sig0 = newtriple_synthSIG0();
		sig0->instanceInittriple_synthSIG0(sample_rate);
		sig0->filltriple_synthSIG0(65536, ftbl0triple_synthSIG0);
		deletetriple_synthSIG0(sig0);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 44.1f / fConst0;
		fConst2 = 1.0f - fConst1;
		fConst3 = 1.0f / fConst0;
		fConst4 = std::tan(56548.668f / fConst0);
		fConst5 = 2.0f * (1.0f - 1.0f / triple_synth_faustpower2_f(fConst4));
		fConst6 = 1.0f / fConst4;
		fConst7 = (fConst6 + -0.618034f) / fConst4 + 1.0f;
		fConst8 = 1.0f / ((fConst6 + 0.618034f) / fConst4 + 1.0f);
		fConst9 = (fConst6 + -1.618034f) / fConst4 + 1.0f;
		fConst10 = 1.0f / ((fConst6 + 1.618034f) / fConst4 + 1.0f);
		fConst11 = 3.1415927f / fConst0;
		fConst12 = 1e+01f / fConst0;
		fConst13 = 0.5f * fConst0;
		fConst14 = 0.25f * fConst0;
		fConst15 = 1.76e+03f / fConst0;
		fConst16 = 4.4e+02f / fConst0;
		fConst17 = 1.0f - fConst6;
		fConst18 = 1.0f / (fConst6 + 1.0f);
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(1e+04f);
		fHslider3 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider4 = static_cast<FAUSTFLOAT>(0.4f);
		fButton0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider5 = static_cast<FAUSTFLOAT>(0.03f);
		fHslider6 = static_cast<FAUSTFLOAT>(0.3f);
		fHslider7 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider8 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider9 = static_cast<FAUSTFLOAT>(0.0f);
		fEntry0 = static_cast<FAUSTFLOAT>(2e+02f);
		fEntry1 = static_cast<FAUSTFLOAT>(0.0f);
		fEntry2 = static_cast<FAUSTFLOAT>(3.0f);
		fHslider10 = static_cast<FAUSTFLOAT>(0.1f);
		fEntry3 = static_cast<FAUSTFLOAT>(1.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = faust_wrap_add(l0, 1)) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = faust_wrap_add(l1, 1)) {
			fRec0[l1] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = faust_wrap_add(l4, 1)) {
			fRec3[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = faust_wrap_add(l5, 1)) {
			fRec2[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = faust_wrap_add(l6, 1)) {
			fRec7[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = faust_wrap_add(l7, 1)) {
			fRec8[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = faust_wrap_add(l8, 1)) {
			fRec10[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = faust_wrap_add(l9, 1)) {
			fVec2[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = faust_wrap_add(l10, 1)) {
			iRec11[l10] = 0;
		}
		for (int l11 = 0; l11 < 2; l11 = faust_wrap_add(l11, 1)) {
			fRec12[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = faust_wrap_add(l12, 1)) {
			fRec13[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = faust_wrap_add(l13, 1)) {
			fRec14[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = faust_wrap_add(l14, 1)) {
			fRec15[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = faust_wrap_add(l15, 1)) {
			fRec16[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = faust_wrap_add(l16, 1)) {
			fRec18[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = faust_wrap_add(l17, 1)) {
			fRec17[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = faust_wrap_add(l18, 1)) {
			fRec19[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = faust_wrap_add(l19, 1)) {
			fRec20[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = faust_wrap_add(l20, 1)) {
			fRec21[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = faust_wrap_add(l21, 1)) {
			fVec3[l21] = 0.0f;
		}
		IOTA0 = 0;
		for (int l22 = 0; l22 < 4096; l22 = faust_wrap_add(l22, 1)) {
			fVec4[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = faust_wrap_add(l23, 1)) {
			fRec22[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = faust_wrap_add(l24, 1)) {
			fRec23[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = faust_wrap_add(l25, 1)) {
			fRec24[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 2; l26 = faust_wrap_add(l26, 1)) {
			fVec5[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 4096; l27 = faust_wrap_add(l27, 1)) {
			fVec6[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 2; l28 = faust_wrap_add(l28, 1)) {
			fRec25[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 2; l29 = faust_wrap_add(l29, 1)) {
			fRec26[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 2; l30 = faust_wrap_add(l30, 1)) {
			fRec27[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 2; l31 = faust_wrap_add(l31, 1)) {
			fRec28[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 2; l32 = faust_wrap_add(l32, 1)) {
			fRec29[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 2; l33 = faust_wrap_add(l33, 1)) {
			fRec30[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 3; l34 = faust_wrap_add(l34, 1)) {
			fRec9[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 2; l35 = faust_wrap_add(l35, 1)) {
			fVec7[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 2; l36 = faust_wrap_add(l36, 1)) {
			fRec6[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 3; l37 = faust_wrap_add(l37, 1)) {
			fRec5[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 3; l38 = faust_wrap_add(l38, 1)) {
			fRec4[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 2; l39 = faust_wrap_add(l39, 1)) {
			fRec35[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 2; l40 = faust_wrap_add(l40, 1)) {
			fRec36[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 2; l41 = faust_wrap_add(l41, 1)) {
			fVec8[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 4096; l42 = faust_wrap_add(l42, 1)) {
			fVec9[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 2; l43 = faust_wrap_add(l43, 1)) {
			fRec37[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 2; l44 = faust_wrap_add(l44, 1)) {
			fRec38[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 3; l45 = faust_wrap_add(l45, 1)) {
			fRec34[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 2; l46 = faust_wrap_add(l46, 1)) {
			fVec10[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 2; l47 = faust_wrap_add(l47, 1)) {
			fRec33[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 3; l48 = faust_wrap_add(l48, 1)) {
			fRec32[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 3; l49 = faust_wrap_add(l49, 1)) {
			fRec31[l49] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual triple_synth* clone() {
		return new triple_synth(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Synth");
		ui_interface->declare(&fEntry1, "midi", "pitchwheel");
		ui_interface->addNumEntry("Pitchwheel", &fEntry1, FAUSTFLOAT(0.0f), FAUSTFLOAT(-2.0f), FAUSTFLOAT(2.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fEntry2, "lv2", "scalepoint Saw 3");
		ui_interface->declare(&fEntry2, "lv2", "scalepoint Sine 0");
		ui_interface->declare(&fEntry2, "lv2", "scalepoint Square 2");
		ui_interface->declare(&fEntry2, "lv2", "scalepoint Triangle 1");
		ui_interface->addNumEntry("Waveform", &fEntry2, FAUSTFLOAT(3.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(3.0f), FAUSTFLOAT(1.0f));
		ui_interface->declare(0, "0", "");
		ui_interface->openHorizontalBox("Envelope");
		ui_interface->declare(&fHslider5, "0", "");
		ui_interface->declare(&fHslider5, "unit", "s");
		ui_interface->addHorizontalSlider("Attack", &fHslider5, FAUSTFLOAT(0.03f), FAUSTFLOAT(0.001f), FAUSTFLOAT(2.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fHslider6, "1", "");
		ui_interface->declare(&fHslider6, "unit", "s");
		ui_interface->addHorizontalSlider("Decay", &fHslider6, FAUSTFLOAT(0.3f), FAUSTFLOAT(0.001f), FAUSTFLOAT(4.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fHslider7, "2", "");
		ui_interface->addHorizontalSlider("Sustain", &fHslider7, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.01f));
		ui_interface->declare(&fHslider4, "3", "");
		ui_interface->declare(&fHslider4, "unit", "s");
		ui_interface->addHorizontalSlider("Release", &fHslider4, FAUSTFLOAT(0.4f), FAUSTFLOAT(0.001f), FAUSTFLOAT(4.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fEntry3, "4", "");
		ui_interface->addNumEntry("gain", &fEntry3, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.01f));
		ui_interface->declare(&fButton0, "5", "");
		ui_interface->addButton("gate", &fButton0);
		ui_interface->closeBox();
		ui_interface->declare(&fHslider1, "10", "");
		ui_interface->declare(&fHslider1, "unit", "Hz");
		ui_interface->addHorizontalSlider("Tremolo Freq", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.02f));
		ui_interface->declare(&fHslider0, "11", "");
		ui_interface->addHorizontalSlider("Tremolo Depth", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(0, "1", "");
		ui_interface->openHorizontalBox("Filter");
		ui_interface->declare(&fHslider2, "0", "");
		ui_interface->declare(&fHslider2, "scale", "log");
		ui_interface->declare(&fHslider2, "tooltip", "Filter cutoff frequency");
		ui_interface->declare(&fHslider2, "unit", "Hz");
		ui_interface->addHorizontalSlider("Cutoff", &fHslider2, FAUSTFLOAT(1e+04f), FAUSTFLOAT(1.1e+02f), FAUSTFLOAT(2e+04f), FAUSTFLOAT(0.01f));
		ui_interface->declare(&fHslider3, "1", "");
		ui_interface->addHorizontalSlider("Cutoff Q", &fHslider3, FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.1f));
		ui_interface->closeBox();
		ui_interface->declare(&fHslider10, "6", "");
		ui_interface->addHorizontalSlider("Detune", &fHslider10, FAUSTFLOAT(0.1f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fHslider9, "8", "");
		ui_interface->declare(&fHslider9, "unit", "Hz");
		ui_interface->addHorizontalSlider("Vibrato Freq", &fHslider9, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.02f));
		ui_interface->declare(&fHslider8, "9", "");
		ui_interface->addHorizontalSlider("Vibrato Depth", &fHslider8, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addNumEntry("freq", &fEntry0, FAUSTFLOAT(2e+02f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(2e+03f), FAUSTFLOAT(0.01f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst1 * static_cast<float>(fHslider0);
		float fSlow1 = fConst1 * static_cast<float>(fHslider1);
		float fSlow2 = fConst1 * static_cast<float>(fHslider2);
		float fSlow3 = fConst1 * static_cast<float>(fHslider3);
		float fSlow4 = fConst1 * static_cast<float>(fHslider4);
		float fSlow5 = static_cast<float>(fButton0);
		int iSlow6 = fSlow5 == 0.0f;
		float fSlow7 = fConst1 * static_cast<float>(fHslider5);
		float fSlow8 = fConst1 * static_cast<float>(fHslider6);
		float fSlow9 = fConst1 * static_cast<float>(fHslider7);
		float fSlow10 = fConst1 * static_cast<float>(fHslider8);
		float fSlow11 = fConst1 * static_cast<float>(fHslider9);
		float fSlow12 = static_cast<float>(fEntry1) + 17.31234f * std::log(0.0022727272f * static_cast<float>(fEntry0));
		float fSlow13 = static_cast<float>(fEntry2);
		float fSlow14 = static_cast<float>(fSlow13 >= 3.0f);
		float fSlow15 = fConst1 * static_cast<float>(fHslider10);
		float fSlow16 = static_cast<float>(fSlow13 >= 2.0f);
		float fSlow17 = static_cast<float>(fSlow13 >= 1.0f);
		float fSlow18 = 0.3f * static_cast<float>(fEntry3);
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			iVec0[0] = 1;
			fRec0[0] = fSlow0 + fConst2 * fRec0[1];
			int iTemp0 = faust_wrap_sub(1, iVec0[1]);
			fRec3[0] = fSlow1 + fConst2 * fRec3[1];
			float fTemp1 = ((iTemp0) ? 0.0f : fRec2[1] + fConst3 * fRec3[0]);
			fRec2[0] = fTemp1 - std::floor(fTemp1);
			float fTemp2 = 1.0f + 0.5f * (-1.0f + ftbl0triple_synthSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec2[0]), 65535))]) * fRec0[0];
			fRec7[0] = fSlow2 + fConst2 * fRec7[1];
			float fTemp3 = std::tan(fConst11 * std::max<float>(2e+01f, std::min<float>(2e+04f, fRec7[0])));
			fRec8[0] = fSlow3 + fConst2 * fRec8[1];
			float fTemp4 = 1.0f / fRec8[0];
			float fTemp5 = 1.0f / fTemp3;
			float fTemp6 = 1.0f + (fTemp5 + fTemp4) / fTemp3;
			float fTemp7 = 1.0f - 1.0f / triple_synth_faustpower2_f(fTemp3);
			float fTemp8 = 1.0f + (fTemp5 - fTemp4) / fTemp3;
			fRec10[0] = fSlow4 + fConst2 * fRec10[1];
			fVec2[0] = fSlow5;
			iRec11[0] = faust_wrap_mul(iSlow6, faust_wrap_add(1, iRec11[1]));
			fRec12[0] = fSlow7 + fConst2 * fRec12[1];
			float fTemp9 = std::max<float>(1.0f, fConst0 * fRec12[0]);
			fRec13[0] = fSlow5 + fRec13[1] * static_cast<float>(fVec2[1] >= fSlow5);
			fRec14[0] = fSlow8 + fConst2 * fRec14[1];
			fRec15[0] = fSlow9 + fConst2 * fRec15[1];
			float fTemp10 = std::max<float>(0.0f, std::min<float>(fRec13[0] / fTemp9, std::max<float>(1.0f + (1.0f - fRec15[0]) * (fTemp9 - fRec13[0]) / std::max<float>(1.0f, fConst0 * fRec14[0]), fRec15[0])) * (1.0f - static_cast<float>(iRec11[0]) / std::max<float>(1.0f, fConst0 * fRec10[0])));
			fRec16[0] = fSlow10 + fConst2 * fRec16[1];
			fRec18[0] = fSlow11 + fConst2 * fRec18[1];
			float fTemp11 = ((iTemp0) ? 0.0f : fRec17[1] + fConst3 * fRec18[0]);
			fRec17[0] = fTemp11 - std::floor(fTemp11);
			float fTemp12 = ftbl0triple_synthSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec17[0]), 65535))] * fRec16[0];
			float fTemp13 = std::pow(2.0f, 0.083333336f * (fSlow12 + fTemp12));
			float fTemp14 = 4.4e+02f * fTemp13;
			float fTemp15 = std::max<float>(1.1920929e-07f, std::fabs(fTemp14));
			float fTemp16 = fRec19[1] + fConst3 * fTemp15;
			float fTemp17 = -1.0f + fTemp16;
			int iTemp18 = fTemp17 < 0.0f;
			fRec19[0] = ((iTemp18) ? fTemp16 : fTemp17);
			float fTemp19 = fConst12 + fRec20[1];
			float fTemp20 = fRec20[1] - fConst12;
			fRec20[0] = ((fTemp19 < fSlow14) ? fTemp19 : ((fTemp20 > fSlow14) ? fTemp20 : fSlow14));
			float fTemp21 = std::max<float>(fTemp14, 23.44895f);
			float fTemp22 = std::max<float>(2e+01f, std::fabs(fTemp21));
			float fTemp23 = ((iTemp0) ? 0.0f : fRec21[1] + fConst3 * fTemp22);
			fRec21[0] = fTemp23 - std::floor(fTemp23);
			float fTemp24 = triple_synth_faustpower2_f(-1.0f + 2.0f * fRec21[0]);
			fVec3[0] = fTemp24;
			float fTemp25 = static_cast<float>(iVec0[1]);
			float fTemp26 = fTemp25 * (fTemp24 - fVec3[1]) / fTemp22;
			fVec4[IOTA0 & 4095] = fTemp26;
			float fTemp27 = std::max<float>(0.0f, std::min<float>(2047.0f, fConst13 / fTemp21));
			int iTemp28 = static_cast<int>(fTemp27);
			float fTemp29 = std::floor(fTemp27);
			float fTemp30 = fTemp26 - fVec4[(faust_wrap_sub(IOTA0, iTemp28)) & 4095] * (1.0f - fTemp27 + fTemp29) - (fTemp27 - fTemp29) * fVec4[(faust_wrap_sub(IOTA0, faust_wrap_add(1, iTemp28))) & 4095];
			float fTemp31 = 1.0f - fRec20[0];
			float fTemp32 = fConst14 * fTemp31 * fTemp30 + fRec20[0] * (-1.0f + 2.0f * ((iTemp18) ? fTemp16 : fTemp16 + fTemp17 * (1.0f - fConst0 / fTemp15)));
			fRec22[0] = fSlow15 + fConst2 * fRec22[1];
			float fTemp33 = std::pow(2.0f, 0.083333336f * (fSlow12 - fRec22[0] + fTemp12));
			float fTemp34 = 4.4e+02f * fTemp33;
			float fTemp35 = std::max<float>(1.1920929e-07f, std::fabs(fTemp34));
			float fTemp36 = fRec23[1] + fConst3 * fTemp35;
			float fTemp37 = -1.0f + fTemp36;
			int iTemp38 = fTemp37 < 0.0f;
			fRec23[0] = ((iTemp38) ? fTemp36 : fTemp37);
			float fTemp39 = std::max<float>(fTemp34, 23.44895f);
			float fTemp40 = std::max<float>(2e+01f, std::fabs(fTemp39));
			float fTemp41 = ((iTemp0) ? 0.0f : fRec24[1] + fConst3 * fTemp40);
			fRec24[0] = fTemp41 - std::floor(fTemp41);
			float fTemp42 = triple_synth_faustpower2_f(-1.0f + 2.0f * fRec24[0]);
			fVec5[0] = fTemp42;
			float fTemp43 = fTemp25 * (fTemp42 - fVec5[1]) / fTemp40;
			fVec6[IOTA0 & 4095] = fTemp43;
			float fTemp44 = std::max<float>(0.0f, std::min<float>(2047.0f, fConst13 / fTemp39));
			int iTemp45 = static_cast<int>(fTemp44);
			float fTemp46 = std::floor(fTemp44);
			float fTemp47 = fTemp43 - fVec6[(faust_wrap_sub(IOTA0, iTemp45)) & 4095] * (1.0f - fTemp44 + fTemp46) - (fTemp44 - fTemp46) * fVec6[(faust_wrap_sub(IOTA0, faust_wrap_add(1, iTemp45))) & 4095];
			float fTemp48 = fConst12 + fRec25[1];
			float fTemp49 = fRec25[1] - fConst12;
			fRec25[0] = ((fTemp48 < fSlow16) ? fTemp48 : ((fTemp49 > fSlow16) ? fTemp49 : fSlow16));
			fRec26[0] = 0.999f * fRec26[1] + fConst14 * fTemp30;
			float fTemp50 = fConst12 + fRec27[1];
			float fTemp51 = fRec27[1] - fConst12;
			fRec27[0] = ((fTemp50 < fSlow17) ? fTemp50 : ((fTemp51 > fSlow17) ? fTemp51 : fSlow17));
			float fTemp52 = ((iTemp0) ? 0.0f : fRec28[1] + fConst16 * fTemp13);
			fRec28[0] = fTemp52 - std::floor(fTemp52);
			float fTemp53 = 1.0f - fRec27[0];
			float fTemp54 = fTemp53 * ftbl0triple_synthSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec28[0]), 65535))] + fConst15 * fRec27[0] * fTemp13 * fRec26[0];
			fRec29[0] = 0.999f * fRec29[1] + fConst14 * fTemp47;
			float fTemp55 = ((iTemp0) ? 0.0f : fRec30[1] + fConst16 * fTemp33);
			fRec30[0] = fTemp55 - std::floor(fTemp55);
			float fTemp56 = 1.0f - fRec25[0];
			fRec9[0] = fSlow18 * (fTemp56 * (ftbl0triple_synthSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec30[0]), 65535))] * fTemp53 + fConst15 * fTemp33 * fRec27[0] * fRec29[0] + fTemp54) + fRec25[0] * (fConst14 * fTemp47 * fTemp31 + fRec20[0] * (-1.0f + 2.0f * ((iTemp38) ? fTemp36 : fTemp36 + fTemp37 * (1.0f - fConst0 / fTemp35))) + fTemp32)) * fTemp10 - (fRec9[2] * fTemp8 + 2.0f * fRec9[1] * fTemp7) / fTemp6;
			float fTemp57 = (fRec9[2] + fRec9[0] + 2.0f * fRec9[1]) / fTemp6;
			fVec7[0] = fTemp57;
			fRec6[0] = -(fConst18 * (fConst17 * fRec6[1] - (fTemp57 + fVec7[1])));
			fRec5[0] = fRec6[0] - fConst10 * (fConst9 * fRec5[2] + fConst5 * fRec5[1]);
			fRec4[0] = fConst10 * (fRec5[2] + fRec5[0] + 2.0f * fRec5[1]) - fConst8 * (fConst7 * fRec4[2] + fConst5 * fRec4[1]);
			output0[i0] = static_cast<FAUSTFLOAT>(fConst8 * (fRec4[2] + fRec4[0] + 2.0f * fRec4[1]) * fTemp2);
			float fTemp58 = std::pow(2.0f, 0.083333336f * (fSlow12 + fTemp12 + fRec22[0]));
			float fTemp59 = 4.4e+02f * fTemp58;
			float fTemp60 = std::max<float>(1.1920929e-07f, std::fabs(fTemp59));
			float fTemp61 = fRec35[1] + fConst3 * fTemp60;
			float fTemp62 = -1.0f + fTemp61;
			int iTemp63 = fTemp62 < 0.0f;
			fRec35[0] = ((iTemp63) ? fTemp61 : fTemp62);
			float fTemp64 = std::max<float>(fTemp59, 23.44895f);
			float fTemp65 = std::max<float>(2e+01f, std::fabs(fTemp64));
			float fTemp66 = ((iTemp0) ? 0.0f : fRec36[1] + fConst3 * fTemp65);
			fRec36[0] = fTemp66 - std::floor(fTemp66);
			float fTemp67 = triple_synth_faustpower2_f(-1.0f + 2.0f * fRec36[0]);
			fVec8[0] = fTemp67;
			float fTemp68 = fTemp25 * (fTemp67 - fVec8[1]) / fTemp65;
			fVec9[IOTA0 & 4095] = fTemp68;
			float fTemp69 = std::max<float>(0.0f, std::min<float>(2047.0f, fConst13 / fTemp64));
			int iTemp70 = static_cast<int>(fTemp69);
			float fTemp71 = std::floor(fTemp69);
			float fTemp72 = fTemp68 - fVec9[(faust_wrap_sub(IOTA0, iTemp70)) & 4095] * (1.0f - fTemp69 + fTemp71) - (fTemp69 - fTemp71) * fVec9[(faust_wrap_sub(IOTA0, faust_wrap_add(1, iTemp70))) & 4095];
			fRec37[0] = 0.999f * fRec37[1] + fConst14 * fTemp72;
			float fTemp73 = ((iTemp0) ? 0.0f : fRec38[1] + fConst16 * fTemp58);
			fRec38[0] = fTemp73 - std::floor(fTemp73);
			fRec34[0] = fSlow18 * fTemp10 * (fTemp56 * (fTemp54 + fTemp53 * ftbl0triple_synthSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec38[0]), 65535))] + fConst15 * fRec27[0] * fTemp58 * fRec37[0]) + fRec25[0] * (fTemp32 + fConst14 * fTemp31 * fTemp72 + fRec20[0] * (-1.0f + 2.0f * ((iTemp63) ? fTemp61 : fTemp61 + fTemp62 * (1.0f - fConst0 / fTemp60))))) - (fTemp8 * fRec34[2] + 2.0f * fTemp7 * fRec34[1]) / fTemp6;
			float fTemp74 = (fRec34[2] + fRec34[0] + 2.0f * fRec34[1]) / fTemp6;
			fVec10[0] = fTemp74;
			fRec33[0] = -(fConst18 * (fConst17 * fRec33[1] - (fTemp74 + fVec10[1])));
			fRec32[0] = fRec33[0] - fConst10 * (fConst9 * fRec32[2] + fConst5 * fRec32[1]);
			fRec31[0] = fConst10 * (fRec32[2] + fRec32[0] + 2.0f * fRec32[1]) - fConst8 * (fConst7 * fRec31[2] + fConst5 * fRec31[1]);
			output1[i0] = static_cast<FAUSTFLOAT>(fConst8 * fTemp2 * (fRec31[2] + fRec31[0] + 2.0f * fRec31[1]));
			iVec0[1] = iVec0[0];
			fRec0[1] = fRec0[0];
			fRec3[1] = fRec3[0];
			fRec2[1] = fRec2[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec10[1] = fRec10[0];
			fVec2[1] = fVec2[0];
			iRec11[1] = iRec11[0];
			fRec12[1] = fRec12[0];
			fRec13[1] = fRec13[0];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec16[1] = fRec16[0];
			fRec18[1] = fRec18[0];
			fRec17[1] = fRec17[0];
			fRec19[1] = fRec19[0];
			fRec20[1] = fRec20[0];
			fRec21[1] = fRec21[0];
			fVec3[1] = fVec3[0];
			IOTA0 = faust_wrap_add(IOTA0, 1);
			fRec22[1] = fRec22[0];
			fRec23[1] = fRec23[0];
			fRec24[1] = fRec24[0];
			fVec5[1] = fVec5[0];
			fRec25[1] = fRec25[0];
			fRec26[1] = fRec26[0];
			fRec27[1] = fRec27[0];
			fRec28[1] = fRec28[0];
			fRec29[1] = fRec29[0];
			fRec30[1] = fRec30[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fVec7[1] = fVec7[0];
			fRec6[1] = fRec6[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec35[1] = fRec35[0];
			fRec36[1] = fRec36[0];
			fVec8[1] = fVec8[0];
			fRec37[1] = fRec37[0];
			fRec38[1] = fRec38[0];
			fRec34[2] = fRec34[1];
			fRec34[1] = fRec34[0];
			fVec10[1] = fVec10[0];
			fRec33[1] = fRec33[0];
			fRec32[2] = fRec32[1];
			fRec32[1] = fRec32[0];
			fRec31[2] = fRec31[1];
			fRec31[1] = fRec31[0];
		}
	}

};

/*************************** END USER SECTION *****************************/

/******************* BEGIN ARCHITECTURE SECTION (part 2/2) ***************/

#include <memory>
#include <mutex>

namespace zrythm_faust
{

  /** Creates a new instance of the generated DSP class. */
  std::unique_ptr<zrythm::plugins::faust::dsp> create_triple_synth ()
  {
    return std::make_unique<triple_synth> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_triple_synth (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        triple_synth::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
