/* ------------------------------------------------------------
author: "Zrythm"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Zita Rev1"
version: "1.0"
Code generated with Faust 2.88.0 (https://faust.grame.fr)
Compilation options: -a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn zita_rev1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __zita_rev1_H__
#define  __zita_rev1_H__

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
#define FAUSTCLASS zita_rev1
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

static float zita_rev1_faustpower2_f(float value) {
	return value * value;
}

class zita_rev1 : public dsp {
	
 private:
	
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec0[2];
	float fRec1[2];
	float fRec2[2];
	float fRec3[2];
	FAUSTFLOAT fHslider1;
	float fConst2;
	float fConst3;
	FAUSTFLOAT fHslider2;
	float fConst4;
	FAUSTFLOAT fHslider3;
	float fRec4[2];
	float fConst5;
	float fConst6;
	float fRec5[2];
	float fRec6[2];
	float fRec7[2];
	float fConst7;
	float fConst8;
	float fRec8[2];
	float fConst9;
	float fConst10;
	float fRec9[2];
	float fRec10[2];
	float fRec11[2];
	float fConst11;
	float fConst12;
	float fRec12[2];
	int IOTA0;
	float fVec0[65536];
	float fConst13;
	int iConst14;
	float fVec1[16384];
	FAUSTFLOAT fHslider4;
	float fConst15;
	float fVec2[8192];
	int iConst16;
	float fRec13[2];
	float fVec3[65536];
	float fConst17;
	int iConst18;
	float fVec4[16384];
	float fVec5[8192];
	float fConst19;
	float fConst20;
	float fRec16[2];
	int iConst21;
	float fRec17[2];
	float fVec6[65536];
	float fConst22;
	int iConst23;
	float fVec7[8192];
	int iConst24;
	float fRec18[2];
	float fVec8[32768];
	float fConst25;
	int iConst26;
	float fVec9[4096];
	int iConst27;
	float fRec20[2];
	float fConst28;
	float fConst29;
	float fRec21[2];
	float fVec10[32768];
	float fConst30;
	int iConst31;
	float fVec11[4096];
	float fConst32;
	float fConst33;
	float fRec23[2];
	float fVec12[65536];
	float fConst34;
	int iConst35;
	float fVec13[4096];
	int iConst36;
	float fRec24[2];
	int iConst37;
	float fRec26[2];
	float fVec14[65536];
	float fConst38;
	int iConst39;
	float fVec15[8192];
	int iConst40;
	float fRec28[2];
	float fVec16[32768];
	float fConst41;
	int iConst42;
	float fVec17[8192];
	int iConst43;
	float fRec31[2];
	float fRec32[3];
	float fRec33[3];
	float fRec34[3];
	float fRec35[3];
	float fRec36[3];
	float fRec37[3];
	float fRec38[3];
	float fRec39[3];
	FAUSTFLOAT fHslider5;
	
 public:
	zita_rev1() {
	}
	
	zita_rev1(const zita_rev1&) = default;
	
	virtual ~zita_rev1() = default;
	
	zita_rev1& operator=(const zita_rev1&) = default;
	
	void metadata(Meta* m) { 
		m->declare("author", "Zrythm");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.23.0");
		m->declare("compile_options", "-a data/plugins/zrythm-arch.cpp -lang cpp -fpga-mem-th 4 -ct 1 -cn zita_rev1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Zita reverb algorithm");
		m->declare("filename", "zita_rev1.dsp");
		m->declare("filters.lib/allpass_comb:author", "Julius O. Smith III");
		m->declare("filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/allpass_comb:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass0_highpass1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "LicenseRef-STK-4.3");
		m->declare("filters.lib/version", "1.9.0");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LicenseRef-LGPL-2.1-or-later-with-Faust-exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Zita Rev1");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "1.5.1");
		m->declare("routes.lib/hadamard:author", "Remy Muller, revised by Romain Michon");
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
		fConst2 = std::floor(0.219991f * fConst0 + 0.5f);
		fConst3 = 6.9077554f * (fConst2 / fConst0);
		fConst4 = 6.2831855f / fConst0;
		fConst5 = std::floor(0.256891f * fConst0 + 0.5f);
		fConst6 = 6.9077554f * (fConst5 / fConst0);
		fConst7 = std::floor(0.192303f * fConst0 + 0.5f);
		fConst8 = 6.9077554f * (fConst7 / fConst0);
		fConst9 = std::floor(0.174713f * fConst0 + 0.5f);
		fConst10 = 6.9077554f * (fConst9 / fConst0);
		fConst11 = std::floor(0.153129f * fConst0 + 0.5f);
		fConst12 = 6.9077554f * (fConst11 / fConst0);
		fConst13 = std::floor(0.029291f * fConst0 + 0.5f);
		iConst14 = static_cast<int>(std::min<float>(32768.0f, std::max<float>(0.0f, fConst7 - fConst13)));
		fConst15 = 0.001f * fConst0;
		iConst16 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst13 + -1.0f)));
		fConst17 = std::floor(0.022904f * fConst0 + 0.5f);
		iConst18 = static_cast<int>(std::min<float>(32768.0f, std::max<float>(0.0f, fConst9 - fConst17)));
		fConst19 = std::floor(0.125f * fConst0 + 0.5f);
		fConst20 = 6.9077554f * (fConst19 / fConst0);
		iConst21 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst17 + -1.0f)));
		fConst22 = std::floor(0.027333f * fConst0 + 0.5f);
		iConst23 = static_cast<int>(std::min<float>(65536.0f, std::max<float>(0.0f, fConst5 - fConst22)));
		iConst24 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst22 + -1.0f)));
		fConst25 = std::floor(0.020346f * fConst0 + 0.5f);
		iConst26 = static_cast<int>(std::min<float>(32768.0f, std::max<float>(0.0f, fConst11 - fConst25)));
		iConst27 = static_cast<int>(std::min<float>(4096.0f, std::max<float>(0.0f, fConst25 + -1.0f)));
		fConst28 = std::floor(0.127837f * fConst0 + 0.5f);
		fConst29 = 6.9077554f * (fConst28 / fConst0);
		fConst30 = std::floor(0.013458f * fConst0 + 0.5f);
		iConst31 = static_cast<int>(std::min<float>(32768.0f, std::max<float>(0.0f, fConst19 - fConst30)));
		fConst32 = std::floor(0.210389f * fConst0 + 0.5f);
		fConst33 = 6.9077554f * (fConst32 / fConst0);
		fConst34 = std::floor(0.019123f * fConst0 + 0.5f);
		iConst35 = static_cast<int>(std::min<float>(65536.0f, std::max<float>(0.0f, fConst2 - fConst34)));
		iConst36 = static_cast<int>(std::min<float>(4096.0f, std::max<float>(0.0f, fConst34 + -1.0f)));
		iConst37 = static_cast<int>(std::min<float>(4096.0f, std::max<float>(0.0f, fConst30 + -1.0f)));
		fConst38 = std::floor(0.024421f * fConst0 + 0.5f);
		iConst39 = static_cast<int>(std::min<float>(65536.0f, std::max<float>(0.0f, fConst32 - fConst38)));
		iConst40 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst38 + -1.0f)));
		fConst41 = std::floor(0.031604f * fConst0 + 0.5f);
		iConst42 = static_cast<int>(std::min<float>(32768.0f, std::max<float>(0.0f, fConst28 - fConst41)));
		iConst43 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst41 + -1.0f)));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(2e+02f);
		fHslider1 = static_cast<FAUSTFLOAT>(2.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(6e+03f);
		fHslider3 = static_cast<FAUSTFLOAT>(3.0f);
		fHslider4 = static_cast<FAUSTFLOAT>(2e+01f);
		fHslider5 = static_cast<FAUSTFLOAT>(5e+01f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = faust_wrap_add(l0, 1)) {
			fRec0[l0] = 0.0f;
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
			fRec5[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = faust_wrap_add(l6, 1)) {
			fRec6[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = faust_wrap_add(l7, 1)) {
			fRec7[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = faust_wrap_add(l8, 1)) {
			fRec8[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = faust_wrap_add(l9, 1)) {
			fRec9[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = faust_wrap_add(l10, 1)) {
			fRec10[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = faust_wrap_add(l11, 1)) {
			fRec11[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = faust_wrap_add(l12, 1)) {
			fRec12[l12] = 0.0f;
		}
		IOTA0 = 0;
		for (int l13 = 0; l13 < 65536; l13 = faust_wrap_add(l13, 1)) {
			fVec0[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 16384; l14 = faust_wrap_add(l14, 1)) {
			fVec1[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 8192; l15 = faust_wrap_add(l15, 1)) {
			fVec2[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = faust_wrap_add(l16, 1)) {
			fRec13[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 65536; l17 = faust_wrap_add(l17, 1)) {
			fVec3[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 16384; l18 = faust_wrap_add(l18, 1)) {
			fVec4[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 8192; l19 = faust_wrap_add(l19, 1)) {
			fVec5[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = faust_wrap_add(l20, 1)) {
			fRec16[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = faust_wrap_add(l21, 1)) {
			fRec17[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 65536; l22 = faust_wrap_add(l22, 1)) {
			fVec6[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 8192; l23 = faust_wrap_add(l23, 1)) {
			fVec7[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = faust_wrap_add(l24, 1)) {
			fRec18[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 32768; l25 = faust_wrap_add(l25, 1)) {
			fVec8[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 4096; l26 = faust_wrap_add(l26, 1)) {
			fVec9[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 2; l27 = faust_wrap_add(l27, 1)) {
			fRec20[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 2; l28 = faust_wrap_add(l28, 1)) {
			fRec21[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 32768; l29 = faust_wrap_add(l29, 1)) {
			fVec10[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 4096; l30 = faust_wrap_add(l30, 1)) {
			fVec11[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 2; l31 = faust_wrap_add(l31, 1)) {
			fRec23[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 65536; l32 = faust_wrap_add(l32, 1)) {
			fVec12[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 4096; l33 = faust_wrap_add(l33, 1)) {
			fVec13[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 2; l34 = faust_wrap_add(l34, 1)) {
			fRec24[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 2; l35 = faust_wrap_add(l35, 1)) {
			fRec26[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 65536; l36 = faust_wrap_add(l36, 1)) {
			fVec14[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 8192; l37 = faust_wrap_add(l37, 1)) {
			fVec15[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 2; l38 = faust_wrap_add(l38, 1)) {
			fRec28[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 32768; l39 = faust_wrap_add(l39, 1)) {
			fVec16[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 8192; l40 = faust_wrap_add(l40, 1)) {
			fVec17[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 2; l41 = faust_wrap_add(l41, 1)) {
			fRec31[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 3; l42 = faust_wrap_add(l42, 1)) {
			fRec32[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 3; l43 = faust_wrap_add(l43, 1)) {
			fRec33[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 3; l44 = faust_wrap_add(l44, 1)) {
			fRec34[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 3; l45 = faust_wrap_add(l45, 1)) {
			fRec35[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 3; l46 = faust_wrap_add(l46, 1)) {
			fRec36[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 3; l47 = faust_wrap_add(l47, 1)) {
			fRec37[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 3; l48 = faust_wrap_add(l48, 1)) {
			fRec38[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 3; l49 = faust_wrap_add(l49, 1)) {
			fRec39[l49] = 0.0f;
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
	
	virtual zita_rev1* clone() {
		return new zita_rev1(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Zita Rev1");
		ui_interface->declare(&fHslider4, "1", "");
		ui_interface->declare(&fHslider4, "unit", "ms");
		ui_interface->addHorizontalSlider("Pre-Delay", &fHslider4, FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider0, "2", "");
		ui_interface->declare(&fHslider0, "unit", "Hz");
		ui_interface->addHorizontalSlider("F1", &fHslider0, FAUSTFLOAT(2e+02f), FAUSTFLOAT(5e+01f), FAUSTFLOAT(1e+03f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider2, "3", "");
		ui_interface->declare(&fHslider2, "unit", "Hz");
		ui_interface->addHorizontalSlider("F2", &fHslider2, FAUSTFLOAT(6e+03f), FAUSTFLOAT(1.5e+03f), FAUSTFLOAT(9.408e+04f), FAUSTFLOAT(1.0f));
		ui_interface->declare(&fHslider3, "4", "");
		ui_interface->declare(&fHslider3, "tooltip", "T60 = time (in seconds) to decay 60dB in low-frequency band");
		ui_interface->declare(&fHslider3, "unit", "s");
		ui_interface->addHorizontalSlider("Low RT60", &fHslider3, FAUSTFLOAT(3.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(8.0f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider1, "5", "");
		ui_interface->declare(&fHslider1, "tooltip", "T60 = time (in seconds) to decay 60dB in middle band");
		ui_interface->declare(&fHslider1, "unit", "s");
		ui_interface->addHorizontalSlider("Mid RT60", &fHslider1, FAUSTFLOAT(2.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(8.0f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider5, "6", "");
		ui_interface->declare(&fHslider5, "tooltip", "Mix amount");
		ui_interface->declare(&fHslider5, "unit", "percentage");
		ui_interface->addHorizontalSlider("Mix", &fHslider5, FAUSTFLOAT(5e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.1f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = 1.0f / std::tan(fConst1 * static_cast<float>(fHslider0));
		float fSlow1 = 1.0f - fSlow0;
		float fSlow2 = 1.0f / (fSlow0 + 1.0f);
		float fSlow3 = static_cast<float>(fHslider1);
		float fSlow4 = std::exp(-(fConst3 / fSlow3));
		float fSlow5 = zita_rev1_faustpower2_f(fSlow4);
		float fSlow6 = 1.0f - fSlow5;
		float fSlow7 = std::cos(fConst4 * static_cast<float>(fHslider2));
		float fSlow8 = 1.0f - fSlow5 * fSlow7;
		float fSlow9 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow8) / zita_rev1_faustpower2_f(fSlow6) + -1.0f));
		float fSlow10 = fSlow8 / fSlow6;
		float fSlow11 = fSlow10 - fSlow9;
		float fSlow12 = static_cast<float>(fHslider3);
		float fSlow13 = std::exp(-(fConst3 / fSlow12)) / fSlow4 + -1.0f;
		float fSlow14 = fSlow4 * (fSlow9 + (1.0f - fSlow10));
		float fSlow15 = std::exp(-(fConst6 / fSlow3));
		float fSlow16 = zita_rev1_faustpower2_f(fSlow15);
		float fSlow17 = 1.0f - fSlow16;
		float fSlow18 = 1.0f - fSlow7 * fSlow16;
		float fSlow19 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow18) / zita_rev1_faustpower2_f(fSlow17) + -1.0f));
		float fSlow20 = fSlow18 / fSlow17;
		float fSlow21 = fSlow20 - fSlow19;
		float fSlow22 = std::exp(-(fConst6 / fSlow12)) / fSlow15 + -1.0f;
		float fSlow23 = fSlow15 * (fSlow19 + (1.0f - fSlow20));
		float fSlow24 = std::exp(-(fConst8 / fSlow3));
		float fSlow25 = zita_rev1_faustpower2_f(fSlow24);
		float fSlow26 = 1.0f - fSlow25;
		float fSlow27 = 1.0f - fSlow7 * fSlow25;
		float fSlow28 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow27) / zita_rev1_faustpower2_f(fSlow26) + -1.0f));
		float fSlow29 = fSlow27 / fSlow26;
		float fSlow30 = fSlow29 - fSlow28;
		float fSlow31 = std::exp(-(fConst8 / fSlow12)) / fSlow24 + -1.0f;
		float fSlow32 = fSlow24 * (fSlow28 + (1.0f - fSlow29));
		float fSlow33 = std::exp(-(fConst10 / fSlow3));
		float fSlow34 = zita_rev1_faustpower2_f(fSlow33);
		float fSlow35 = 1.0f - fSlow34;
		float fSlow36 = 1.0f - fSlow7 * fSlow34;
		float fSlow37 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow36) / zita_rev1_faustpower2_f(fSlow35) + -1.0f));
		float fSlow38 = fSlow36 / fSlow35;
		float fSlow39 = fSlow38 - fSlow37;
		float fSlow40 = std::exp(-(fConst10 / fSlow12)) / fSlow33 + -1.0f;
		float fSlow41 = fSlow33 * (fSlow37 + (1.0f - fSlow38));
		float fSlow42 = std::exp(-(fConst12 / fSlow3));
		float fSlow43 = zita_rev1_faustpower2_f(fSlow42);
		float fSlow44 = 1.0f - fSlow43;
		float fSlow45 = 1.0f - fSlow7 * fSlow43;
		float fSlow46 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow45) / zita_rev1_faustpower2_f(fSlow44) + -1.0f));
		float fSlow47 = fSlow45 / fSlow44;
		float fSlow48 = fSlow47 - fSlow46;
		float fSlow49 = std::exp(-(fConst12 / fSlow12)) / fSlow42 + -1.0f;
		float fSlow50 = fSlow42 * (fSlow46 + (1.0f - fSlow47));
		int iSlow51 = static_cast<int>(std::min<float>(8192.0f, std::max<float>(0.0f, fConst15 * static_cast<float>(fHslider4))));
		float fSlow52 = std::exp(-(fConst20 / fSlow3));
		float fSlow53 = zita_rev1_faustpower2_f(fSlow52);
		float fSlow54 = 1.0f - fSlow53;
		float fSlow55 = 1.0f - fSlow7 * fSlow53;
		float fSlow56 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow55) / zita_rev1_faustpower2_f(fSlow54) + -1.0f));
		float fSlow57 = fSlow55 / fSlow54;
		float fSlow58 = fSlow57 - fSlow56;
		float fSlow59 = std::exp(-(fConst20 / fSlow12)) / fSlow52 + -1.0f;
		float fSlow60 = fSlow52 * (fSlow56 + (1.0f - fSlow57));
		float fSlow61 = std::exp(-(fConst29 / fSlow3));
		float fSlow62 = zita_rev1_faustpower2_f(fSlow61);
		float fSlow63 = 1.0f - fSlow62;
		float fSlow64 = 1.0f - fSlow7 * fSlow62;
		float fSlow65 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow64) / zita_rev1_faustpower2_f(fSlow63) + -1.0f));
		float fSlow66 = fSlow64 / fSlow63;
		float fSlow67 = fSlow66 - fSlow65;
		float fSlow68 = std::exp(-(fConst29 / fSlow12)) / fSlow61 + -1.0f;
		float fSlow69 = fSlow61 * (fSlow65 + (1.0f - fSlow66));
		float fSlow70 = std::exp(-(fConst33 / fSlow3));
		float fSlow71 = zita_rev1_faustpower2_f(fSlow70);
		float fSlow72 = 1.0f - fSlow71;
		float fSlow73 = 1.0f - fSlow7 * fSlow71;
		float fSlow74 = std::sqrt(std::max<float>(0.0f, zita_rev1_faustpower2_f(fSlow73) / zita_rev1_faustpower2_f(fSlow72) + -1.0f));
		float fSlow75 = fSlow73 / fSlow72;
		float fSlow76 = fSlow75 - fSlow74;
		float fSlow77 = std::exp(-(fConst33 / fSlow12)) / fSlow70 + -1.0f;
		float fSlow78 = fSlow70 * (fSlow74 + (1.0f - fSlow75));
		float fSlow79 = static_cast<float>(fHslider5);
		float fSlow80 = 0.0037f * fSlow79;
		float fSlow81 = 1.0f - 0.01f * fSlow79;
		for (int i0 = 0; i0 < count; i0 = faust_wrap_add(i0, 1)) {
			fRec0[0] = -(fSlow2 * (fSlow1 * fRec0[1] - (fRec35[1] + fRec35[2])));
			fRec1[0] = -(fSlow2 * (fSlow1 * fRec1[1] - (fRec36[1] + fRec36[2])));
			fRec2[0] = -(fSlow2 * (fSlow1 * fRec2[1] - (fRec38[1] + fRec38[2])));
			fRec3[0] = -(fSlow2 * (fSlow1 * fRec3[1] - (fRec33[1] + fRec33[2])));
			fRec4[0] = fSlow14 * (fRec38[1] + fSlow13 * fRec2[0]) + fSlow11 * fRec4[1];
			fRec5[0] = fSlow23 * (fRec35[1] + fSlow22 * fRec0[0]) + fSlow21 * fRec5[1];
			fRec6[0] = -(fSlow2 * (fSlow1 * fRec6[1] - (fRec39[1] + fRec39[2])));
			fRec7[0] = -(fSlow2 * (fSlow1 * fRec7[1] - (fRec37[1] + fRec37[2])));
			fRec8[0] = fSlow32 * (fRec37[1] + fSlow31 * fRec7[0]) + fSlow30 * fRec8[1];
			fRec9[0] = fSlow41 * (fRec36[1] + fSlow40 * fRec1[0]) + fSlow39 * fRec9[1];
			fRec10[0] = -(fSlow2 * (fSlow1 * fRec10[1] - (fRec32[1] + fRec32[2])));
			fRec11[0] = -(fSlow2 * (fSlow1 * fRec11[1] - (fRec34[1] + fRec34[2])));
			fRec12[0] = fSlow50 * (fRec33[1] + fSlow49 * fRec3[0]) + fSlow48 * fRec12[1];
			fVec0[IOTA0 & 65535] = 1e-20f + 0.35355338f * fRec8[0];
			float fTemp0 = static_cast<float>(input1[i0]);
			fVec1[IOTA0 & 16383] = fTemp0;
			float fTemp1 = 0.3f * fVec1[(faust_wrap_sub(IOTA0, iSlow51)) & 16383];
			float fTemp2 = fTemp1 + 0.6f * fRec13[1] + fVec0[(faust_wrap_sub(IOTA0, iConst14)) & 65535];
			fVec2[IOTA0 & 8191] = fTemp2;
			fRec13[0] = fVec2[(faust_wrap_sub(IOTA0, iConst16)) & 8191];
			float fRec14 = -(0.6f * fTemp2);
			fVec3[IOTA0 & 65535] = 1e-20f + 0.35355338f * fRec9[0];
			float fTemp3 = static_cast<float>(input0[i0]);
			fVec4[IOTA0 & 16383] = fTemp3;
			float fTemp4 = 0.3f * fVec4[(faust_wrap_sub(IOTA0, iSlow51)) & 16383];
			float fTemp5 = fTemp4 - 0.6f * fRec17[1] + fVec3[(faust_wrap_sub(IOTA0, iConst18)) & 65535];
			fVec5[IOTA0 & 8191] = fTemp5;
			float fRec15 = 0.6f * fTemp5;
			fRec16[0] = fSlow60 * (fRec39[1] + fSlow59 * fRec6[0]) + fSlow58 * fRec16[1];
			fRec17[0] = fVec5[(faust_wrap_sub(IOTA0, iConst21)) & 8191];
			fVec6[IOTA0 & 65535] = 1e-20f + 0.35355338f * fRec5[0];
			float fTemp6 = 0.6f * fRec18[1] + fVec6[(faust_wrap_sub(IOTA0, iConst23)) & 65535];
			fVec7[IOTA0 & 8191] = fTemp6 - fTemp1;
			fRec18[0] = fVec7[(faust_wrap_sub(IOTA0, iConst24)) & 8191];
			fVec8[IOTA0 & 32767] = 1e-20f + 0.35355338f * fRec12[0];
			float fTemp7 = fTemp4 - 0.6f * fRec20[1] + fVec8[(faust_wrap_sub(IOTA0, iConst26)) & 32767];
			fVec9[IOTA0 & 4095] = fTemp7;
			float fRec19 = 0.6f * fTemp7;
			fRec20[0] = fVec9[(faust_wrap_sub(IOTA0, iConst27)) & 4095];
			fRec21[0] = fSlow69 * (fRec34[1] + fSlow68 * fRec11[0]) + fSlow67 * fRec21[1];
			fVec10[IOTA0 & 32767] = 1e-20f + 0.35355338f * fRec16[0];
			float fTemp8 = fVec10[(faust_wrap_sub(IOTA0, iConst31)) & 32767] - (fTemp4 + 0.6f * fRec26[1]);
			fVec11[IOTA0 & 4095] = fTemp8;
			float fRec22 = 0.6f * fTemp8;
			fRec23[0] = fSlow78 * (fRec32[1] + fSlow77 * fRec10[0]) + fSlow76 * fRec23[1];
			fVec12[IOTA0 & 65535] = 1e-20f + 0.35355338f * fRec4[0];
			float fTemp9 = 0.6f * fRec24[1] + fVec12[(faust_wrap_sub(IOTA0, iConst35)) & 65535];
			fVec13[IOTA0 & 4095] = fTemp9 - fTemp1;
			fRec24[0] = fVec13[(faust_wrap_sub(IOTA0, iConst36)) & 4095];
			float fRec25 = 0.6f * (fTemp1 - fTemp6);
			fRec26[0] = fVec11[(faust_wrap_sub(IOTA0, iConst37)) & 4095];
			float fRec27 = 0.6f * (fTemp1 - fTemp9);
			fVec14[IOTA0 & 65535] = 1e-20f + 0.35355338f * fRec23[0];
			float fTemp10 = fTemp1 + 0.6f * fRec28[1] + fVec14[(faust_wrap_sub(IOTA0, iConst39)) & 65535];
			fVec15[IOTA0 & 8191] = fTemp10;
			fRec28[0] = fVec15[(faust_wrap_sub(IOTA0, iConst40)) & 8191];
			float fRec29 = -(0.6f * fTemp10);
			fVec16[IOTA0 & 32767] = 1e-20f + 0.35355338f * fRec21[0];
			float fTemp11 = fVec16[(faust_wrap_sub(IOTA0, iConst42)) & 32767] - (fTemp4 + 0.6f * fRec31[1]);
			fVec17[IOTA0 & 8191] = fTemp11;
			float fRec30 = 0.6f * fTemp11;
			fRec31[0] = fVec17[(faust_wrap_sub(IOTA0, iConst43)) & 8191];
			float fTemp12 = fRec13[1] + fRec18[1];
			fRec32[0] = fRec22 + fRec30 + fRec15 + fRec19 + fRec31[1] + fRec26[1] + fRec17[1] + fRec20[1] - (fRec27 + fRec25 + fRec14 + fRec29 + fRec28[1] + fRec24[1] + fTemp12);
			float fTemp13 = fRec13[1] + fRec17[1];
			float fTemp14 = fRec18[1] + fTemp13;
			fRec33[0] = fRec27 + fRec25 + fRec14 + fRec29 + fRec22 + fRec30 + fRec15 + fRec19 + fRec31[1] + fRec28[1] + fRec24[1] + fRec26[1] + fRec20[1] + fTemp14;
			fRec34[0] = fRec14 + fRec29 + fRec15 + fRec19 + fRec28[1] + fRec20[1] + fTemp13 - (fRec27 + fRec25 + fRec22 + fRec30 + fRec31[1] + fRec24[1] + fRec18[1] + fRec26[1]);
			float fTemp15 = fRec17[1] + fRec18[1];
			fRec35[0] = fRec27 + fRec25 + fRec15 + fRec19 + fRec24[1] + fRec20[1] + fTemp15 - (fRec14 + fRec29 + fRec22 + fRec30 + fRec31[1] + fRec28[1] + fRec13[1] + fRec26[1]);
			fRec36[0] = fRec25 + fRec29 + fRec30 + fRec19 + fRec31[1] + fRec28[1] + fRec18[1] + fRec20[1] - (fRec27 + fRec14 + fRec22 + fRec15 + fRec24[1] + fRec26[1] + fTemp13);
			fRec37[0] = fRec27 + fRec14 + fRec30 + fRec19 + fRec31[1] + fRec24[1] + fRec13[1] + fRec20[1] - (fRec25 + fRec29 + fRec22 + fRec15 + fRec28[1] + fRec26[1] + fTemp15);
			fRec38[0] = fRec25 + fRec14 + fRec22 + fRec19 + fRec26[1] + fRec20[1] + fTemp12 - (fRec27 + fRec29 + fRec30 + fRec15 + fRec31[1] + fRec28[1] + fRec17[1] + fRec24[1]);
			fRec39[0] = fRec27 + fRec29 + fRec22 + fRec19 + fRec28[1] + fRec24[1] + fRec20[1] + fRec26[1] - (fRec25 + fRec14 + fRec30 + fRec15 + fRec31[1] + fTemp14);
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow81 * fTemp3 + fSlow80 * (fRec32[0] + fRec34[0]));
			output1[i0] = static_cast<FAUSTFLOAT>(fSlow81 * fTemp0 + fSlow80 * (fRec32[0] - fRec34[0]));
			fRec0[1] = fRec0[0];
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec6[1] = fRec6[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec9[1] = fRec9[0];
			fRec10[1] = fRec10[0];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
			IOTA0 = faust_wrap_add(IOTA0, 1);
			fRec13[1] = fRec13[0];
			fRec16[1] = fRec16[0];
			fRec17[1] = fRec17[0];
			fRec18[1] = fRec18[0];
			fRec20[1] = fRec20[0];
			fRec21[1] = fRec21[0];
			fRec23[1] = fRec23[0];
			fRec24[1] = fRec24[0];
			fRec26[1] = fRec26[0];
			fRec28[1] = fRec28[0];
			fRec31[1] = fRec31[0];
			fRec32[2] = fRec32[1];
			fRec32[1] = fRec32[0];
			fRec33[2] = fRec33[1];
			fRec33[1] = fRec33[0];
			fRec34[2] = fRec34[1];
			fRec34[1] = fRec34[0];
			fRec35[2] = fRec35[1];
			fRec35[1] = fRec35[0];
			fRec36[2] = fRec36[1];
			fRec36[1] = fRec36[0];
			fRec37[2] = fRec37[1];
			fRec37[1] = fRec37[0];
			fRec38[2] = fRec38[1];
			fRec38[1] = fRec38[0];
			fRec39[2] = fRec39[1];
			fRec39[1] = fRec39[0];
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
  std::unique_ptr<zrythm::plugins::faust::dsp> create_zita_rev1 ()
  {
    return std::make_unique<zita_rev1> ();
  }

  /**
   * Calls the static classInit() for the given sample rate.
   *
   * classInit() fills static tables shared by all instances of the class, so it
   * is only called when needed: on first use, or when the sample rate changes
   * (processing is stopped during preparation, so this cannot race with
   * compute()).
   */
  void class_init_zita_rev1 (int sample_rate)
  {
    static std::mutex mutex;
    static int        initialized_sample_rate = 0;

    const std::lock_guard lock (mutex);
    if (initialized_sample_rate != sample_rate)
      {
        zita_rev1::classInit (sample_rate);
        initialized_sample_rate = sample_rate;
      }
  }

} // namespace zrythm_faust

/******************** END ARCHITECTURE SECTION (part 2/2) ****************/

#endif
