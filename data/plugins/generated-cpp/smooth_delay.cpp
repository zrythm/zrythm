/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Smooth Delay"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn smooth_delay -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __smooth_delay_H__
#define  __smooth_delay_H__

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
#define FAUSTCLASS smooth_delay
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


class smooth_delay : public dsp {
	
 private:
	
	FAUSTFLOAT fHslider0;
	int IOTA0;
	float fVec0[1048576];
	FAUSTFLOAT fHslider1;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec1[2];
	FAUSTFLOAT fHslider2;
	float fConst2;
	float fRec2[2];
	float fRec3[2];
	float fRec4[2];
	float fRec0[2];
	FAUSTFLOAT fHslider3;
	float fVec1[1048576];
	float fRec5[2];
	
 public:
	smooth_delay() {
	}
	
	smooth_delay(const smooth_delay&) = default;
	
	virtual ~smooth_delay() = default;
	
	smooth_delay& operator=(const smooth_delay&) = default;
	
	void metadata(Meta* m) { 
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn smooth_delay -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Delay plugin");
		m->declare("filename", "smooth_delay.dsp");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Smooth Delay");
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
		fConst1 = 0.001f * fConst0;
		fConst2 = 1e+03f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(1e+02f);
		fHslider2 = static_cast<FAUSTFLOAT>(1e+01f);
		fHslider3 = static_cast<FAUSTFLOAT>(5e+01f);
	}
	
	virtual void instanceClear() {
		IOTA0 = 0;
		for (int l0 = 0; l0 < 1048576; l0 = faust_wrap_add(l0, 1)) {
			fVec0[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = faust_wrap_add(l1, 1)) {
			fRec1[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = faust_wrap_add(l2, 1)) {
			fRec2[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = faust_wrap_add(l3, 1)) {
			fRec3[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = faust_wrap_add(l4, 1)) {
			fRec4[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = faust_wrap_add(l5, 1)) {
			fRec0[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 1048576; l6 = faust_wrap_add(l6, 1)) {
			fVec1[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = faust_wrap_add(l7, 1)) {
			fRec5[l7] = 0.0f;
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
	
	virtual smooth_delay* clone() {
		return new smooth_delay(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Smooth Delay");
		ui_interface->declare(&fHslider2, "1", "");
		ui_interface->declare(&fHslider2, "unit", "ms");
		ui_interface->addHorizontalSlider("Interpolation", &fHslider2, FAUSTFLOAT(1e+01f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider1, "2", "");
		ui_interface->declare(&fHslider1, "unit", "ms");
		ui_interface->addHorizontalSlider("Delay", &fHslider1, FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.0f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider0, "3", "");
		ui_interface->addHorizontalSlider("Feedback", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider3, "4", "");
		ui_interface->declare(&fHslider3, "tooltip", "Mix amount");
		ui_interface->declare(&fHslider3, "unit", "percentage");
		ui_interface->addHorizontalSlider("Mix", &fHslider3, FAUSTFLOAT(5e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.1f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = 0.01f * static_cast<float>(fHslider0);
		float fSlow1 = fConst1 * static_cast<float>(fHslider1);
		float fSlow2 = fConst2 / static_cast<float>(fHslider2);
		float fSlow3 = 0.01f * static_cast<float>(fHslider3);
		float fSlow4 = 1.0f - fSlow3;
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			float fTemp0 = static_cast<float>(input0[i0]);
			float fTemp1 = fTemp0 + fSlow0 * fRec0[1];
			fVec0[IOTA0 & 1048575] = fTemp1;
			fRec1[0] = (((fRec3[1] >= 1.0f) & (fRec4[1] != fSlow1)) ? fSlow1 : fRec1[1]);
			float fTemp2 = ((fRec2[1] != 0.0f) ? (((fRec3[1] > 0.0f) & (fRec3[1] < 1.0f)) ? fRec2[1] : 0.0f) : (((fRec3[1] == 0.0f) & (fSlow1 != fRec1[1])) ? fSlow2 : (((fRec3[1] == 1.0f) & (fSlow1 != fRec4[1])) ? -fSlow2 : 0.0f)));
			fRec2[0] = fTemp2;
			fRec3[0] = std::max<float>(0.0f, std::min<float>(1.0f, fRec3[1] + fTemp2));
			fRec4[0] = (((fRec3[1] <= 0.0f) & (fRec1[1] != fSlow1)) ? fSlow1 : fRec4[1]);
			int iTemp3 = static_cast<int>(std::min<float>(524288.0f, std::max<float>(0.0f, fRec1[0])));
			float fTemp4 = fVec0[(faust_wrap_sub(IOTA0, iTemp3)) & 1048575];
			int iTemp5 = static_cast<int>(std::min<float>(524288.0f, std::max<float>(0.0f, fRec4[0])));
			fRec0[0] = fTemp4 + fRec3[0] * (fVec0[(faust_wrap_sub(IOTA0, iTemp5)) & 1048575] - fTemp4);
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow4 * fTemp0 + fSlow3 * fRec0[0]);
			float fTemp6 = static_cast<float>(input1[i0]);
			float fTemp7 = fTemp6 + fSlow0 * fRec5[1];
			fVec1[IOTA0 & 1048575] = fTemp7;
			float fTemp8 = fVec1[(faust_wrap_sub(IOTA0, iTemp3)) & 1048575];
			fRec5[0] = fTemp8 + fRec3[0] * (fVec1[(faust_wrap_sub(IOTA0, iTemp5)) & 1048575] - fTemp8);
			output1[i0] = static_cast<FAUSTFLOAT>(fSlow4 * fTemp6 + fSlow3 * fRec5[0]);
			IOTA0 = faust_wrap_add(IOTA0, 1);
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec0[1] = fRec0[0];
			fRec5[1] = fRec5[0];
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
  std::unique_ptr<zrythm::plugins::faust::dsp> create_smooth_delay ()
  {
    return std::make_unique<smooth_delay> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_smooth_delay (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        smooth_delay::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
