/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Parametric EQ"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn parametric_eq -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __parametric_eq_H__
#define  __parametric_eq_H__

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
#define FAUSTCLASS parametric_eq
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

static float parametric_eq_faustpower2_f(float value) {
	return value * value;
}

class parametric_eq : public dsp {
	
 private:
	
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider1;
	float fRec2[2];
	FAUSTFLOAT fHslider2;
	float fConst2;
	FAUSTFLOAT fHslider3;
	FAUSTFLOAT fHslider4;
	float fVec0[2];
	float fRec5[2];
	float fRec4[3];
	FAUSTFLOAT fHslider5;
	float fRec7[2];
	float fRec6[3];
	float fRec3[3];
	float fVec1[2];
	float fRec1[2];
	float fRec0[3];
	FAUSTFLOAT fHslider6;
	float fRec9[2];
	float fRec8[3];
	float fVec2[2];
	float fRec14[2];
	float fRec13[3];
	float fRec16[2];
	float fRec15[3];
	float fRec12[3];
	float fVec3[2];
	float fRec11[2];
	float fRec10[3];
	float fRec18[2];
	float fRec17[3];
	
 public:
	parametric_eq() {
	}
	
	parametric_eq(const parametric_eq&) = default;
	
	virtual ~parametric_eq() = default;
	
	parametric_eq& operator=(const parametric_eq&) = default;
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "1.4.0");
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn parametric_eq -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("description", "Parametric equalizer with high/low shelves");
		m->declare("filename", "parametric_eq.dsp");
		m->declare("filters.lib/filterbank:author", "Julius O. Smith III");
		m->declare("filters.lib/filterbank:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/filterbank:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highpass:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/highshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/highshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highshelf:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/low_shelf:author", "Julius O. Smith III");
		m->declare("filters.lib/low_shelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/low_shelf:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass0_highpass1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/lowshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowshelf:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/peak_eq:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/peak_eq_cq:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq_cq:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq_cq:license", "LicenseRef-STK-4.3");
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
		m->declare("name", "Parametric EQ");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("signals.lib/name", "Faust Routing Library");
		m->declare("signals.lib/version", "1.7.0");
		m->declare("version", "1.0");
		m->declare("zrythm-utils.lib/copyright", "© 2022 Alexandros Theodotou");
		m->declare("zrythm-utils.lib/license", "AGPL-3.0-or-later");
		m->declare("zrythm-utils.lib/name", "Zrythm utils");
		m->declare("zrythm-utils.lib/version", "1.0");
	}

	virtual int getNumInputs() {
		return 2;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 3.1415927f / fConst0;
		fConst2 = 6.2831855f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(8e+03f);
		fHslider1 = static_cast<FAUSTFLOAT>(7.2e+02f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider4 = static_cast<FAUSTFLOAT>(2e+02f);
		fHslider5 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider6 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = faust_wrap_add(l0, 1)) {
			fRec2[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = faust_wrap_add(l1, 1)) {
			fVec0[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = faust_wrap_add(l2, 1)) {
			fRec5[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 3; l3 = faust_wrap_add(l3, 1)) {
			fRec4[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = faust_wrap_add(l4, 1)) {
			fRec7[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 3; l5 = faust_wrap_add(l5, 1)) {
			fRec6[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 3; l6 = faust_wrap_add(l6, 1)) {
			fRec3[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = faust_wrap_add(l7, 1)) {
			fVec1[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = faust_wrap_add(l8, 1)) {
			fRec1[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 3; l9 = faust_wrap_add(l9, 1)) {
			fRec0[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = faust_wrap_add(l10, 1)) {
			fRec9[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = faust_wrap_add(l11, 1)) {
			fRec8[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = faust_wrap_add(l12, 1)) {
			fVec2[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = faust_wrap_add(l13, 1)) {
			fRec14[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 3; l14 = faust_wrap_add(l14, 1)) {
			fRec13[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = faust_wrap_add(l15, 1)) {
			fRec16[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 3; l16 = faust_wrap_add(l16, 1)) {
			fRec15[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 3; l17 = faust_wrap_add(l17, 1)) {
			fRec12[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = faust_wrap_add(l18, 1)) {
			fVec3[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = faust_wrap_add(l19, 1)) {
			fRec11[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 3; l20 = faust_wrap_add(l20, 1)) {
			fRec10[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = faust_wrap_add(l21, 1)) {
			fRec18[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 3; l22 = faust_wrap_add(l22, 1)) {
			fRec17[l22] = 0.0f;
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
	
	virtual parametric_eq* clone() {
		return new parametric_eq(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->declare(0, "0", "");
		ui_interface->declare(0, "tooltip", "See Faust's filters.lib         for info and pointers");
		ui_interface->openHorizontalBox("PARAMETRIC EQ SECTIONS");
		ui_interface->declare(0, "1", "");
		ui_interface->openVerticalBox("Low Shelf");
		ui_interface->declare(&fHslider5, "0", "");
		ui_interface->declare(&fHslider5, "style", "knob");
		ui_interface->declare(&fHslider5, "tooltip", "Amount of low-frequency boost or cut in decibels");
		ui_interface->declare(&fHslider5, "unit", "dB");
		ui_interface->addHorizontalSlider("Low Shelf Gain", &fHslider5, FAUSTFLOAT(0.0f), FAUSTFLOAT(-4e+01f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider4, "1", "");
		ui_interface->declare(&fHslider4, "scale", "log");
		ui_interface->declare(&fHslider4, "style", "knob");
		ui_interface->declare(&fHslider4, "tooltip", "Transition-frequency from boost (cut) to unity gain");
		ui_interface->declare(&fHslider4, "unit", "Hz");
		ui_interface->addHorizontalSlider("Low Shelf Frequency", &fHslider4, FAUSTFLOAT(2e+02f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(1.0f));
		ui_interface->closeBox();
		ui_interface->declare(0, "2", "");
		ui_interface->declare(0, "tooltip", "Parametric Equalizer         sections from filters.lib");
		ui_interface->openVerticalBox("Peaking Equalizer");
		ui_interface->declare(&fHslider2, "0", "");
		ui_interface->declare(&fHslider2, "style", "knob");
		ui_interface->declare(&fHslider2, "tooltip", "Amount of         local boost or cut in decibels");
		ui_interface->declare(&fHslider2, "unit", "dB");
		ui_interface->addHorizontalSlider("Peak Gain", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(-4e+01f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider1, "1", "");
		ui_interface->declare(&fHslider1, "style", "knob");
		ui_interface->declare(&fHslider1, "tooltip", "Peak         Frequency");
		ui_interface->declare(&fHslider1, "unit", "Hz");
		ui_interface->addHorizontalSlider("Peak Frequency", &fHslider1, FAUSTFLOAT(7.2e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(1.6e+04f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider3, "2", "");
		ui_interface->declare(&fHslider3, "scale", "log");
		ui_interface->declare(&fHslider3, "style", "knob");
		ui_interface->declare(&fHslider3, "tooltip", "Quality factor (Q) of the peak = center-frequency/bandwidth");
		ui_interface->addHorizontalSlider("Peak Q", &fHslider3, FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.01f));
		ui_interface->closeBox();
		ui_interface->declare(0, "3", "");
		ui_interface->declare(0, "tooltip", "A high shelf provides a boost         or cut above some frequency");
		ui_interface->openVerticalBox("High Shelf");
		ui_interface->declare(&fHslider6, "0", "");
		ui_interface->declare(&fHslider6, "style", "knob");
		ui_interface->declare(&fHslider6, "tooltip", "Amount of         high-frequency boost or cut in decibels");
		ui_interface->declare(&fHslider6, "unit", "dB");
		ui_interface->addHorizontalSlider("High Shelf Gain", &fHslider6, FAUSTFLOAT(0.0f), FAUSTFLOAT(-4e+01f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider0, "1", "");
		ui_interface->declare(&fHslider0, "scale", "log");
		ui_interface->declare(&fHslider0, "style", "knob");
		ui_interface->declare(&fHslider0, "tooltip", "Transition-frequency from boost (cut) to unity gain");
		ui_interface->declare(&fHslider0, "unit", "Hz");
		ui_interface->addHorizontalSlider("High Shelf Frequency", &fHslider0, FAUSTFLOAT(8e+03f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(1.8e+04f), FAUSTFLOAT(1.0f));
		ui_interface->closeBox();
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = std::tan(fConst1 * static_cast<float>(fHslider0));
		float fSlow1 = parametric_eq_faustpower2_f(fSlow0);
		float fSlow2 = 2.0f * (1.0f - 1.0f / fSlow1);
		float fSlow3 = 1.0f / fSlow0;
		float fSlow4 = (fSlow3 + -1.0f) / fSlow0 + 1.0f;
		float fSlow5 = 1.0f / ((fSlow3 + 1.0f) / fSlow0 + 1.0f);
		float fSlow6 = 0.001f * static_cast<float>(fHslider1);
		float fSlow7 = static_cast<float>(fHslider2);
		int iSlow8 = fSlow7 > 0.0f;
		float fSlow9 = static_cast<float>(fHslider3);
		float fSlow10 = fConst1 * (std::pow(1e+01f, 0.05f * std::fabs(fSlow7)) / fSlow9);
		float fSlow11 = fConst1 / fSlow9;
		float fSlow12 = std::tan(fConst1 * static_cast<float>(fHslider4));
		float fSlow13 = 1.0f / parametric_eq_faustpower2_f(fSlow12);
		float fSlow14 = 2.0f * (1.0f - fSlow13);
		float fSlow15 = 1.0f / fSlow12;
		float fSlow16 = (fSlow15 + -1.0f) / fSlow12 + 1.0f;
		float fSlow17 = 1.0f / ((fSlow15 + 1.0f) / fSlow12 + 1.0f);
		float fSlow18 = 1.0f - fSlow15;
		float fSlow19 = 1.0f / (fSlow15 + 1.0f);
		float fSlow20 = std::pow(1e+01f, 0.05f * static_cast<float>(fHslider5));
		float fSlow21 = 1.0f - fSlow3;
		float fSlow22 = 1.0f / (fSlow3 + 1.0f);
		float fSlow23 = std::pow(1e+01f, 0.05f * static_cast<float>(fHslider6)) / fSlow1;
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			fRec2[0] = fSlow6 + 0.999f * fRec2[1];
			float fTemp0 = std::tan(fConst1 * fRec2[0]);
			float fTemp1 = fRec2[0] / std::sin(fConst2 * fRec2[0]);
			float fTemp2 = fSlow10 * fTemp1;
			float fTemp3 = fSlow11 * fTemp1;
			float fTemp4 = ((iSlow8) ? fTemp3 : fTemp2);
			float fTemp5 = 1.0f / fTemp0;
			float fTemp6 = 1.0f + (fTemp5 + fTemp4) / fTemp0;
			float fTemp7 = ((iSlow8) ? fTemp2 : fTemp3);
			float fTemp8 = 1.0f + (fTemp5 - fTemp7) / fTemp0;
			float fTemp9 = 1.0f - 1.0f / parametric_eq_faustpower2_f(fTemp0);
			float fTemp10 = 2.0f * fRec3[1] * fTemp9;
			float fTemp11 = 1.0f + (fTemp5 - fTemp4) / fTemp0;
			float fTemp12 = static_cast<float>(input0[i0]);
			fVec0[0] = fTemp12;
			fRec5[0] = -(fSlow19 * (fSlow18 * fRec5[1] - (fTemp12 + fVec0[1])));
			fRec4[0] = fRec5[0] - fSlow17 * (fSlow16 * fRec4[2] + fSlow14 * fRec4[1]);
			fRec7[0] = -(fSlow19 * (fSlow18 * fRec7[1] - fSlow15 * (fTemp12 - fVec0[1])));
			fRec6[0] = fRec7[0] - fSlow17 * (fSlow16 * fRec6[2] + fSlow14 * fRec6[1]);
			fRec3[0] = fSlow17 * (fSlow13 * (fRec6[2] + (fRec6[0] - 2.0f * fRec6[1])) + fSlow20 * (fRec4[2] + fRec4[0] + 2.0f * fRec4[1])) - (fRec3[2] * fTemp11 + fTemp10) / fTemp6;
			float fTemp13 = 1.0f + (fTemp5 + fTemp7) / fTemp0;
			float fTemp14 = (fTemp10 + fRec3[0] * fTemp13 + fRec3[2] * fTemp8) / fTemp6;
			fVec1[0] = fTemp14;
			fRec1[0] = -(fSlow22 * (fSlow21 * fRec1[1] - fSlow3 * (fTemp14 - fVec1[1])));
			fRec0[0] = fRec1[0] - fSlow5 * (fSlow4 * fRec0[2] + fSlow2 * fRec0[1]);
			fRec9[0] = -(fSlow22 * (fSlow21 * fRec9[1] - (fTemp14 + fVec1[1])));
			fRec8[0] = fRec9[0] - fSlow5 * (fSlow4 * fRec8[2] + fSlow2 * fRec8[1]);
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow5 * (fRec8[2] + fRec8[0] + 2.0f * fRec8[1] + fSlow23 * (fRec0[2] + (fRec0[0] - 2.0f * fRec0[1]))));
			float fTemp15 = 2.0f * fTemp9 * fRec12[1];
			float fTemp16 = static_cast<float>(input1[i0]);
			fVec2[0] = fTemp16;
			fRec14[0] = -(fSlow19 * (fSlow18 * fRec14[1] - (fTemp16 + fVec2[1])));
			fRec13[0] = fRec14[0] - fSlow17 * (fSlow16 * fRec13[2] + fSlow14 * fRec13[1]);
			fRec16[0] = -(fSlow19 * (fSlow18 * fRec16[1] - fSlow15 * (fTemp16 - fVec2[1])));
			fRec15[0] = fRec16[0] - fSlow17 * (fSlow16 * fRec15[2] + fSlow14 * fRec15[1]);
			fRec12[0] = fSlow17 * (fSlow13 * (fRec15[2] + (fRec15[0] - 2.0f * fRec15[1])) + fSlow20 * (fRec13[2] + fRec13[0] + 2.0f * fRec13[1])) - (fTemp11 * fRec12[2] + fTemp15) / fTemp6;
			float fTemp17 = (fTemp15 + fTemp13 * fRec12[0] + fTemp8 * fRec12[2]) / fTemp6;
			fVec3[0] = fTemp17;
			fRec11[0] = -(fSlow22 * (fSlow21 * fRec11[1] - fSlow3 * (fTemp17 - fVec3[1])));
			fRec10[0] = fRec11[0] - fSlow5 * (fSlow4 * fRec10[2] + fSlow2 * fRec10[1]);
			fRec18[0] = -(fSlow22 * (fSlow21 * fRec18[1] - (fTemp17 + fVec3[1])));
			fRec17[0] = fRec18[0] - fSlow5 * (fSlow4 * fRec17[2] + fSlow2 * fRec17[1]);
			output1[i0] = static_cast<FAUSTFLOAT>(fSlow5 * (fRec17[2] + fRec17[0] + 2.0f * fRec17[1] + fSlow23 * (fRec10[2] + (fRec10[0] - 2.0f * fRec10[1]))));
			fRec2[1] = fRec2[0];
			fVec0[1] = fVec0[0];
			fRec5[1] = fRec5[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec7[1] = fRec7[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fVec1[1] = fVec1[0];
			fRec1[1] = fRec1[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec9[1] = fRec9[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fVec2[1] = fVec2[0];
			fRec14[1] = fRec14[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fRec16[1] = fRec16[0];
			fRec15[2] = fRec15[1];
			fRec15[1] = fRec15[0];
			fRec12[2] = fRec12[1];
			fRec12[1] = fRec12[0];
			fVec3[1] = fVec3[0];
			fRec11[1] = fRec11[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec18[1] = fRec18[0];
			fRec17[2] = fRec17[1];
			fRec17[1] = fRec17[0];
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
  std::unique_ptr<zrythm::plugins::faust::dsp> create_parametric_eq ()
  {
    return std::make_unique<parametric_eq> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_parametric_eq (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        parametric_eq::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
