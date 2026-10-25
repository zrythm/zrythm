/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Phaser"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn phaser -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __phaser_H__
#define  __phaser_H__

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
#define FAUSTCLASS phaser
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

static float phaser_faustpower2_f(float value) {
	return value * value;
}
static float phaser_faustpower3_f(float value) {
	return value * value * value;
}
static float phaser_faustpower4_f(float value) {
	return value * value * value * value;
}

class phaser : public dsp {
	
 private:
	
	int iVec0[2];
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec5[2];
	float fRec6[2];
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT fHslider3;
	float fRec7[2];
	float fConst2;
	FAUSTFLOAT fHslider4;
	float fConst3;
	FAUSTFLOAT fHslider5;
	FAUSTFLOAT fHslider6;
	float fRec0[3];
	float fRec1[3];
	float fRec2[3];
	float fRec3[3];
	float fRec4[2];
	FAUSTFLOAT fCheckbox0;
	FAUSTFLOAT fCheckbox1;
	FAUSTFLOAT fHslider7;
	float fRec8[3];
	float fRec9[3];
	float fRec10[3];
	float fRec11[3];
	float fRec12[2];
	
 public:
	phaser() {
	}
	
	phaser(const phaser&) = default;
	
	virtual ~phaser() = default;
	
	phaser& operator=(const phaser&) = default;
	
	void metadata(Meta* m) { 
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn phaser -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("description", "Phaser effect");
		m->declare("filename", "phaser.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/nlf2:author", "Julius O. Smith III");
		m->declare("filters.lib/nlf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/nlf2:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/version", "1.9.0");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Phaser");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.8.0");
		m->declare("phaflangers.lib/name", "Faust Phaser and Flanger Library");
		m->declare("phaflangers.lib/version", "1.1.0");
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
		fConst1 = 6.2831855f / fConst0;
		fConst2 = 1.0f / fConst0;
		fConst3 = 3.1415927f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider1 = static_cast<FAUSTFLOAT>(1e+02f);
		fHslider2 = static_cast<FAUSTFLOAT>(8e+02f);
		fHslider3 = static_cast<FAUSTFLOAT>(1.5f);
		fHslider4 = static_cast<FAUSTFLOAT>(1e+03f);
		fHslider5 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider6 = static_cast<FAUSTFLOAT>(0.0f);
		fCheckbox0 = static_cast<FAUSTFLOAT>(0.0f);
		fCheckbox1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider7 = static_cast<FAUSTFLOAT>(1.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = faust_wrap_add(l0, 1)) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = faust_wrap_add(l1, 1)) {
			fRec5[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = faust_wrap_add(l2, 1)) {
			fRec6[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = faust_wrap_add(l3, 1)) {
			fRec7[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 3; l4 = faust_wrap_add(l4, 1)) {
			fRec0[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 3; l5 = faust_wrap_add(l5, 1)) {
			fRec1[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 3; l6 = faust_wrap_add(l6, 1)) {
			fRec2[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 3; l7 = faust_wrap_add(l7, 1)) {
			fRec3[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = faust_wrap_add(l8, 1)) {
			fRec4[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 3; l9 = faust_wrap_add(l9, 1)) {
			fRec8[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 3; l10 = faust_wrap_add(l10, 1)) {
			fRec9[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = faust_wrap_add(l11, 1)) {
			fRec10[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 3; l12 = faust_wrap_add(l12, 1)) {
			fRec11[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = faust_wrap_add(l13, 1)) {
			fRec12[l13] = 0.0f;
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
	
	virtual phaser* clone() {
		return new phaser(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->declare(0, "tooltip", "Reference:         https://ccrma.stanford.edu/~jos/pasp/Flanging.html");
		ui_interface->openVerticalBox("PHASER2");
		ui_interface->declare(0, "0", "");
		ui_interface->openHorizontalBox("0x00");
		ui_interface->declare(&fCheckbox0, "1", "");
		ui_interface->addCheckButton("Invert Internal Phaser Sum", &fCheckbox0);
		ui_interface->declare(&fCheckbox1, "2", "");
		ui_interface->addCheckButton("Vibrato Mode", &fCheckbox1);
		ui_interface->closeBox();
		ui_interface->declare(0, "1", "");
		ui_interface->openHorizontalBox("0x00");
		ui_interface->declare(&fHslider0, "1", "");
		ui_interface->declare(&fHslider0, "style", "knob");
		ui_interface->declare(&fHslider0, "unit", "Hz");
		ui_interface->addHorizontalSlider("Speed", &fHslider0, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fHslider7, "2", "");
		ui_interface->declare(&fHslider7, "style", "knob");
		ui_interface->addHorizontalSlider("Notch Depth (Intensity)", &fHslider7, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->declare(&fHslider5, "3", "");
		ui_interface->declare(&fHslider5, "style", "knob");
		ui_interface->addHorizontalSlider("Feedback Gain", &fHslider5, FAUSTFLOAT(0.0f), FAUSTFLOAT(-0.999f), FAUSTFLOAT(0.999f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
		ui_interface->declare(0, "2", "");
		ui_interface->openHorizontalBox("0x00");
		ui_interface->declare(&fHslider4, "1", "");
		ui_interface->declare(&fHslider4, "scale", "log");
		ui_interface->declare(&fHslider4, "style", "knob");
		ui_interface->declare(&fHslider4, "unit", "Hz");
		ui_interface->addHorizontalSlider("Notch width", &fHslider4, FAUSTFLOAT(1e+03f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider1, "2", "");
		ui_interface->declare(&fHslider1, "scale", "log");
		ui_interface->declare(&fHslider1, "style", "knob");
		ui_interface->declare(&fHslider1, "unit", "Hz");
		ui_interface->addHorizontalSlider("Min Notch1 Freq", &fHslider1, FAUSTFLOAT(1e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider2, "3", "");
		ui_interface->declare(&fHslider2, "scale", "log");
		ui_interface->declare(&fHslider2, "style", "knob");
		ui_interface->declare(&fHslider2, "unit", "Hz");
		ui_interface->addHorizontalSlider("Max Notch1 Freq", &fHslider2, FAUSTFLOAT(8e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(1e+04f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider3, "4", "");
		ui_interface->declare(&fHslider3, "style", "knob");
		ui_interface->declare(&fHslider3, "tooltip", "NotchFreq(n+1)/NotchFreq(n)");
		ui_interface->addHorizontalSlider("Notch Freq Ratio", &fHslider3, FAUSTFLOAT(1.5f), FAUSTFLOAT(1.1f), FAUSTFLOAT(4.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
		ui_interface->declare(0, "3", "");
		ui_interface->openHorizontalBox("0x00");
		ui_interface->declare(&fHslider6, "unit", "dB");
		ui_interface->addHorizontalSlider("Phaser Output Level", &fHslider6, FAUSTFLOAT(0.0f), FAUSTFLOAT(-6e+01f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.1f));
		ui_interface->closeBox();
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst1 * static_cast<float>(fHslider0);
		float fSlow1 = std::cos(fSlow0);
		float fSlow2 = std::sin(fSlow0);
		float fSlow3 = static_cast<float>(fHslider1);
		float fSlow4 = 3.1415927f * (fSlow3 - std::max<float>(fSlow3, static_cast<float>(fHslider2)));
		float fSlow5 = 6.2831855f * fSlow3;
		float fSlow6 = 0.001f * static_cast<float>(fHslider3);
		float fSlow7 = std::exp(-(fConst3 * static_cast<float>(fHslider4)));
		float fSlow8 = 2.0f * fSlow7;
		float fSlow9 = static_cast<float>(fHslider5);
		float fSlow10 = phaser_faustpower2_f(fSlow7);
		float fSlow11 = std::pow(1e+01f, 0.05f * static_cast<float>(fHslider6));
		float fSlow12 = 0.5f * ((static_cast<int>(static_cast<float>(fCheckbox1))) ? 2.0f : static_cast<float>(fHslider7));
		float fSlow13 = ((static_cast<int>(static_cast<float>(fCheckbox0))) ? -fSlow12 : fSlow12);
		float fSlow14 = fSlow11 * (1.0f - fSlow12);
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			iVec0[0] = 1;
			fRec5[0] = fSlow2 * fRec6[1] + fSlow1 * fRec5[1];
			fRec6[0] = static_cast<float>(faust_wrap_sub(1, iVec0[1])) - fSlow2 * fRec5[1] + fSlow1 * fRec6[1];
			float fTemp0 = fSlow5 - fSlow4 * (1.0f - fRec5[0]);
			fRec7[0] = fSlow6 + 0.999f * fRec7[1];
			float fTemp1 = fRec0[1] * std::cos(fConst2 * fRec7[0] * fTemp0);
			float fTemp2 = static_cast<float>(input0[i0]);
			fRec0[0] = fSlow11 * fTemp2 - fSlow10 * fRec0[2] + fSlow9 * fRec4[1] + fSlow8 * fTemp1;
			float fTemp3 = phaser_faustpower2_f(fRec7[0]);
			float fTemp4 = fRec1[1] * std::cos(fConst2 * fTemp0 * fTemp3);
			fRec1[0] = fRec0[2] + fSlow10 * (fRec0[0] - fRec1[2]) - fSlow8 * (fTemp1 - fTemp4);
			float fTemp5 = phaser_faustpower3_f(fRec7[0]);
			float fTemp6 = fRec2[1] * std::cos(fConst2 * fTemp0 * fTemp5);
			fRec2[0] = fRec1[2] + fSlow10 * (fRec1[0] - fRec2[2]) - fSlow8 * (fTemp4 - fTemp6);
			float fTemp7 = phaser_faustpower4_f(fRec7[0]);
			float fTemp8 = fRec3[1] * std::cos(fConst2 * fTemp0 * fTemp7);
			fRec3[0] = fRec2[2] + fSlow10 * (fRec2[0] - fRec3[2]) - fSlow8 * (fTemp6 - fTemp8);
			fRec4[0] = fRec3[2] + fSlow10 * fRec3[0] - fSlow8 * fTemp8;
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow14 * fTemp2 + fSlow13 * fRec4[0]);
			float fTemp9 = fSlow5 - fSlow4 * (1.0f - fRec6[0]);
			float fTemp10 = fRec8[1] * std::cos(fConst2 * fRec7[0] * fTemp9);
			float fTemp11 = static_cast<float>(input1[i0]);
			fRec8[0] = fSlow11 * fTemp11 - fSlow10 * fRec8[2] + fSlow9 * fRec12[1] + fSlow8 * fTemp10;
			float fTemp12 = fRec9[1] * std::cos(fConst2 * fTemp3 * fTemp9);
			fRec9[0] = fRec8[2] + fSlow10 * (fRec8[0] - fRec9[2]) - fSlow8 * (fTemp10 - fTemp12);
			float fTemp13 = fRec10[1] * std::cos(fConst2 * fTemp5 * fTemp9);
			fRec10[0] = fRec9[2] + fSlow10 * (fRec9[0] - fRec10[2]) - fSlow8 * (fTemp12 - fTemp13);
			float fTemp14 = fRec11[1] * std::cos(fConst2 * fTemp7 * fTemp9);
			fRec11[0] = fRec10[2] + fSlow10 * (fRec10[0] - fRec11[2]) - fSlow8 * (fTemp13 - fTemp14);
			fRec12[0] = fRec11[2] + fSlow10 * fRec11[0] - fSlow8 * fTemp14;
			output1[i0] = static_cast<FAUSTFLOAT>(fSlow14 * fTemp11 + fSlow13 * fRec12[0]);
			iVec0[1] = iVec0[0];
			fRec5[1] = fRec5[0];
			fRec6[1] = fRec6[0];
			fRec7[1] = fRec7[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec11[2] = fRec11[1];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
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
  std::unique_ptr<zrythm::plugins::faust::dsp> create_phaser ()
  {
    return std::make_unique<phaser> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_phaser (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        phaser::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
