/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Compressor"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn compressor -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __compressor_H__
#define  __compressor_H__

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
#define FAUSTCLASS compressor
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


class compressor : public dsp {
	
 private:
	
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT fHslider2;
	float fRec1[2];
	FAUSTFLOAT fHslider3;
	float fRec0[2];
	FAUSTFLOAT fHslider4;
	
 public:
	compressor() {
	}
	
	compressor(const compressor&) = default;
	
	virtual ~compressor() = default;
	
	compressor& operator=(const compressor&) = default;
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/amp_follower_ar:author", "Jonatan Liljedahl, revised by Romain Michon");
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "1.4.0");
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn compressor -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("compressors.lib/compression_gain_mono:author", "Julius O. Smith III");
		m->declare("compressors.lib/compression_gain_mono:copyright", "Copyright (C) 2014-2020 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("compressors.lib/compression_gain_mono:license", "LicenseRef-STK-4.3");
		m->declare("compressors.lib/compressor_stereo:author", "Julius O. Smith III");
		m->declare("compressors.lib/compressor_stereo:copyright", "Copyright (C) 2014-2020 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("compressors.lib/compressor_stereo:license", "LicenseRef-STK-4.3");
		m->declare("compressors.lib/name", "Faust Compressor Effect Library");
		m->declare("compressors.lib/version", "1.6.0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("description", "Basic compressor");
		m->declare("filename", "compressor.dsp");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Compressor");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("signals.lib/name", "Faust Routing Library");
		m->declare("signals.lib/onePoleSwitching:author", "Jonatan Liljedahl, revised by Dario Sanfilippo");
		m->declare("signals.lib/onePoleSwitching:licence", "LicenseRef-STK-4.3");
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
		fConst0 = 1.0f / std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(1e+01f);
		fHslider1 = static_cast<FAUSTFLOAT>(-2e+01f);
		fHslider2 = static_cast<FAUSTFLOAT>(1e+01f);
		fHslider3 = static_cast<FAUSTFLOAT>(1.0f);
		fHslider4 = static_cast<FAUSTFLOAT>(5e+01f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = faust_wrap_add(l0, 1)) {
			fRec1[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = faust_wrap_add(l1, 1)) {
			fRec0[l1] = 0.0f;
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
	
	virtual compressor* clone() {
		return new compressor(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Compressor");
		ui_interface->declare(&fHslider3, "1", "");
		ui_interface->declare(&fHslider3, "tooltip", "Compression ratio");
		ui_interface->addHorizontalSlider("Ratio", &fHslider3, FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider1, "2", "");
		ui_interface->declare(&fHslider1, "unit", "Hz");
		ui_interface->addHorizontalSlider("Threshold", &fHslider1, FAUSTFLOAT(-2e+01f), FAUSTFLOAT(-5e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider0, "3", "");
		ui_interface->declare(&fHslider0, "unit", "ms");
		ui_interface->addHorizontalSlider("Attack", &fHslider0, FAUSTFLOAT(1e+01f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+02f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider2, "4", "");
		ui_interface->declare(&fHslider2, "unit", "ms");
		ui_interface->addHorizontalSlider("Release", &fHslider2, FAUSTFLOAT(1e+01f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+02f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider4, "5", "");
		ui_interface->declare(&fHslider4, "tooltip", "Mix amount");
		ui_interface->declare(&fHslider4, "unit", "percentage");
		ui_interface->addHorizontalSlider("Mix", &fHslider4, FAUSTFLOAT(5e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.1f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = static_cast<float>(fHslider0);
		float fSlow1 = 0.0005f * fSlow0;
		int iSlow2 = std::fabs(fSlow1) < 1.1920929e-07f;
		float fSlow3 = ((iSlow2) ? 0.0f : std::exp(-(fConst0 / ((iSlow2) ? 1.0f : fSlow1))));
		float fSlow4 = static_cast<float>(fHslider1);
		float fSlow5 = 0.001f * static_cast<float>(fHslider2);
		int iSlow6 = std::fabs(fSlow5) < 1.1920929e-07f;
		float fSlow7 = ((iSlow6) ? 0.0f : std::exp(-(fConst0 / ((iSlow6) ? 1.0f : fSlow5))));
		float fSlow8 = 0.001f * fSlow0;
		int iSlow9 = std::fabs(fSlow8) < 1.1920929e-07f;
		float fSlow10 = ((iSlow9) ? 0.0f : std::exp(-(fConst0 / ((iSlow9) ? 1.0f : fSlow8))));
		float fSlow11 = (1.0f - fSlow3) * (1.0f / std::max<float>(1.1920929e-07f, static_cast<float>(fHslider3)) + -1.0f);
		float fSlow12 = 0.01f * static_cast<float>(fHslider4);
		float fSlow13 = 1.0f - fSlow12;
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			float fTemp0 = static_cast<float>(input1[i0]);
			float fTemp1 = static_cast<float>(input0[i0]);
			float fTemp2 = std::fabs(std::fabs(fTemp1) + std::fabs(fTemp0));
			float fTemp3 = ((fTemp2 > fRec1[1]) ? fSlow10 : fSlow7);
			fRec1[0] = fTemp2 * (1.0f - fTemp3) + fRec1[1] * fTemp3;
			fRec0[0] = fSlow11 * std::max<float>(2e+01f * std::log10(std::max<float>(1.1754944e-38f, fRec1[0])) - fSlow4, 0.0f) + fSlow3 * fRec0[1];
			float fTemp4 = fSlow13 + fSlow12 * std::pow(1e+01f, 0.05f * fRec0[0]);
			output0[i0] = static_cast<FAUSTFLOAT>(fTemp1 * fTemp4);
			output1[i0] = static_cast<FAUSTFLOAT>(fTemp0 * fTemp4);
			fRec1[1] = fRec1[0];
			fRec0[1] = fRec0[0];
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
  std::unique_ptr<zrythm::plugins::faust::dsp> create_compressor ()
  {
    return std::make_unique<compressor> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_compressor (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        compressor::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
