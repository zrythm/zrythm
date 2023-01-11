/* ------------------------------------------------------------
author: "Zrythm DAW"
copyright: "© 2022 Alexandros Theodotou"
license: "AGPL-3.0-or-later"
name: "Parametric EQ"
version: "1.0"
Code generated with Faust 2.54.9 (https://faust.grame.fr)
Compilation options: -a /usr/share/faust/lv2.cpp -lang cpp -i -cn parametric_eq -es 1 -mcd 16 -single -ftz 0 -vec -lv 0 -vs 32
------------------------------------------------------------ */

#ifndef  __parametric_eq_H__
#define  __parametric_eq_H__

/************************************************************************
 ************************************************************************
    FAUST Architecture File
    Copyright (C) 2009-2016 Albert Graef <aggraef@gmail.com>
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the GNU C Library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA.
 ************************************************************************
 ************************************************************************/

/* LV2 architecture for Faust synths. */

/* NOTE: This requires one of the Boost headers (boost/circular_buffer.hpp),
   so to compile Faust programs created with this architecture you need to
   have at least the Boost headers installed somewhere on your include path
   (the Boost libraries aren't needed). */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <list>
#include <map>
#include <set>

// generic Faust dsp and UI classes
/************************** BEGIN dsp.h ********************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __dsp__
#define __dsp__

#include <string>
#include <vector>

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ***************************************************************************/

#ifndef __export__
#define __export__

#define FAUSTVERSION "2.54.9"

// Use FAUST_API for code that is part of the external API but is also compiled in faust and libfaust
// Use LIBFAUST_API for code that is compiled in faust and libfaust

#ifdef _WIN32
    #pragma warning (disable: 4251)
    #ifdef FAUST_EXE
        #define FAUST_API
        #define LIBFAUST_API
    #elif FAUST_LIB
        #define FAUST_API __declspec(dllexport)
        #define LIBFAUST_API __declspec(dllexport)
    #else
        #define FAUST_API
        #define LIBFAUST_API 
    #endif
#else
    #ifdef FAUST_EXE
        #define FAUST_API
        #define LIBFAUST_API
    #else
        #define FAUST_API __attribute__((visibility("default")))
        #define LIBFAUST_API __attribute__((visibility("default")))
    #endif
#endif

#endif

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

struct FAUST_API UI;
struct FAUST_API Meta;

/**
 * DSP memory manager.
 */

struct FAUST_API dsp_memory_manager {
    
    virtual ~dsp_memory_manager() {}
    
    /**
     * Inform the Memory Manager with the number of expected memory zones.
     * @param count - the number of expected memory zones
     */
    virtual void begin(size_t /*count*/) {}
    
    /**
     * Give the Memory Manager information on a given memory zone.
     * @param size - the size in bytes of the memory zone
     * @param reads - the number of Read access to the zone used to compute one frame
     * @param writes - the number of Write access to the zone used to compute one frame
     */
    virtual void info(size_t /*size*/, size_t /*reads*/, size_t /*writes*/) {}

    /**
     * Inform the Memory Manager that all memory zones have been described,
     * to possibly start a 'compute the best allocation strategy' step.
     */
    virtual void end() {}
    
    /**
     * Allocate a memory zone.
     * @param size - the memory zone size in bytes
     */
    virtual void* allocate(size_t size) = 0;
    
    /**
     * Destroy a memory zone.
     * @param ptr - the memory zone pointer to be deallocated
     */
    virtual void destroy(void* ptr) = 0;
    
};

/**
* Signal processor definition.
*/

class FAUST_API dsp {

    public:

        dsp() {}
        virtual ~dsp() {}

        /* Return instance number of audio inputs */
        virtual int getNumInputs() = 0;
    
        /* Return instance number of audio outputs */
        virtual int getNumOutputs() = 0;
    
        /**
         * Trigger the ui_interface parameter with instance specific calls
         * to 'openTabBox', 'addButton', 'addVerticalSlider'... in order to build the UI.
         *
         * @param ui_interface - the user interface builder
         */
        virtual void buildUserInterface(UI* ui_interface) = 0;
    
        /* Return the sample rate currently used by the instance */
        virtual int getSampleRate() = 0;
    
        /**
         * Global init, calls the following methods:
         * - static class 'classInit': static tables initialization
         * - 'instanceInit': constants and instance state initialization
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void init(int sample_rate) = 0;

        /**
         * Init instance state
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void instanceInit(int sample_rate) = 0;
    
        /**
         * Init instance constant state
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void instanceConstants(int sample_rate) = 0;
    
        /* Init default control parameters values */
        virtual void instanceResetUserInterface() = 0;
    
        /* Init instance state (like delay lines...) but keep the control parameter values */
        virtual void instanceClear() = 0;
 
        /**
         * Return a clone of the instance.
         *
         * @return a copy of the instance on success, otherwise a null pointer.
         */
        virtual dsp* clone() = 0;
    
        /**
         * Trigger the Meta* parameter with instance specific calls to 'declare' (key, value) metadata.
         *
         * @param m - the Meta* meta user
         */
        virtual void metadata(Meta* m) = 0;
    
        /**
         * DSP instance computation, to be called with successive in/out audio buffers.
         *
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
    
        /**
         * DSP instance computation: alternative method to be used by subclasses.
         *
         * @param date_usec - the timestamp in microsec given by audio driver.
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         *
         */
        virtual void compute(double /*date_usec*/, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { compute(count, inputs, outputs); }
       
};

/**
 * Generic DSP decorator.
 */

class FAUST_API decorator_dsp : public dsp {

    protected:

        dsp* fDSP;

    public:

        decorator_dsp(dsp* dsp = nullptr):fDSP(dsp) {}
        virtual ~decorator_dsp() { delete fDSP; }

        virtual int getNumInputs() { return fDSP->getNumInputs(); }
        virtual int getNumOutputs() { return fDSP->getNumOutputs(); }
        virtual void buildUserInterface(UI* ui_interface) { fDSP->buildUserInterface(ui_interface); }
        virtual int getSampleRate() { return fDSP->getSampleRate(); }
        virtual void init(int sample_rate) { fDSP->init(sample_rate); }
        virtual void instanceInit(int sample_rate) { fDSP->instanceInit(sample_rate); }
        virtual void instanceConstants(int sample_rate) { fDSP->instanceConstants(sample_rate); }
        virtual void instanceResetUserInterface() { fDSP->instanceResetUserInterface(); }
        virtual void instanceClear() { fDSP->instanceClear(); }
        virtual decorator_dsp* clone() { return new decorator_dsp(fDSP->clone()); }
        virtual void metadata(Meta* m) { fDSP->metadata(m); }
        // Beware: subclasses usually have to overload the two 'compute' methods
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(count, inputs, outputs); }
        virtual void compute(double date_usec, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { fDSP->compute(date_usec, count, inputs, outputs); }
    
};

/**
 * DSP factory class, used with LLVM and Interpreter backends
 * to create DSP instances from a compiled DSP program.
 */

class FAUST_API dsp_factory {
    
    protected:
    
        // So that to force sub-classes to use deleteDSPFactory(dsp_factory* factory);
        virtual ~dsp_factory() {}
    
    public:
    
        virtual std::string getName() = 0;
        virtual std::string getSHAKey() = 0;
        virtual std::string getDSPCode() = 0;
        virtual std::string getCompileOptions() = 0;
        virtual std::vector<std::string> getLibraryList() = 0;
        virtual std::vector<std::string> getIncludePathnames() = 0;
        virtual std::vector<std::string> getWarningMessages() = 0;
    
        virtual dsp* createDSPInstance() = 0;
    
        virtual void setMemoryManager(dsp_memory_manager* manager) = 0;
        virtual dsp_memory_manager* getMemoryManager() = 0;
    
};

// Denormal handling

#if defined (__SSE__)
#include <xmmintrin.h>
#endif

class FAUST_API ScopedNoDenormals {
    
    private:
    
        intptr_t fpsr = 0;
        
        void setFpStatusRegister(intptr_t fpsr_aux) noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            asm volatile("msr fpcr, %0" : : "ri" (fpsr_aux));
        #elif defined (__SSE__)
            // The volatile keyword here is needed to workaround a bug in AppleClang 13.0
            // which aggressively optimises away the variable otherwise
            volatile uint32_t fpsr_w = static_cast<uint32_t>(fpsr_aux);
            _mm_setcsr(fpsr_w);
        #endif
        }
        
        void getFpStatusRegister() noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            asm volatile("mrs %0, fpcr" : "=r" (fpsr));
        #elif defined (__SSE__)
            fpsr = static_cast<intptr_t>(_mm_getcsr());
        #endif
        }
    
    public:
    
        ScopedNoDenormals() noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            intptr_t mask = (1 << 24 /* FZ */);
        #elif defined (__SSE__)
        #if defined (__SSE2__)
            intptr_t mask = 0x8040;
        #else
            intptr_t mask = 0x8000;
        #endif
        #else
            intptr_t mask = 0x0000;
        #endif
            getFpStatusRegister();
            setFpStatusRegister(fpsr | mask);
        }
        
        ~ScopedNoDenormals() noexcept
        {
            setFpStatusRegister(fpsr);
        }

};

#define AVOIDDENORMALS ScopedNoDenormals ftz_scope;

#endif

/************************** END dsp.h **************************/
/************************** BEGIN UI.h *****************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ********************************************************************/

#ifndef __UI_H__
#define __UI_H__


#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

/*******************************************************************************
 * UI : Faust DSP User Interface
 * User Interface as expected by the buildUserInterface() method of a DSP.
 * This abstract class contains only the method that the Faust compiler can
 * generate to describe a DSP user interface.
 ******************************************************************************/

struct Soundfile;

template <typename REAL>
struct FAUST_API UIReal {
    
    UIReal() {}
    virtual ~UIReal() {}
    
    // -- widget's layouts
    
    virtual void openTabBox(const char* label) = 0;
    virtual void openHorizontalBox(const char* label) = 0;
    virtual void openVerticalBox(const char* label) = 0;
    virtual void closeBox() = 0;
    
    // -- active widgets
    
    virtual void addButton(const char* label, REAL* zone) = 0;
    virtual void addCheckButton(const char* label, REAL* zone) = 0;
    virtual void addVerticalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addHorizontalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addNumEntry(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    
    // -- passive widgets
    
    virtual void addHorizontalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;
    virtual void addVerticalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;
    
    // -- soundfiles
    
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) = 0;
    
    // -- metadata declarations
    
    virtual void declare(REAL* /*zone*/, const char* /*key*/, const char* /*val*/) {}

    // To be used by LLVM client
    virtual int sizeOfFAUSTFLOAT() { return sizeof(FAUSTFLOAT); }
};

struct FAUST_API UI : public UIReal<FAUSTFLOAT> {
    UI() {}
    virtual ~UI() {}
};

#endif
/**************************  END  UI.h **************************/

using namespace std;

typedef pair<const char*,const char*> strpair;

struct Meta : std::map<const char*, const char*>
{
  void declare(const char *key, const char *value)
  {
    (*this)[key] = value;
  }
  const char* get(const char *key, const char *def)
  {
    if (this->find(key) != this->end())
      return (*this)[key];
    else
      return def;
  }
};

/******************************************************************************
*******************************************************************************

		       VECTOR INTRINSICS

*******************************************************************************
*******************************************************************************/


/***************************************************************************
   LV2 UI interface
 ***************************************************************************/

enum ui_elem_type_t {
  UI_BUTTON, UI_CHECK_BUTTON,
  UI_V_SLIDER, UI_H_SLIDER, UI_NUM_ENTRY,
  UI_V_BARGRAPH, UI_H_BARGRAPH,
  UI_END_GROUP, UI_V_GROUP, UI_H_GROUP, UI_T_GROUP
};

struct ui_elem_t {
  ui_elem_type_t type;
  const char *label;
  int port;
  float *zone;
  void *ref;
  float init, min, max, step;
};

class LV2UI : public UI
{
public:
  bool is_instr;
  int nelems, nports;
  ui_elem_t *elems;
  map< int, list<strpair> > metadata;

  LV2UI(int maxvoices = 0);
  virtual ~LV2UI();

protected:
  void add_elem(ui_elem_type_t type, const char *label = NULL);
  void add_elem(ui_elem_type_t type, const char *label, float *zone);
  void add_elem(ui_elem_type_t type, const char *label, float *zone,
		float init, float min, float max, float step);
  void add_elem(ui_elem_type_t type, const char *label, float *zone,
		float min, float max);

  bool have_freq, have_gain, have_gate;
  bool is_voice_ctrl(const char *label);

public:
  virtual void addButton(const char* label, float* zone);
  virtual void addCheckButton(const char* label, float* zone);
  virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step);
  virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step);
  virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step);

  virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max);
  virtual void addVerticalBargraph(const char* label, float* zone, float min, float max);

  virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {}

  virtual void openTabBox(const char* label);
  virtual void openHorizontalBox(const char* label);
  virtual void openVerticalBox(const char* label);
  virtual void closeBox();

  virtual void run();

  virtual void declare(float* zone, const char* key, const char* value);
};

LV2UI::LV2UI(int maxvoices)
{
  is_instr = maxvoices>0;
  have_freq = have_gain = have_gate = false;
  nelems = nports = 0;
  elems = NULL;
}

LV2UI::~LV2UI()
{
  if (elems) free(elems);
}

void LV2UI::declare(float* zone, const char* key, const char* value)
{
  map< int, list<strpair> >::iterator it = metadata.find(nelems);
  if (it != metadata.end())
    it->second.push_back(strpair(key, value));
  else
    metadata[nelems] = list<strpair>(1, strpair(key, value));
}

inline void LV2UI::add_elem(ui_elem_type_t type, const char *label)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  elems[nelems].type = type;
  elems[nelems].label = label;
  elems[nelems].port = -1;
  elems[nelems].zone = NULL;
  elems[nelems].ref = NULL;
  elems[nelems].init = 0.0;
  elems[nelems].min = 0.0;
  elems[nelems].max = 0.0;
  elems[nelems].step = 0.0;
  nelems++;
}

#define portno(label) (is_voice_ctrl(label)?-1:nports++)

inline void LV2UI::add_elem(ui_elem_type_t type, const char *label, float *zone)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  elems[nelems].type = type;
  elems[nelems].label = label;
  elems[nelems].port = portno(label);
  elems[nelems].zone = zone;
  elems[nelems].ref = NULL;
  elems[nelems].init = 0.0;
  elems[nelems].min = 0.0;
  elems[nelems].max = 0.0;
  elems[nelems].step = 0.0;
  nelems++;
}

inline void LV2UI::add_elem(ui_elem_type_t type, const char *label, float *zone,
			     float init, float min, float max, float step)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  elems[nelems].type = type;
  elems[nelems].label = label;
  elems[nelems].port = portno(label);
  elems[nelems].zone = zone;
  elems[nelems].ref = NULL;
  elems[nelems].init = init;
  elems[nelems].min = min;
  elems[nelems].max = max;
  elems[nelems].step = step;
  nelems++;
}

inline void LV2UI::add_elem(ui_elem_type_t type, const char *label, float *zone,
			     float min, float max)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  elems[nelems].type = type;
  elems[nelems].label = label;
  elems[nelems].port = portno(label);
  elems[nelems].zone = zone;
  elems[nelems].ref = NULL;
  elems[nelems].init = 0.0;
  elems[nelems].min = min;
  elems[nelems].max = max;
  elems[nelems].step = 0.0;
  nelems++;
}

inline bool LV2UI::is_voice_ctrl(const char *label)
{
  if (!is_instr)
    return false;
  else if (!have_freq && !strcmp(label, "freq"))
    return (have_freq = true);
  else if (!have_gain && !strcmp(label, "gain"))
    return (have_gain = true);
  else if (!have_gate && !strcmp(label, "gate"))
    return (have_gate = true);
  else
    return false;
}

void LV2UI::addButton(const char* label, float* zone)
{ add_elem(UI_BUTTON, label, zone); }
void LV2UI::addCheckButton(const char* label, float* zone)
{ add_elem(UI_CHECK_BUTTON, label, zone); }
void LV2UI::addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_V_SLIDER, label, zone, init, min, max, step); }
void LV2UI::addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_H_SLIDER, label, zone, init, min, max, step); }
void LV2UI::addNumEntry(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_NUM_ENTRY, label, zone, init, min, max, step); }

void LV2UI::addHorizontalBargraph(const char* label, float* zone, float min, float max)
{ add_elem(UI_H_BARGRAPH, label, zone, min, max); }
void LV2UI::addVerticalBargraph(const char* label, float* zone, float min, float max)
{ add_elem(UI_V_BARGRAPH, label, zone, min, max); }

void LV2UI::openTabBox(const char* label)
{ add_elem(UI_T_GROUP, label); }
void LV2UI::openHorizontalBox(const char* label)
{ add_elem(UI_H_GROUP, label); }
void LV2UI::openVerticalBox(const char* label)
{ add_elem(UI_V_GROUP, label); }
void LV2UI::closeBox()
{ add_elem(UI_END_GROUP); }

void LV2UI::run() {}

//----------------------------------------------------------------------------
//  FAUST generated signal processor
//----------------------------------------------------------------------------

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

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
	
	float fYec0_perm[4];
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst1;
	float fRec4_perm[4];
	float fRec3_perm[4];
	float fRec6_perm[4];
	float fRec5_perm[4];
	FAUSTFLOAT fHslider1;
	float fRec7_perm[4];
	FAUSTFLOAT fHslider2;
	float fConst2;
	FAUSTFLOAT fHslider3;
	FAUSTFLOAT fHslider4;
	float fRec2_perm[4];
	float fYec1_perm[4];
	FAUSTFLOAT fHslider5;
	float fRec1_perm[4];
	float fRec0_perm[4];
	float fRec9_perm[4];
	float fRec8_perm[4];
	FAUSTFLOAT fHslider6;
	float fYec2_perm[4];
	float fRec14_perm[4];
	float fRec13_perm[4];
	float fRec16_perm[4];
	float fRec15_perm[4];
	float fRec12_perm[4];
	float fYec3_perm[4];
	float fRec11_perm[4];
	float fRec10_perm[4];
	float fRec18_perm[4];
	float fRec17_perm[4];
	
 public:
	
	void metadata(Meta* m) { 
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "0.2");
		m->declare("author", "Zrythm DAW");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "0.9");
		m->declare("compile_options", "-a /usr/share/faust/lv2.cpp -lang cpp -i -cn parametric_eq -es 1 -mcd 16 -single -ftz 0 -vec -lv 0 -vs 32");
		m->declare("copyright", "© 2022 Alexandros Theodotou");
		m->declare("description", "Parametric equalizer with high/low shelves");
		m->declare("filename", "parametric_eq.dsp");
		m->declare("filters.lib/filterbank:author", "Julius O. Smith III");
		m->declare("filters.lib/filterbank:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/filterbank:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/highshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/highshelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/low_shelf:author", "Julius O. Smith III");
		m->declare("filters.lib/low_shelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/low_shelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowshelf:author", "Julius O. Smith III");
		m->declare("filters.lib/lowshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowshelf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/peak_eq:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/peak_eq_cq:author", "Julius O. Smith III");
		m->declare("filters.lib/peak_eq_cq:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/peak_eq_cq:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "0.3");
		m->declare("license", "AGPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.5");
		m->declare("name", "Parametric EQ");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "0.3");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "0.3");
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
		float fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = 3.1415927f / fConst0;
		fConst2 = 6.2831855f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(2e+02f);
		fHslider1 = FAUSTFLOAT(7.2e+02f);
		fHslider2 = FAUSTFLOAT(0.0f);
		fHslider3 = FAUSTFLOAT(1.0f);
		fHslider4 = FAUSTFLOAT(0.0f);
		fHslider5 = FAUSTFLOAT(8e+03f);
		fHslider6 = FAUSTFLOAT(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 4; l0 = l0 + 1) {
			fYec0_perm[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 4; l1 = l1 + 1) {
			fRec4_perm[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 4; l2 = l2 + 1) {
			fRec3_perm[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 4; l3 = l3 + 1) {
			fRec6_perm[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 4; l4 = l4 + 1) {
			fRec5_perm[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 4; l5 = l5 + 1) {
			fRec7_perm[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 4; l6 = l6 + 1) {
			fRec2_perm[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 4; l7 = l7 + 1) {
			fYec1_perm[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 4; l8 = l8 + 1) {
			fRec1_perm[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 4; l9 = l9 + 1) {
			fRec0_perm[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 4; l10 = l10 + 1) {
			fRec9_perm[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 4; l11 = l11 + 1) {
			fRec8_perm[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 4; l12 = l12 + 1) {
			fYec2_perm[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 4; l13 = l13 + 1) {
			fRec14_perm[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 4; l14 = l14 + 1) {
			fRec13_perm[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 4; l15 = l15 + 1) {
			fRec16_perm[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 4; l16 = l16 + 1) {
			fRec15_perm[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 4; l17 = l17 + 1) {
			fRec12_perm[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 4; l18 = l18 + 1) {
			fYec3_perm[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 4; l19 = l19 + 1) {
			fRec11_perm[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 4; l20 = l20 + 1) {
			fRec10_perm[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 4; l21 = l21 + 1) {
			fRec18_perm[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 4; l22 = l22 + 1) {
			fRec17_perm[l22] = 0.0f;
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
		return new parametric_eq();
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
		ui_interface->declare(&fHslider4, "0", "");
		ui_interface->declare(&fHslider4, "style", "knob");
		ui_interface->declare(&fHslider4, "tooltip", "Amount of low-frequency boost or cut in decibels");
		ui_interface->declare(&fHslider4, "unit", "dB");
		ui_interface->addHorizontalSlider("Low Shelf Gain", &fHslider4, FAUSTFLOAT(0.0f), FAUSTFLOAT(-4e+01f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(0.1f));
		ui_interface->declare(&fHslider0, "1", "");
		ui_interface->declare(&fHslider0, "scale", "log");
		ui_interface->declare(&fHslider0, "style", "knob");
		ui_interface->declare(&fHslider0, "tooltip", "Transition-frequency from boost (cut) to unity gain");
		ui_interface->declare(&fHslider0, "unit", "Hz");
		ui_interface->addHorizontalSlider("Low Shelf Frequency", &fHslider0, FAUSTFLOAT(2e+02f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(1.0f));
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
		ui_interface->declare(&fHslider5, "1", "");
		ui_interface->declare(&fHslider5, "scale", "log");
		ui_interface->declare(&fHslider5, "style", "knob");
		ui_interface->declare(&fHslider5, "tooltip", "Transition-frequency from boost (cut) to unity gain");
		ui_interface->declare(&fHslider5, "unit", "Hz");
		ui_interface->addHorizontalSlider("High Shelf Frequency", &fHslider5, FAUSTFLOAT(8e+03f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(1.8e+04f), FAUSTFLOAT(1.0f));
		ui_interface->closeBox();
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0_ptr = inputs[0];
		FAUSTFLOAT* input1_ptr = inputs[1];
		FAUSTFLOAT* output0_ptr = outputs[0];
		FAUSTFLOAT* output1_ptr = outputs[1];
		float fYec0_tmp[36];
		float* fYec0 = &fYec0_tmp[4];
		float fSlow0 = std::tan(fConst1 * float(fHslider0));
		float fSlow1 = 1.0f / fSlow0;
		float fSlow2 = 1.0f - fSlow1;
		float fSlow3 = fSlow1 + 1.0f;
		float fSlow4 = 1.0f / fSlow3;
		float fSlow5 = 0.0f - 1.0f / (fSlow0 * fSlow3);
		float fRec4_tmp[36];
		float* fRec4 = &fRec4_tmp[4];
		float fSlow6 = parametric_eq_faustpower2_f(fSlow0);
		float fSlow7 = 1.0f / fSlow6;
		float fSlow8 = 2.0f * (1.0f - fSlow7);
		float fSlow9 = (fSlow1 + -1.0f) / fSlow0 + 1.0f;
		float fSlow10 = 1.0f / ((fSlow1 + 1.0f) / fSlow0 + 1.0f);
		float fRec3_tmp[36];
		float* fRec3 = &fRec3_tmp[4];
		float fRec6_tmp[36];
		float* fRec6 = &fRec6_tmp[4];
		float fRec5_tmp[36];
		float* fRec5 = &fRec5_tmp[4];
		float fSlow11 = 0.001f * float(fHslider1);
		float fRec7_tmp[36];
		float* fRec7 = &fRec7_tmp[4];
		float fZec0[32];
		float fSlow12 = float(fHslider2);
		int iSlow13 = fSlow12 > 0.0f;
		float fZec1[32];
		float fSlow14 = float(fHslider3);
		float fSlow15 = fConst1 * (std::pow(1e+01f, 0.05f * std::fabs(fSlow12)) / fSlow14);
		float fZec2[32];
		float fSlow16 = fConst1 / fSlow14;
		float fZec3[32];
		float fZec4[32];
		float fZec5[32];
		float fZec6[32];
		float fZec7[32];
		float fZec8[32];
		float fZec9[32];
		float fSlow17 = std::pow(1e+01f, 0.05f * float(fHslider4));
		float fSlow18 = 0.0f - 2.0f / fSlow6;
		float fRec2_tmp[36];
		float* fRec2 = &fRec2_tmp[4];
		float fZec10[32];
		float fZec11[32];
		float fZec12[32];
		float fYec1_tmp[36];
		float* fYec1 = &fYec1_tmp[4];
		float fSlow19 = std::tan(fConst1 * float(fHslider5));
		float fSlow20 = 1.0f / fSlow19;
		float fSlow21 = 1.0f - fSlow20;
		float fSlow22 = fSlow20 + 1.0f;
		float fSlow23 = 1.0f / fSlow22;
		float fRec1_tmp[36];
		float* fRec1 = &fRec1_tmp[4];
		float fSlow24 = parametric_eq_faustpower2_f(fSlow19);
		float fSlow25 = 1.0f / fSlow24;
		float fSlow26 = 2.0f * (1.0f - fSlow25);
		float fSlow27 = (fSlow20 + -1.0f) / fSlow19 + 1.0f;
		float fSlow28 = 1.0f / ((fSlow20 + 1.0f) / fSlow19 + 1.0f);
		float fRec0_tmp[36];
		float* fRec0 = &fRec0_tmp[4];
		float fSlow29 = 0.0f - 1.0f / (fSlow19 * fSlow22);
		float fRec9_tmp[36];
		float* fRec9 = &fRec9_tmp[4];
		float fRec8_tmp[36];
		float* fRec8 = &fRec8_tmp[4];
		float fSlow30 = 0.0f - 2.0f / fSlow24;
		float fSlow31 = std::pow(1e+01f, 0.05f * float(fHslider6));
		float fYec2_tmp[36];
		float* fYec2 = &fYec2_tmp[4];
		float fRec14_tmp[36];
		float* fRec14 = &fRec14_tmp[4];
		float fRec13_tmp[36];
		float* fRec13 = &fRec13_tmp[4];
		float fRec16_tmp[36];
		float* fRec16 = &fRec16_tmp[4];
		float fRec15_tmp[36];
		float* fRec15 = &fRec15_tmp[4];
		float fZec13[32];
		float fRec12_tmp[36];
		float* fRec12 = &fRec12_tmp[4];
		float fYec3_tmp[36];
		float* fYec3 = &fYec3_tmp[4];
		float fRec11_tmp[36];
		float* fRec11 = &fRec11_tmp[4];
		float fRec10_tmp[36];
		float* fRec10 = &fRec10_tmp[4];
		float fRec18_tmp[36];
		float* fRec18 = &fRec18_tmp[4];
		float fRec17_tmp[36];
		float* fRec17 = &fRec17_tmp[4];
		int vindex = 0;
		/* Main loop */
		for (vindex = 0; vindex <= (count - 32); vindex = vindex + 32) {
			FAUSTFLOAT* input0 = &input0_ptr[vindex];
			FAUSTFLOAT* input1 = &input1_ptr[vindex];
			FAUSTFLOAT* output0 = &output0_ptr[vindex];
			FAUSTFLOAT* output1 = &output1_ptr[vindex];
			int vsize = 32;
			/* Recursive loop 0 */
			/* Pre code */
			for (int j10 = 0; j10 < 4; j10 = j10 + 1) {
				fRec7_tmp[j10] = fRec7_perm[j10];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec7[i] = fSlow11 + 0.999f * fRec7[i - 1];
			}
			/* Post code */
			for (int j11 = 0; j11 < 4; j11 = j11 + 1) {
				fRec7_perm[j11] = fRec7_tmp[vsize + j11];
			}
			/* Vectorizable loop 1 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec1[i] = fRec7[i] / std::sin(fConst2 * fRec7[i]);
			}
			/* Vectorizable loop 2 */
			/* Pre code */
			for (int j0 = 0; j0 < 4; j0 = j0 + 1) {
				fYec0_tmp[j0] = fYec0_perm[j0];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec0[i] = float(input0[i]);
			}
			/* Post code */
			for (int j1 = 0; j1 < 4; j1 = j1 + 1) {
				fYec0_perm[j1] = fYec0_tmp[vsize + j1];
			}
			/* Vectorizable loop 3 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec0[i] = std::tan(fConst1 * fRec7[i]);
			}
			/* Vectorizable loop 4 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec2[i] = fSlow15 * fZec1[i];
			}
			/* Vectorizable loop 5 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec3[i] = fSlow16 * fZec1[i];
			}
			/* Vectorizable loop 6 */
			/* Pre code */
			for (int j24 = 0; j24 < 4; j24 = j24 + 1) {
				fYec2_tmp[j24] = fYec2_perm[j24];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec2[i] = float(input1[i]);
			}
			/* Post code */
			for (int j25 = 0; j25 < 4; j25 = j25 + 1) {
				fYec2_perm[j25] = fYec2_tmp[vsize + j25];
			}
			/* Recursive loop 7 */
			/* Pre code */
			for (int j2 = 0; j2 < 4; j2 = j2 + 1) {
				fRec4_tmp[j2] = fRec4_perm[j2];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec4[i] = fSlow5 * fYec0[i - 1] - fSlow4 * (fSlow2 * fRec4[i - 1] - fSlow1 * float(input0[i]));
			}
			/* Post code */
			for (int j3 = 0; j3 < 4; j3 = j3 + 1) {
				fRec4_perm[j3] = fRec4_tmp[vsize + j3];
			}
			/* Recursive loop 8 */
			/* Pre code */
			for (int j6 = 0; j6 < 4; j6 = j6 + 1) {
				fRec6_tmp[j6] = fRec6_perm[j6];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec6[i] = 0.0f - fSlow4 * (fSlow2 * fRec6[i - 1] - (float(input0[i]) + fYec0[i - 1]));
			}
			/* Post code */
			for (int j7 = 0; j7 < 4; j7 = j7 + 1) {
				fRec6_perm[j7] = fRec6_tmp[vsize + j7];
			}
			/* Vectorizable loop 9 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec4[i] = ((iSlow13) ? fZec3[i] : fZec2[i]);
			}
			/* Vectorizable loop 10 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec5[i] = 1.0f / fZec0[i];
			}
			/* Recursive loop 11 */
			/* Pre code */
			for (int j26 = 0; j26 < 4; j26 = j26 + 1) {
				fRec14_tmp[j26] = fRec14_perm[j26];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec14[i] = fSlow5 * fYec2[i - 1] - fSlow4 * (fSlow2 * fRec14[i - 1] - fSlow1 * float(input1[i]));
			}
			/* Post code */
			for (int j27 = 0; j27 < 4; j27 = j27 + 1) {
				fRec14_perm[j27] = fRec14_tmp[vsize + j27];
			}
			/* Recursive loop 12 */
			/* Pre code */
			for (int j30 = 0; j30 < 4; j30 = j30 + 1) {
				fRec16_tmp[j30] = fRec16_perm[j30];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec16[i] = 0.0f - fSlow4 * (fSlow2 * fRec16[i - 1] - (float(input1[i]) + fYec2[i - 1]));
			}
			/* Post code */
			for (int j31 = 0; j31 < 4; j31 = j31 + 1) {
				fRec16_perm[j31] = fRec16_tmp[vsize + j31];
			}
			/* Recursive loop 13 */
			/* Pre code */
			for (int j4 = 0; j4 < 4; j4 = j4 + 1) {
				fRec3_tmp[j4] = fRec3_perm[j4];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec3[i] = fRec4[i] - fSlow10 * (fSlow9 * fRec3[i - 2] + fSlow8 * fRec3[i - 1]);
			}
			/* Post code */
			for (int j5 = 0; j5 < 4; j5 = j5 + 1) {
				fRec3_perm[j5] = fRec3_tmp[vsize + j5];
			}
			/* Recursive loop 14 */
			/* Pre code */
			for (int j8 = 0; j8 < 4; j8 = j8 + 1) {
				fRec5_tmp[j8] = fRec5_perm[j8];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec5[i] = fRec6[i] - fSlow10 * (fSlow9 * fRec5[i - 2] + fSlow8 * fRec5[i - 1]);
			}
			/* Post code */
			for (int j9 = 0; j9 < 4; j9 = j9 + 1) {
				fRec5_perm[j9] = fRec5_tmp[vsize + j9];
			}
			/* Vectorizable loop 15 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec6[i] = (fZec5[i] + fZec4[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 16 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec7[i] = 1.0f - 1.0f / parametric_eq_faustpower2_f(fZec0[i]);
			}
			/* Vectorizable loop 17 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec9[i] = (fZec5[i] - fZec4[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 18 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec10[i] = ((iSlow13) ? fZec2[i] : fZec3[i]);
			}
			/* Recursive loop 19 */
			/* Pre code */
			for (int j28 = 0; j28 < 4; j28 = j28 + 1) {
				fRec13_tmp[j28] = fRec13_perm[j28];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec13[i] = fRec14[i] - fSlow10 * (fSlow9 * fRec13[i - 2] + fSlow8 * fRec13[i - 1]);
			}
			/* Post code */
			for (int j29 = 0; j29 < 4; j29 = j29 + 1) {
				fRec13_perm[j29] = fRec13_tmp[vsize + j29];
			}
			/* Recursive loop 20 */
			/* Pre code */
			for (int j32 = 0; j32 < 4; j32 = j32 + 1) {
				fRec15_tmp[j32] = fRec15_perm[j32];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec15[i] = fRec16[i] - fSlow10 * (fSlow9 * fRec15[i - 2] + fSlow8 * fRec15[i - 1]);
			}
			/* Post code */
			for (int j33 = 0; j33 < 4; j33 = j33 + 1) {
				fRec15_perm[j33] = fRec15_tmp[vsize + j33];
			}
			/* Recursive loop 21 */
			/* Pre code */
			for (int j12 = 0; j12 < 4; j12 = j12 + 1) {
				fRec2_tmp[j12] = fRec2_perm[j12];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec8[i] = 2.0f * fRec2[i - 1] * fZec7[i];
				fRec2[i] = fSlow10 * (fSlow7 * fRec3[i] + fSlow18 * fRec3[i - 1] + fSlow7 * fRec3[i - 2] + fSlow17 * (fRec5[i - 2] + fRec5[i] + 2.0f * fRec5[i - 1])) - (fRec2[i - 2] * fZec9[i] + fZec8[i]) / fZec6[i];
			}
			/* Post code */
			for (int j13 = 0; j13 < 4; j13 = j13 + 1) {
				fRec2_perm[j13] = fRec2_tmp[vsize + j13];
			}
			/* Vectorizable loop 22 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec11[i] = (fZec5[i] - fZec10[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 23 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec12[i] = (fZec5[i] + fZec10[i]) / fZec0[i] + 1.0f;
			}
			/* Recursive loop 24 */
			/* Pre code */
			for (int j34 = 0; j34 < 4; j34 = j34 + 1) {
				fRec12_tmp[j34] = fRec12_perm[j34];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec13[i] = 2.0f * fZec7[i] * fRec12[i - 1];
				fRec12[i] = fSlow10 * (fSlow7 * fRec13[i] + fSlow18 * fRec13[i - 1] + fSlow7 * fRec13[i - 2] + fSlow17 * (fRec15[i - 2] + fRec15[i] + 2.0f * fRec15[i - 1])) - (fZec9[i] * fRec12[i - 2] + fZec13[i]) / fZec6[i];
			}
			/* Post code */
			for (int j35 = 0; j35 < 4; j35 = j35 + 1) {
				fRec12_perm[j35] = fRec12_tmp[vsize + j35];
			}
			/* Vectorizable loop 25 */
			/* Pre code */
			for (int j14 = 0; j14 < 4; j14 = j14 + 1) {
				fYec1_tmp[j14] = fYec1_perm[j14];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec1[i] = (fZec8[i] + fRec2[i] * fZec12[i] + fRec2[i - 2] * fZec11[i]) / fZec6[i];
			}
			/* Post code */
			for (int j15 = 0; j15 < 4; j15 = j15 + 1) {
				fYec1_perm[j15] = fYec1_tmp[vsize + j15];
			}
			/* Vectorizable loop 26 */
			/* Pre code */
			for (int j36 = 0; j36 < 4; j36 = j36 + 1) {
				fYec3_tmp[j36] = fYec3_perm[j36];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec3[i] = (fZec13[i] + fRec12[i] * fZec12[i] + fZec11[i] * fRec12[i - 2]) / fZec6[i];
			}
			/* Post code */
			for (int j37 = 0; j37 < 4; j37 = j37 + 1) {
				fYec3_perm[j37] = fYec3_tmp[vsize + j37];
			}
			/* Recursive loop 27 */
			/* Pre code */
			for (int j16 = 0; j16 < 4; j16 = j16 + 1) {
				fRec1_tmp[j16] = fRec1_perm[j16];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec1[i] = 0.0f - fSlow23 * (fSlow21 * fRec1[i - 1] - (fYec1[i] + fYec1[i - 1]));
			}
			/* Post code */
			for (int j17 = 0; j17 < 4; j17 = j17 + 1) {
				fRec1_perm[j17] = fRec1_tmp[vsize + j17];
			}
			/* Recursive loop 28 */
			/* Pre code */
			for (int j20 = 0; j20 < 4; j20 = j20 + 1) {
				fRec9_tmp[j20] = fRec9_perm[j20];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec9[i] = fSlow29 * fYec1[i - 1] - fSlow23 * (fSlow21 * fRec9[i - 1] - fSlow20 * fYec1[i]);
			}
			/* Post code */
			for (int j21 = 0; j21 < 4; j21 = j21 + 1) {
				fRec9_perm[j21] = fRec9_tmp[vsize + j21];
			}
			/* Recursive loop 29 */
			/* Pre code */
			for (int j38 = 0; j38 < 4; j38 = j38 + 1) {
				fRec11_tmp[j38] = fRec11_perm[j38];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec11[i] = 0.0f - fSlow23 * (fSlow21 * fRec11[i - 1] - (fYec3[i] + fYec3[i - 1]));
			}
			/* Post code */
			for (int j39 = 0; j39 < 4; j39 = j39 + 1) {
				fRec11_perm[j39] = fRec11_tmp[vsize + j39];
			}
			/* Recursive loop 30 */
			/* Pre code */
			for (int j42 = 0; j42 < 4; j42 = j42 + 1) {
				fRec18_tmp[j42] = fRec18_perm[j42];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec18[i] = fSlow29 * fYec3[i - 1] - fSlow23 * (fSlow21 * fRec18[i - 1] - fSlow20 * fYec3[i]);
			}
			/* Post code */
			for (int j43 = 0; j43 < 4; j43 = j43 + 1) {
				fRec18_perm[j43] = fRec18_tmp[vsize + j43];
			}
			/* Recursive loop 31 */
			/* Pre code */
			for (int j18 = 0; j18 < 4; j18 = j18 + 1) {
				fRec0_tmp[j18] = fRec0_perm[j18];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec0[i] = fRec1[i] - fSlow28 * (fSlow27 * fRec0[i - 2] + fSlow26 * fRec0[i - 1]);
			}
			/* Post code */
			for (int j19 = 0; j19 < 4; j19 = j19 + 1) {
				fRec0_perm[j19] = fRec0_tmp[vsize + j19];
			}
			/* Recursive loop 32 */
			/* Pre code */
			for (int j22 = 0; j22 < 4; j22 = j22 + 1) {
				fRec8_tmp[j22] = fRec8_perm[j22];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec8[i] = fRec9[i] - fSlow28 * (fSlow27 * fRec8[i - 2] + fSlow26 * fRec8[i - 1]);
			}
			/* Post code */
			for (int j23 = 0; j23 < 4; j23 = j23 + 1) {
				fRec8_perm[j23] = fRec8_tmp[vsize + j23];
			}
			/* Recursive loop 33 */
			/* Pre code */
			for (int j40 = 0; j40 < 4; j40 = j40 + 1) {
				fRec10_tmp[j40] = fRec10_perm[j40];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec10[i] = fRec11[i] - fSlow28 * (fSlow27 * fRec10[i - 2] + fSlow26 * fRec10[i - 1]);
			}
			/* Post code */
			for (int j41 = 0; j41 < 4; j41 = j41 + 1) {
				fRec10_perm[j41] = fRec10_tmp[vsize + j41];
			}
			/* Recursive loop 34 */
			/* Pre code */
			for (int j44 = 0; j44 < 4; j44 = j44 + 1) {
				fRec17_tmp[j44] = fRec17_perm[j44];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec17[i] = fRec18[i] - fSlow28 * (fSlow27 * fRec17[i - 2] + fSlow26 * fRec17[i - 1]);
			}
			/* Post code */
			for (int j45 = 0; j45 < 4; j45 = j45 + 1) {
				fRec17_perm[j45] = fRec17_tmp[vsize + j45];
			}
			/* Vectorizable loop 35 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				output0[i] = FAUSTFLOAT(fSlow28 * (fRec0[i - 2] + fRec0[i] + 2.0f * fRec0[i - 1] + fSlow31 * (fSlow25 * fRec8[i] + fSlow30 * fRec8[i - 1] + fSlow25 * fRec8[i - 2])));
			}
			/* Vectorizable loop 36 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				output1[i] = FAUSTFLOAT(fSlow28 * (fRec10[i - 2] + fRec10[i] + 2.0f * fRec10[i - 1] + fSlow31 * (fSlow25 * fRec17[i] + fSlow30 * fRec17[i - 1] + fSlow25 * fRec17[i - 2])));
			}
		}
		/* Remaining frames */
		if ((vindex < count)) {
			FAUSTFLOAT* input0 = &input0_ptr[vindex];
			FAUSTFLOAT* input1 = &input1_ptr[vindex];
			FAUSTFLOAT* output0 = &output0_ptr[vindex];
			FAUSTFLOAT* output1 = &output1_ptr[vindex];
			int vsize = count - vindex;
			/* Recursive loop 0 */
			/* Pre code */
			for (int j10 = 0; j10 < 4; j10 = j10 + 1) {
				fRec7_tmp[j10] = fRec7_perm[j10];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec7[i] = fSlow11 + 0.999f * fRec7[i - 1];
			}
			/* Post code */
			for (int j11 = 0; j11 < 4; j11 = j11 + 1) {
				fRec7_perm[j11] = fRec7_tmp[vsize + j11];
			}
			/* Vectorizable loop 1 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec1[i] = fRec7[i] / std::sin(fConst2 * fRec7[i]);
			}
			/* Vectorizable loop 2 */
			/* Pre code */
			for (int j0 = 0; j0 < 4; j0 = j0 + 1) {
				fYec0_tmp[j0] = fYec0_perm[j0];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec0[i] = float(input0[i]);
			}
			/* Post code */
			for (int j1 = 0; j1 < 4; j1 = j1 + 1) {
				fYec0_perm[j1] = fYec0_tmp[vsize + j1];
			}
			/* Vectorizable loop 3 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec0[i] = std::tan(fConst1 * fRec7[i]);
			}
			/* Vectorizable loop 4 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec2[i] = fSlow15 * fZec1[i];
			}
			/* Vectorizable loop 5 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec3[i] = fSlow16 * fZec1[i];
			}
			/* Vectorizable loop 6 */
			/* Pre code */
			for (int j24 = 0; j24 < 4; j24 = j24 + 1) {
				fYec2_tmp[j24] = fYec2_perm[j24];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec2[i] = float(input1[i]);
			}
			/* Post code */
			for (int j25 = 0; j25 < 4; j25 = j25 + 1) {
				fYec2_perm[j25] = fYec2_tmp[vsize + j25];
			}
			/* Recursive loop 7 */
			/* Pre code */
			for (int j2 = 0; j2 < 4; j2 = j2 + 1) {
				fRec4_tmp[j2] = fRec4_perm[j2];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec4[i] = fSlow5 * fYec0[i - 1] - fSlow4 * (fSlow2 * fRec4[i - 1] - fSlow1 * float(input0[i]));
			}
			/* Post code */
			for (int j3 = 0; j3 < 4; j3 = j3 + 1) {
				fRec4_perm[j3] = fRec4_tmp[vsize + j3];
			}
			/* Recursive loop 8 */
			/* Pre code */
			for (int j6 = 0; j6 < 4; j6 = j6 + 1) {
				fRec6_tmp[j6] = fRec6_perm[j6];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec6[i] = 0.0f - fSlow4 * (fSlow2 * fRec6[i - 1] - (float(input0[i]) + fYec0[i - 1]));
			}
			/* Post code */
			for (int j7 = 0; j7 < 4; j7 = j7 + 1) {
				fRec6_perm[j7] = fRec6_tmp[vsize + j7];
			}
			/* Vectorizable loop 9 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec4[i] = ((iSlow13) ? fZec3[i] : fZec2[i]);
			}
			/* Vectorizable loop 10 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec5[i] = 1.0f / fZec0[i];
			}
			/* Recursive loop 11 */
			/* Pre code */
			for (int j26 = 0; j26 < 4; j26 = j26 + 1) {
				fRec14_tmp[j26] = fRec14_perm[j26];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec14[i] = fSlow5 * fYec2[i - 1] - fSlow4 * (fSlow2 * fRec14[i - 1] - fSlow1 * float(input1[i]));
			}
			/* Post code */
			for (int j27 = 0; j27 < 4; j27 = j27 + 1) {
				fRec14_perm[j27] = fRec14_tmp[vsize + j27];
			}
			/* Recursive loop 12 */
			/* Pre code */
			for (int j30 = 0; j30 < 4; j30 = j30 + 1) {
				fRec16_tmp[j30] = fRec16_perm[j30];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec16[i] = 0.0f - fSlow4 * (fSlow2 * fRec16[i - 1] - (float(input1[i]) + fYec2[i - 1]));
			}
			/* Post code */
			for (int j31 = 0; j31 < 4; j31 = j31 + 1) {
				fRec16_perm[j31] = fRec16_tmp[vsize + j31];
			}
			/* Recursive loop 13 */
			/* Pre code */
			for (int j4 = 0; j4 < 4; j4 = j4 + 1) {
				fRec3_tmp[j4] = fRec3_perm[j4];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec3[i] = fRec4[i] - fSlow10 * (fSlow9 * fRec3[i - 2] + fSlow8 * fRec3[i - 1]);
			}
			/* Post code */
			for (int j5 = 0; j5 < 4; j5 = j5 + 1) {
				fRec3_perm[j5] = fRec3_tmp[vsize + j5];
			}
			/* Recursive loop 14 */
			/* Pre code */
			for (int j8 = 0; j8 < 4; j8 = j8 + 1) {
				fRec5_tmp[j8] = fRec5_perm[j8];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec5[i] = fRec6[i] - fSlow10 * (fSlow9 * fRec5[i - 2] + fSlow8 * fRec5[i - 1]);
			}
			/* Post code */
			for (int j9 = 0; j9 < 4; j9 = j9 + 1) {
				fRec5_perm[j9] = fRec5_tmp[vsize + j9];
			}
			/* Vectorizable loop 15 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec6[i] = (fZec5[i] + fZec4[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 16 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec7[i] = 1.0f - 1.0f / parametric_eq_faustpower2_f(fZec0[i]);
			}
			/* Vectorizable loop 17 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec9[i] = (fZec5[i] - fZec4[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 18 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec10[i] = ((iSlow13) ? fZec2[i] : fZec3[i]);
			}
			/* Recursive loop 19 */
			/* Pre code */
			for (int j28 = 0; j28 < 4; j28 = j28 + 1) {
				fRec13_tmp[j28] = fRec13_perm[j28];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec13[i] = fRec14[i] - fSlow10 * (fSlow9 * fRec13[i - 2] + fSlow8 * fRec13[i - 1]);
			}
			/* Post code */
			for (int j29 = 0; j29 < 4; j29 = j29 + 1) {
				fRec13_perm[j29] = fRec13_tmp[vsize + j29];
			}
			/* Recursive loop 20 */
			/* Pre code */
			for (int j32 = 0; j32 < 4; j32 = j32 + 1) {
				fRec15_tmp[j32] = fRec15_perm[j32];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec15[i] = fRec16[i] - fSlow10 * (fSlow9 * fRec15[i - 2] + fSlow8 * fRec15[i - 1]);
			}
			/* Post code */
			for (int j33 = 0; j33 < 4; j33 = j33 + 1) {
				fRec15_perm[j33] = fRec15_tmp[vsize + j33];
			}
			/* Recursive loop 21 */
			/* Pre code */
			for (int j12 = 0; j12 < 4; j12 = j12 + 1) {
				fRec2_tmp[j12] = fRec2_perm[j12];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec8[i] = 2.0f * fRec2[i - 1] * fZec7[i];
				fRec2[i] = fSlow10 * (fSlow7 * fRec3[i] + fSlow18 * fRec3[i - 1] + fSlow7 * fRec3[i - 2] + fSlow17 * (fRec5[i - 2] + fRec5[i] + 2.0f * fRec5[i - 1])) - (fRec2[i - 2] * fZec9[i] + fZec8[i]) / fZec6[i];
			}
			/* Post code */
			for (int j13 = 0; j13 < 4; j13 = j13 + 1) {
				fRec2_perm[j13] = fRec2_tmp[vsize + j13];
			}
			/* Vectorizable loop 22 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec11[i] = (fZec5[i] - fZec10[i]) / fZec0[i] + 1.0f;
			}
			/* Vectorizable loop 23 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec12[i] = (fZec5[i] + fZec10[i]) / fZec0[i] + 1.0f;
			}
			/* Recursive loop 24 */
			/* Pre code */
			for (int j34 = 0; j34 < 4; j34 = j34 + 1) {
				fRec12_tmp[j34] = fRec12_perm[j34];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fZec13[i] = 2.0f * fZec7[i] * fRec12[i - 1];
				fRec12[i] = fSlow10 * (fSlow7 * fRec13[i] + fSlow18 * fRec13[i - 1] + fSlow7 * fRec13[i - 2] + fSlow17 * (fRec15[i - 2] + fRec15[i] + 2.0f * fRec15[i - 1])) - (fZec9[i] * fRec12[i - 2] + fZec13[i]) / fZec6[i];
			}
			/* Post code */
			for (int j35 = 0; j35 < 4; j35 = j35 + 1) {
				fRec12_perm[j35] = fRec12_tmp[vsize + j35];
			}
			/* Vectorizable loop 25 */
			/* Pre code */
			for (int j14 = 0; j14 < 4; j14 = j14 + 1) {
				fYec1_tmp[j14] = fYec1_perm[j14];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec1[i] = (fZec8[i] + fRec2[i] * fZec12[i] + fRec2[i - 2] * fZec11[i]) / fZec6[i];
			}
			/* Post code */
			for (int j15 = 0; j15 < 4; j15 = j15 + 1) {
				fYec1_perm[j15] = fYec1_tmp[vsize + j15];
			}
			/* Vectorizable loop 26 */
			/* Pre code */
			for (int j36 = 0; j36 < 4; j36 = j36 + 1) {
				fYec3_tmp[j36] = fYec3_perm[j36];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fYec3[i] = (fZec13[i] + fRec12[i] * fZec12[i] + fZec11[i] * fRec12[i - 2]) / fZec6[i];
			}
			/* Post code */
			for (int j37 = 0; j37 < 4; j37 = j37 + 1) {
				fYec3_perm[j37] = fYec3_tmp[vsize + j37];
			}
			/* Recursive loop 27 */
			/* Pre code */
			for (int j16 = 0; j16 < 4; j16 = j16 + 1) {
				fRec1_tmp[j16] = fRec1_perm[j16];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec1[i] = 0.0f - fSlow23 * (fSlow21 * fRec1[i - 1] - (fYec1[i] + fYec1[i - 1]));
			}
			/* Post code */
			for (int j17 = 0; j17 < 4; j17 = j17 + 1) {
				fRec1_perm[j17] = fRec1_tmp[vsize + j17];
			}
			/* Recursive loop 28 */
			/* Pre code */
			for (int j20 = 0; j20 < 4; j20 = j20 + 1) {
				fRec9_tmp[j20] = fRec9_perm[j20];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec9[i] = fSlow29 * fYec1[i - 1] - fSlow23 * (fSlow21 * fRec9[i - 1] - fSlow20 * fYec1[i]);
			}
			/* Post code */
			for (int j21 = 0; j21 < 4; j21 = j21 + 1) {
				fRec9_perm[j21] = fRec9_tmp[vsize + j21];
			}
			/* Recursive loop 29 */
			/* Pre code */
			for (int j38 = 0; j38 < 4; j38 = j38 + 1) {
				fRec11_tmp[j38] = fRec11_perm[j38];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec11[i] = 0.0f - fSlow23 * (fSlow21 * fRec11[i - 1] - (fYec3[i] + fYec3[i - 1]));
			}
			/* Post code */
			for (int j39 = 0; j39 < 4; j39 = j39 + 1) {
				fRec11_perm[j39] = fRec11_tmp[vsize + j39];
			}
			/* Recursive loop 30 */
			/* Pre code */
			for (int j42 = 0; j42 < 4; j42 = j42 + 1) {
				fRec18_tmp[j42] = fRec18_perm[j42];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec18[i] = fSlow29 * fYec3[i - 1] - fSlow23 * (fSlow21 * fRec18[i - 1] - fSlow20 * fYec3[i]);
			}
			/* Post code */
			for (int j43 = 0; j43 < 4; j43 = j43 + 1) {
				fRec18_perm[j43] = fRec18_tmp[vsize + j43];
			}
			/* Recursive loop 31 */
			/* Pre code */
			for (int j18 = 0; j18 < 4; j18 = j18 + 1) {
				fRec0_tmp[j18] = fRec0_perm[j18];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec0[i] = fRec1[i] - fSlow28 * (fSlow27 * fRec0[i - 2] + fSlow26 * fRec0[i - 1]);
			}
			/* Post code */
			for (int j19 = 0; j19 < 4; j19 = j19 + 1) {
				fRec0_perm[j19] = fRec0_tmp[vsize + j19];
			}
			/* Recursive loop 32 */
			/* Pre code */
			for (int j22 = 0; j22 < 4; j22 = j22 + 1) {
				fRec8_tmp[j22] = fRec8_perm[j22];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec8[i] = fRec9[i] - fSlow28 * (fSlow27 * fRec8[i - 2] + fSlow26 * fRec8[i - 1]);
			}
			/* Post code */
			for (int j23 = 0; j23 < 4; j23 = j23 + 1) {
				fRec8_perm[j23] = fRec8_tmp[vsize + j23];
			}
			/* Recursive loop 33 */
			/* Pre code */
			for (int j40 = 0; j40 < 4; j40 = j40 + 1) {
				fRec10_tmp[j40] = fRec10_perm[j40];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec10[i] = fRec11[i] - fSlow28 * (fSlow27 * fRec10[i - 2] + fSlow26 * fRec10[i - 1]);
			}
			/* Post code */
			for (int j41 = 0; j41 < 4; j41 = j41 + 1) {
				fRec10_perm[j41] = fRec10_tmp[vsize + j41];
			}
			/* Recursive loop 34 */
			/* Pre code */
			for (int j44 = 0; j44 < 4; j44 = j44 + 1) {
				fRec17_tmp[j44] = fRec17_perm[j44];
			}
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				fRec17[i] = fRec18[i] - fSlow28 * (fSlow27 * fRec17[i - 2] + fSlow26 * fRec17[i - 1]);
			}
			/* Post code */
			for (int j45 = 0; j45 < 4; j45 = j45 + 1) {
				fRec17_perm[j45] = fRec17_tmp[vsize + j45];
			}
			/* Vectorizable loop 35 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				output0[i] = FAUSTFLOAT(fSlow28 * (fRec0[i - 2] + fRec0[i] + 2.0f * fRec0[i - 1] + fSlow31 * (fSlow25 * fRec8[i] + fSlow30 * fRec8[i - 1] + fSlow25 * fRec8[i - 2])));
			}
			/* Vectorizable loop 36 */
			/* Compute code */
			for (int i = 0; i < vsize; i = i + 1) {
				output1[i] = FAUSTFLOAT(fSlow28 * (fRec10[i - 2] + fRec10[i] + 2.0f * fRec10[i - 1] + fSlow31 * (fSlow25 * fRec17[i] + fSlow30 * fRec17[i - 1] + fSlow25 * fRec17[i - 2])));
			}
		}
	}

};

//----------------------------------------------------------------------------
//  LV2 interface
//----------------------------------------------------------------------------

#line 286 "lv2.cpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <bitset>
#include <boost/circular_buffer.hpp>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

// Set this to the proper shared library extension for your system
#ifndef DLLEXT
#define DLLEXT ".so"
#endif

#ifndef URI_PREFIX
#define URI_PREFIX "https://lv2.zrythm.org/faust-builtin"
#endif

#ifndef PLUGIN_URI
#define PLUGIN_URI URI_PREFIX "/parametric_eq"
#endif

#define MIDI_EVENT_URI "http://lv2plug.in/ns/ext/midi#MidiEvent"

/* Setting NVOICES at compile time overrides meta data in the Faust source. If
   set, this must be an integer value >= 0. A nonzero value indicates an
   instrument (VSTi) plugin with the given maximum number of voices. Use 1 for
   a monophonic synthesizer, and 0 for a simple effect plugin. If NVOICES
   isn't defined at compile time then the number of voices of an instrument
   plugin can also be set with the global "nvoices" meta data key in the Faust
   source. This setting also adds a special "polyphony" control to the plugin
   which can be used to dynamically adjust the actual number of voices in the
   range 1..NVOICES. */
//#define NVOICES 16

/* This enables a special "tuning" control for instruments which lets you
   select the MTS tuning to be used for the synth. In order to use this, you
   just drop some sysex (.syx) files with MTS octave-based tunings in 1- or
   2-byte format into the ~/.fautvst/tuning directory (these can be generated
   with the author's sclsyx program, https://bitbucket.org/agraef/sclsyx).
   The control will only be shown if any .syx files were found at startup. 0
   selects the default tuning (standard 12-tone equal temperament), i>0 the
   tuning in the ith sysex file (in alphabetic order). */
#ifndef FAUST_MTS
#define FAUST_MTS 1
#endif

/* This allows various manifest data to be generated from the corresponding
   metadata (author, name, description, license) in the Faust source. */
#ifndef FAUST_META
#define FAUST_META 1
#endif

/* This enables automatic MIDI controller mapping based on the midi:ctrl
   attributes in the Faust source. We have this enabled by default, but you
   may have to disable it if the custom controller mapping gets in the way of
   the automation facilities that the host provides. (But then again if the
   host wants to do its own controller mapping then it probably won't, or at
   least shouldn't, send us the MIDI controllers in the first place.) */
#ifndef FAUST_MIDICC
#define FAUST_MIDICC 1
#endif

/* This enables or disables the plugin's custom Qt GUI (cf. lv2ui.cpp). Note
   that this only affects the plugin manifest, the GUI code itself is in a
   separate module created with the lv2ui.cpp architecture. Also, you'll have
   to use the alternative lv2ui manifest templates to tell the LV2 host about
   the GUI. */
#ifndef FAUST_UI
#define FAUST_UI 0
#endif

// You can define these for various debugging output items.
//#define DEBUG_META 1 // recognized MIDI controller metadata
//#define DEBUG_VOICES 1 // triggering of synth voices
//#define DEBUG_VOICE_ALLOC 1 // voice allocation
//#define DEBUG_MIDI 1 // incoming MIDI messages
//#define DEBUG_NOTES 1 // note messages
//#define DEBUG_MIDICC 1 // controller messages
//#define DEBUG_RPN 1 // RPN messages (pitch bend range, master tuning)
//#define DEBUG_MTS 1 // MTS messages (octave/scale tuning)

// Note and voice data structures.

struct NoteInfo {
  uint8_t ch;
  int8_t note;
};

struct VoiceData {
  // Octave tunings (offsets in semitones) per MIDI channel.
  float tuning[16][12];
  // Allocated voices per MIDI channel and note.
  int8_t notes[16][128];
  // Free and used voices.
  int n_free, n_used;
  boost::circular_buffer<int> free_voices;
  boost::circular_buffer<int> used_voices;
  NoteInfo *note_info;
  // Voices queued for note-offs (zero-length notes).
  set<int> queued;
  // Last gate value during run() for each voice. We need to keep track of
  // these so that we can force the Faust synth to retrigger a note when
  // needed.
  float *lastgate;
  // Current pitch bend and pitch bend range on each MIDI channel, in semitones.
  float bend[16], range[16];
  // Current coarse, fine and total master tuning on each MIDI channel (tuning
  // offset relative to A4 = 440 Hz, in semitones).
  float coarse[16], fine[16], tune[16];
  VoiceData(int n) : free_voices(n), used_voices(n) { }
};

#if FAUST_MTS

// Helper classes to read and store MTS tunings.

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <string>
#include <vector>

struct MTSTuning {
  char *name; // name of the tuning
  int len; // length of sysex data in bytes
  unsigned char *data; // sysex data
  MTSTuning() : name(0), len(0), data(0) {}
  MTSTuning& operator=(const MTSTuning &t)
  {
    if (this == &t) return *this;
    if (name) free(name); if (data) free(data);
    name = 0; data = 0; len = t.len;
    if (t.name) {
      name = strdup(t.name); assert(name);
    }
    if (t.data) {
      data = (unsigned char*)malloc(len); assert(data);
      memcpy(data, t.data, len);
    }
    return *this;
  }
  MTSTuning(const MTSTuning& t) : name(0), len(0), data(0)
  { *this = t; }
  MTSTuning(const char *filename);
  ~MTSTuning()
  { if (name) free(name); if (data) free(data); }
};

MTSTuning::MTSTuning(const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  name = 0; len = 0; data = 0;
  if (!fp) return;
  struct stat st;
  if (fstat(fileno(fp), &st)) return;
  len = st.st_size;
  data = (unsigned char*)calloc(len, 1);
  if (!data) {
    len = 0; fclose(fp);
    return;
  }
  assert(len > 0);
  if (fread(data, 1, len, fp) < len) {
    free(data); len = 0; data = 0; fclose(fp);
    return;
  }
  fclose(fp);
  // Do some basic sanity checks.
  if (data[0] != 0xf0 || data[len-1] != 0xf7 || // not a sysex message
      (data[1] != 0x7e && data[1] != 0x7f) || data[3] != 8 || // not MTS
      !((len == 21 && data[4] == 8) ||
	(len == 33 && data[4] == 9))) { // no 1- or 2-byte tuning
    free(data); len = 0; data = 0;
    return;
  }
  // Name of the tuning is the basename of the file, without the trailing .syx
  // suffix.
  string nm = filename;
  size_t p = nm.rfind(".syx");
  if (p != string::npos) nm.erase(p);
  p = nm.rfind('/');
  if (p != string::npos) nm.erase(0, p+1);
  name = strdup(nm.c_str());
  assert(name);
}

struct MTSTunings {
  vector<MTSTuning> tuning;
  MTSTunings() {}
  MTSTunings(const char *path);
};

static bool compareByName(const MTSTuning &a, const MTSTuning &b)
{
  return strcmp(a.name, b.name) < 0;
}

MTSTunings::MTSTunings(const char *path)
{
  DIR *dp = opendir(path);
  if (!dp) return;
  struct dirent *d;
  while ((d = readdir(dp))) {
    string nm = d->d_name;
    if (nm.length() > 4 && nm.substr(nm.length()-4) == ".syx") {
      string pathname = path;
      pathname += "/";
      pathname += nm;
      MTSTuning t(pathname.c_str());
      if (t.data) tuning.push_back(t);
    }
  }
  closedir(dp);
  // sort found tunings by name
  sort(tuning.begin(), tuning.end(), compareByName);
}

#endif

#if FAUST_MIDICC
static float ctrlval(const ui_elem_t &el, uint8_t v)
{
  // Translate the given MIDI controller value to the range and stepsize
  // indicated by the Faust control.
  switch (el.type) {
  case UI_BUTTON: case UI_CHECK_BUTTON:
    return (float)(v>=64);
  default:
    /* Continuous controllers. The problem here is that the range 0..127 is
       not symmetric. We'd like to map 64 to the center of the range
       (max-min)/2 and at the same time retain the full control range
       min..max. So let's just pretend that there are 128 controller values
       and map value 127 to the max value anyway. */
    if (v==127)
      return el.max;
    else
      // XXXFIXME: We might want to add proper quantization according to
      // el.step here.
      return el.min+(el.max-el.min)*v/128;
  }
}
#endif

/***************************************************************************/

/* Polyphonic Faust plugin data structure. XXXTODO: At present this is just a
   big struct which exposes all requisite data. Some more work is needed to
   make the interface a bit more abstract and properly encapsulate the
   internal data structures, so that implementation details can be changed
   more easily. */

struct LV2Plugin {
  const int maxvoices;	// maximum number of voices (zero if not an instrument)
  const int ndsps;	// number of dsp instances (1 if maxvoices==0)
  bool active;		// activation status
  int rate;		// sampling rate
  int nvoices;		// current number of voices (<= maxvoices)
  int tuning_no;	// current tuning number (<= n_tunings)
  parametric_eq **dsp;		// the dsps
  LV2UI **ui;		// their Faust interface descriptions
  int n_in, n_out;	// number of input and output control ports
  int *ctrls;		// Faust ui elements (indices into ui->elems)
  float **ports;	// corresponding LV2 data
  float *portvals;	// cached port data from the last run
  float *midivals[16];	// per-midi channel data
  int *inctrls, *outctrls;	// indices for active and passive controls
  float **inputs, **outputs;	// audio buffers
  int freq, gain, gate;	// indices of voice controls
  unsigned n_samples;	// current block size
  float **outbuf;	// audio buffers for mixing down the voices
  float **inbuf;	// dummy input buffer
  LV2_Atom_Sequence* event_port; // midi input
  float *poly, *tuning;	// polyphony and tuning ports
  std::map<uint8_t,int> ctrlmap; // MIDI controller map
  // Needed host features.
  LV2_URID_Map* map;	// the urid extension
  LV2_URID midi_event;	// midi event uri
  // Current RPN MSB and LSB numbers, as set with controllers 101 and 100.
  uint8_t rpn_msb[16], rpn_lsb[16];
  // Current data entry MSB and LSB numbers, as set with controllers 6 and 38.
  uint8_t data_msb[16], data_lsb[16];
  // Synth voice data (instruments only).
  VoiceData *vd;

  // Static methods. These all use static data so they can be invoked before
  // instantiating a plugin.

  // Global meta data (dsp name, author, etc.).
  static Meta *meta;
  static void init_meta()
  {
    if (!meta && (meta = new Meta)) {
      // We allocate the temporary dsp object on the heap here, to prevent
      // large dsp objects from running out of stack in environments where
      // stack space is precious (e.g., Reaper). Note that if any of these
      // allocations fail then no meta data will be available, but at least we
      // won't make the host crash and burn.
      parametric_eq* tmp_dsp = new parametric_eq();
      if (tmp_dsp) {
	tmp_dsp->metadata(meta);
	delete tmp_dsp;
      }
    }
  }
  static const char *meta_get(const char *key, const char *deflt)
  {
    init_meta();
    return meta?meta->get(key, deflt):deflt;
  }

  static const char *pluginName()
  {
    return meta_get("name", "parametric_eq");
  }

  static const char *pluginAuthor()
  {
    return meta_get("author", "");
  }

  static const char *pluginDescription()
  {
    return meta_get("description", "");
  }

  static const char *pluginLicense()
  {
    return meta_get("license", "");
  }

  static const char *pluginVersion()
  {
    return meta_get("version", "");
  }

  // Load a collection of sysex files with MTS tunings in ~/.faust/tuning.
  static int n_tunings;
#if FAUST_MTS
  static MTSTunings *mts;

  static MTSTunings *load_sysex_data()
  {
    if (!mts) {
      string mts_path;
      // Look for FAUST_HOME. If that isn't set, try $HOME/.faust. If HOME
      // isn't set either, just assume a .faust subdir of the cwd.
      const char *home = getenv("FAUST_HOME");
      if (home)
	mts_path = home;
      else {
	home = getenv("HOME");
	if (home) {
	  mts_path = home;
	  mts_path += "/.faust";
	} else
	  mts_path = ".faust";
      }
      // MTS tunings are looked for in this subdir.
      mts_path += "/tuning";
      mts = new MTSTunings(mts_path.c_str());
#ifdef __APPLE__
      if (!mts || mts->tuning.size() == 0) {
	// Also check ~/Library/Faust/Tuning on the Mac.
	home = getenv("HOME");
	if (home) {
	  if (mts) delete mts;
	  mts_path = home;
	  mts_path += "/Library/Faust/Tuning";
	  mts = new MTSTunings(mts_path.c_str());
	}
      }
#endif
      n_tunings = mts->tuning.size();
    }
    return mts;
  }
#endif

  // The number of voices of an instrument plugin. We get this information
  // from the global meta data (nvoices key) of the dsp module if present, and
  // you can also override this setting at compile time by defining the
  // NVOICES macro. If neither is defined or the value is zero then the plugin
  // becomes a simple audio effect instead.
  static int numVoices()
  {
#ifdef NVOICES
    return NVOICES;
#else
    const char *numVoices = meta_get("nvoices", "0");
    int nvoices = atoi(numVoices);
    if (nvoices < 0 ) nvoices = 0;
    return nvoices;
#endif
  }

  // Instance methods.

  LV2Plugin(const int num_voices, const int sr)
    : maxvoices(num_voices), ndsps(num_voices<=0?1:num_voices),
      vd(num_voices>0?new VoiceData(num_voices):0)
  {
    // Initialize static data.
    init_meta();
#if FAUST_MTS
    // Synth: load tuning sysex data if present.
    if (num_voices>0) load_sysex_data();
#endif
    // Allocate data structures and set some reasonable defaults.
    dsp = (parametric_eq**)calloc(ndsps, sizeof(parametric_eq*));
    ui = (LV2UI**)calloc(ndsps, sizeof(LV2UI*));
    assert(dsp && ui);
    if (vd) {
      vd->note_info = (NoteInfo*)calloc(ndsps, sizeof(NoteInfo));
      vd->lastgate = (float*)calloc(ndsps, sizeof(float));
      assert(vd->note_info && vd->lastgate);
    }
    active = false;
    rate = sr;
    nvoices = maxvoices;
    tuning_no = 0;
    n_in = n_out = 0;
    map = NULL;
    midi_event = -1;
    event_port = NULL;
    poly = tuning = NULL;
    freq = gain = gate = -1;
    if (vd) {
      vd->n_free = maxvoices;
      for (int i = 0; i < maxvoices; i++) {
	vd->free_voices.push_back(i);
	vd->lastgate[i] = 0.0f;
      }
      for (int i = 0; i < 16; i++) {
	vd->bend[i] = 0.0f;
	vd->range[i] = 2.0f;
	vd->coarse[i] = vd->fine[i] = vd->tune[i] = 0.0f;
	for (int j = 0; j < 12; j++)
	  vd->tuning[i][j] = 0.0f;
      }
      vd->n_used = 0;
      memset(vd->notes, 0xff, sizeof(vd->notes));
    }
    n_samples = 0;
    ctrls = inctrls = outctrls = NULL;
    ports = inputs = outputs = inbuf = outbuf = NULL;
    portvals = NULL;
    memset(midivals, 0, sizeof(midivals));
    // Initialize the Faust DSPs.
    for (int i = 0; i < ndsps; i++) {
      dsp[i] = new parametric_eq();
      ui[i] = new LV2UI(num_voices);
      dsp[i]->init(rate);
      dsp[i]->buildUserInterface(ui[i]);
    }
    // The ports are numbered as follows: 0..k-1 are the control ports, then
    // come the n audio input ports, then the m audio output ports, and
    // finally the midi input port and the polyphony and tuning controls.
    int k = ui[0]->nports, p = 0, q = 0;
    int n = dsp[0]->getNumInputs(), m = dsp[0]->getNumOutputs();
    // Allocate tables for the built-in control elements and their ports.
    ctrls = (int*)calloc(k, sizeof(int));
    inctrls = (int*)calloc(k, sizeof(int));
    outctrls = (int*)calloc(k, sizeof(int));
    ports = (float**)calloc(k, sizeof(float*));
    portvals = (float*)calloc(k, sizeof(float));
    assert(k == 0 || (ctrls && inctrls && outctrls && ports && portvals));
    for (int ch = 0; ch < 16; ch++) {
      midivals[ch] = (float*)calloc(k, sizeof(float));
      assert(k == 0 || midivals[ch]);
    }
    // Scan the Faust UI for active and passive controls which become the
    // input and output control ports of the plugin, respectively.
    for (int i = 0, j = 0; i < ui[0]->nelems; i++) {
      switch (ui[0]->elems[i].type) {
      case UI_T_GROUP: case UI_H_GROUP: case UI_V_GROUP: case UI_END_GROUP:
	// control groups (ignored right now)
	break;
      case UI_H_BARGRAPH: case UI_V_BARGRAPH:
	// passive controls (output ports)
	ctrls[j++] = i;
	outctrls[q++] = i;
	break;
      default:
	// active controls (input ports)
	if (maxvoices == 0)
	  goto noinstr;
	else if (freq == -1 &&
		 !strcmp(ui[0]->elems[i].label, "freq"))
	  freq = i;
	else if (gain == -1 &&
		 !strcmp(ui[0]->elems[i].label, "gain"))
	  gain = i;
	else if (gate == -1 &&
		 !strcmp(ui[0]->elems[i].label, "gate"))
	  gate = i;
	else {
	noinstr:
#if FAUST_MIDICC
	  std::map< int, list<strpair> >::iterator it =
	    ui[0]->metadata.find(i);
	  if (it != ui[0]->metadata.end()) {
	    // Scan for controller mappings.
	    for (std::list<strpair>::iterator jt = it->second.begin();
		 jt != it->second.end(); jt++) {
	      const char *key = jt->first, *val = jt->second;
#if DEBUG_META
	      fprintf(stderr, "ctrl '%s' meta: '%s' -> '%s'\n",
		      ui[0]->elems[i].label, key, val);
#endif
	      if (strcmp(key, "midi") == 0) {
		unsigned num;
		if (sscanf(val, "ctrl %u", &num) < 1) continue;
#if 0 // enable this to get feedback about controller assignments
		fprintf(stderr, "%s: cc %d -> %s\n", PLUGIN_URI, num,
			ui[0]->elems[i].label);
#endif
		ctrlmap.insert(std::pair<uint8_t,int>(num, p));
	      }
	    }
	  }
#endif
	  ctrls[j++] = i;
	  inctrls[p++] = i;
	  int p = ui[0]->elems[i].port;
	  float val = ui[0]->elems[i].init;
	  assert(p>=0);
	  portvals[p] = val;
	  for (int ch = 0; ch < 16; ch++)
	    midivals[ch][p] = val;
	}
	break;
      }
    }
    // Realloc the inctrls and outctrls vectors to their appropriate sizes.
    inctrls = (int*)realloc(inctrls, p*sizeof(int));
    assert(p == 0 || inctrls);
    outctrls = (int*)realloc(outctrls, q*sizeof(int));
    assert(q == 0 || outctrls);
    n_in = p; n_out = q;
    // Allocate vectors for the audio input and output ports. Like
    // ports, these will be initialized in the connect_port callback.
    inputs = (float**)calloc(n, sizeof(float*));
    assert(n == 0 || inputs);
    outputs = (float**)calloc(m, sizeof(float*));
    assert(m == 0 || outputs);
    if (maxvoices > 0) {
      // Initialize the mixdown buffer.
      outbuf = (float**)calloc(m, sizeof(float*));
      assert(m == 0 || outbuf);
      // We start out with a blocksize of 512 samples here. Hopefully this is
      // enough for most realtime hosts so that we can avoid reallocations
      // later when we know what the actual blocksize is.
      n_samples = 512;
      for (int i = 0; i < m; i++) {
	outbuf[i] = (float*)malloc(n_samples*sizeof(float));
	assert(outbuf[i]);
      }
      // Initialize a 1-sample dummy input buffer used for retriggering notes.
      inbuf = (float**)calloc(n, sizeof(float*));
      assert(n == 0 || inbuf);
      for (int i = 0; i < m; i++) {
	inbuf[i] = (float*)malloc(sizeof(float));
	assert(inbuf[i]);
	*inbuf[i] = 0.0f;
      }
    }
  }

  ~LV2Plugin()
  {
    const int n = dsp[0]->getNumInputs();
    const int m = dsp[0]->getNumOutputs();
    for (int i = 0; i < ndsps; i++) {
      delete dsp[i];
      delete ui[i];
    }
    free(ctrls); free(inctrls); free(outctrls);
    free(ports); free(portvals);
    free(inputs); free(outputs);
    for (int ch = 0; ch < 16; ch++)
      free(midivals[ch]);
    if (inbuf) {
      for (int i = 0; i < n; i++)
	free(inbuf[i]);
      free(inbuf);
    }
    if (outbuf) {
      for (int i = 0; i < m; i++)
	free(outbuf[i]);
      free(outbuf);
    }
    free(dsp); free(ui);
    if (vd) {
      free(vd->note_info);
      free(vd->lastgate);
      delete vd;
    }
  }
  // Voice allocation.

#if DEBUG_VOICE_ALLOC
  void print_voices(const char *msg)
  {
    fprintf(stderr, "%s: notes =", msg);
    for (uint8_t ch = 0; ch < 16; ch++)
      for (int note = 0; note < 128; note++)
	if (vd->notes[ch][note] >= 0)
	  fprintf(stderr, " [%d] %d(#%d)", ch, note, vd->notes[ch][note]);
    fprintf(stderr, "\nqueued (%d):", vd->queued.size());
    for (int i = 0; i < nvoices; i++)
      if (vd->queued.find(i) != vd->queued.end()) fprintf(stderr, " #%d", i);
    fprintf(stderr, "\nused (%d):", vd->n_used);
    for (boost::circular_buffer<int>::iterator it = vd->used_voices.begin();
	 it != vd->used_voices.end(); it++)
      fprintf(stderr, " #%d->%d", *it, vd->note_info[*it].note);
    fprintf(stderr, "\nfree (%d):", vd->n_free);
    for (boost::circular_buffer<int>::iterator it = vd->free_voices.begin();
	 it != vd->free_voices.end(); it++)
      fprintf(stderr, " #%d", *it);
    fprintf(stderr, "\n");
  }
#endif

  int alloc_voice(uint8_t ch, int8_t note, int8_t vel)
  {
    int i = vd->notes[ch][note];
    if (i >= 0) {
      // note already playing on same channel, retrigger it
      voice_off(i);
      voice_on(i, note, vel, ch);
      // move this voice to the end of the used list
      for (boost::circular_buffer<int>::iterator it =
	     vd->used_voices.begin();
	   it != vd->used_voices.end(); it++) {
	if (*it == i) {
	  vd->used_voices.erase(it);
	  vd->used_voices.push_back(i);
	  break;
	}
      }
#if DEBUG_VOICE_ALLOC
      print_voices("retrigger");
#endif
      return i;
    } else if (vd->n_free > 0) {
      // take voice from free list
      int i = vd->free_voices.front();
      vd->free_voices.pop_front();
      vd->n_free--;
      vd->used_voices.push_back(i);
      vd->note_info[i].ch = ch;
      vd->note_info[i].note = note;
      vd->n_used++;
      voice_on(i, note, vel, ch);
      vd->notes[ch][note] = i;
#if DEBUG_VOICE_ALLOC
      print_voices("alloc");
#endif
      return i;
    } else {
      // steal a voice
      assert(vd->n_used > 0);
      // FIXME: Maybe we should look for the oldest note on the *current*
      // channel here, but this is faster.
      int i = vd->used_voices.front();
      int oldch = vd->note_info[i].ch;
      int oldnote = vd->note_info[i].note;
      voice_off(i);
      vd->notes[oldch][oldnote] = -1;
      vd->queued.erase(i);
      vd->used_voices.pop_front();
      vd->used_voices.push_back(i);
      vd->note_info[i].ch = ch;
      vd->note_info[i].note = note;
      voice_on(i, note, vel, ch);
      vd->notes[ch][note] = i;
#if DEBUG_VOICE_ALLOC
      print_voices("steal");
#endif
      return i;
    }
  }

  int dealloc_voice(uint8_t ch, int8_t note, int8_t vel)
  {
    int i = vd->notes[ch][note];
    if (i >= 0) {
      if (vd->lastgate[i] == 0.0f && gate >= 0) {
	// zero-length note, queued for later
	vd->queued.insert(i);
	vd->notes[ch][note] = -1;
#if DEBUG_VOICE_ALLOC
	print_voices("dealloc (queued)");
#endif
	return i;
      }
      assert(vd->n_free < nvoices);
      vd->free_voices.push_back(i);
      vd->n_free++;
      voice_off(i);
      vd->notes[ch][note] = -1;
      // erase this voice from the used list
      for (boost::circular_buffer<int>::iterator it =
	     vd->used_voices.begin();
	   it != vd->used_voices.end(); it++) {
	if (*it == i) {
	  vd->used_voices.erase(it);
	  vd->n_used--;
	  break;
	}
      }
#if DEBUG_VOICE_ALLOC
      print_voices("dealloc");
#endif
      return i;
    }
    return -1;
  }


  float midicps(int8_t note, uint8_t chan)
  {
    float pitch = note + vd->tune[chan] +
      vd->tuning[chan][note%12] + vd->bend[chan];
    return 440.0*pow(2, (pitch-69.0)/12.0);
  }

  void voice_on(int i, int8_t note, int8_t vel, uint8_t ch)
  {
    if (vd->lastgate[i] == 1.0f && gate >= 0) {
      // Make sure that the synth sees the 0.0f gate so that the voice is
      // properly retriggered.
      *ui[i]->elems[gate].zone = 0.0f;
      dsp[i]->compute(1, inbuf, outbuf);
    }
#if DEBUG_VOICES
    fprintf(stderr, "voice on: %d %d (%g Hz) %d (%g)\n", i,
	    note, midicps(note, ch), vel, vel/127.0);
#endif
    if (freq >= 0)
      *ui[i]->elems[freq].zone = midicps(note, ch);
    if (gate >= 0)
      *ui[i]->elems[gate].zone = 1.0f;
    if (gain >= 0)
      *ui[i]->elems[gain].zone = vel/127.0;
    // reinitialize the per-channel control data for this voice
    for (int idx = 0; idx < n_in; idx++) {
      int j = inctrls[idx], k = ui[0]->elems[j].port;
      *ui[i]->elems[j].zone = midivals[ch][k];
    }
  }

  void voice_off(int i)
  {
#if DEBUG_VOICES
    fprintf(stderr, "voice off: %d\n", i);
#endif
    if (gate >= 0)
      *ui[i]->elems[gate].zone = 0.0f;
  }

  void update_voices(uint8_t chan)
  {
    // update running voices on the given channel after tuning or pitch bend
    // changes
    for (boost::circular_buffer<int>::iterator it =
	   vd->used_voices.begin();
	 it != vd->used_voices.end(); it++) {
      int i = *it;
      if (vd->note_info[i].ch == chan && freq >= 0) {
	int note = vd->note_info[i].note;
	*ui[i]->elems[freq].zone = midicps(note, chan);
      }
    }
  }

  void all_notes_off()
  {
    for (int i = 0; i < nvoices; i++)
      voice_off(i);
    for (int i = 0; i < 16; i++)
      vd->bend[i] = 0.0f;
    memset(vd->notes, 0xff, sizeof(vd->notes));
    vd->free_voices.clear();
    vd->n_free = nvoices;
    for (int i = 0; i < nvoices; i++)
      vd->free_voices.push_back(i);
    vd->queued.clear();
    vd->used_voices.clear();
    vd->n_used = 0;
  }

  void all_notes_off(uint8_t chan)
  {
    for (boost::circular_buffer<int>::iterator it =
	   vd->used_voices.begin();
	 it != vd->used_voices.end(); ) {
      int i = *it;
      if (vd->note_info[i].ch == chan) {
	assert(vd->n_free < nvoices);
	vd->free_voices.push_back(i);
	vd->n_free++;
	voice_off(i);
	vd->notes[vd->note_info[i].ch][vd->note_info[i].note] = -1;
	vd->queued.erase(i);
	// erase this voice from the used list
	it = vd->used_voices.erase(it);
	vd->n_used--;
#if DEBUG_VOICE_ALLOC
	print_voices("dealloc (all-notes-off)");
#endif
      } else
	it++;
    }
    vd->bend[chan] = 0.0f;
  }

  void queued_notes_off()
  {
    if (vd->queued.empty()) return;
    for (int i = 0; i < nvoices; i++)
      if (vd->queued.find(i) != vd->queued.end()) {
	assert(vd->n_free < nvoices);
	vd->free_voices.push_back(i);
	vd->n_free++;
	voice_off(i);
	vd->notes[vd->note_info[i].ch][vd->note_info[i].note] = -1;
	vd->queued.erase(i);
	// erase this voice from the used list
	for (boost::circular_buffer<int>::iterator it =
	       vd->used_voices.begin();
	     it != vd->used_voices.end(); it++) {
	  if (*it == i) {
	    vd->used_voices.erase(it);
	    vd->n_used--;
	    break;
	  }
	}
#if DEBUG_VOICE_ALLOC
	print_voices("dealloc (unqueued)");
#endif
      }
  }

  // Plugin activation status. suspend() deactivates a plugin (disables audio
  // processing), resume() reactivates it. Also, set_rate() changes the sample
  // rate. Note that the audio and MIDI process functions (see below) can
  // still be called in deactivated state, but this is optional. The plugin
  // tries to do some reasonable processing in either case, no matter whether
  // the host plugin architecture actually executes callbacks in suspended
  // state or not.

  void suspend()
  {
    active = false;
    if (maxvoices > 0) all_notes_off();
  }

  void resume()
  {
    for (int i = 0; i < ndsps; i++)
      dsp[i]->init(rate);
    for (int i = 0, j = 0; i < ui[0]->nelems; i++) {
      int p = ui[0]->elems[i].port;
      if (p >= 0) {
	float val = ui[0]->elems[i].init;
	portvals[p] = val;
      }
    }
    active = true;
  }

  void set_rate(int sr)
  {
    rate = sr;
    for (int i = 0; i < ndsps; i++)
      dsp[i]->init(rate);
  }

  // Audio and MIDI process functions. The plugin should run these in the
  // appropriate real-time callbacks.

  void process_audio(int blocksz, float **inputs, float **outputs)
  {
    int n = dsp[0]->getNumInputs(), m = dsp[0]->getNumOutputs();
    AVOIDDENORMALS;
    if (maxvoices > 0) queued_notes_off();
    if (!active) {
      // Depending on the plugin architecture and host, this code might never
      // be invoked, since the plugin is deactivitated at this point. But
      // let's do something reasonable here anyway.
      if (n == m) {
	// copy inputs to outputs
	for (int i = 0; i < m; i++)
	  for (unsigned j = 0; j < blocksz; j++)
	    outputs[i][j] = inputs[i][j];
      } else {
	// silence
	for (int i = 0; i < m; i++)
	  for (unsigned j = 0; j < blocksz; j++)
	    outputs[i][j] = 0.0f;
      }
      return;
    }
    // Handle changes in the polyphony and tuning controls.
    bool is_instr = maxvoices > 0;
    if (is_instr) {
      if (!poly)
	; // this shouldn't happen but...
      else if (nvoices != (int)*poly &&
	       (int)*poly > 0 && (int)*poly <= maxvoices) {
	for (int i = 0; i < nvoices; i++)
	  voice_off(i);
        nvoices = (int)*poly;
	// Reset the voice allocation.
	memset(vd->notes, 0xff, sizeof(vd->notes));
	vd->free_voices.clear();
	vd->n_free = nvoices;
	for (int i = 0; i < nvoices; i++)
	  vd->free_voices.push_back(i);
	vd->used_voices.clear();
	vd->n_used = 0;
      } else
	*poly = nvoices;
#if FAUST_MTS
      if (tuning && tuning_no != (int)*tuning) change_tuning((int)*tuning);
#endif
    }
    // Only update the controls (of all voices simultaneously) if a port value
    // actually changed. This is necessary to allow MIDI controllers to modify
    // the values for individual MIDI channels (see processEvents below). Also
    // note that this will be done *after* processing the MIDI controller data
    // for the current audio block, so manual inputs can still override these.
    for (int i = 0; i < n_in; i++) {
      int j = inctrls[i], k = ui[0]->elems[j].port;
      float &oldval = portvals[k], newval = *ports[k];
      if (newval != oldval) {
	if (is_instr) {
	  // instrument: update running voices
	  for (boost::circular_buffer<int>::iterator it =
		 vd->used_voices.begin();
	       it != vd->used_voices.end(); it++) {
	    int i = *it;
	    *ui[i]->elems[j].zone = newval;
	  }
	} else {
	  // simple effect: here we only have a single dsp instance
	  *ui[0]->elems[j].zone = newval;
	}
	// also update the MIDI controller data for all channels (manual
	// control input is always omni)
	for (int ch = 0; ch < 16; ch++)
	  midivals[ch][k] = newval;
	// record the new value
	oldval = newval;
      }
    }
    // Initialize the output buffers.
    if (n_samples < blocksz) {
      // We need to enlarge the buffers. We're not officially allowed to do
      // this here (presumably in the realtime thread), but since we usually
      // don't know the hosts's block size beforehand, there's really nothing
      // else that we can do. Let's just hope that doing this once suffices,
      // then hopefully noone will notice.
      if (outbuf) {
	for (int i = 0; i < m; i++) {
	  outbuf[i] = (float*)realloc(outbuf[i],
				      blocksz*sizeof(float));
	  assert(outbuf[i]);
	}
      }
      n_samples = blocksz;
    }
    if (outbuf) {
      // Polyphonic instrument: Mix the voices down to one signal.
      for (int i = 0; i < m; i++)
	for (unsigned j = 0; j < blocksz; j++)
	  outputs[i][j] = 0.0f;
      for (int l = 0; l < nvoices; l++) {
	// Let Faust do all the hard work.
	dsp[l]->compute(blocksz, inputs, outbuf);
	for (int i = 0; i < m; i++)
	  for (unsigned j = 0; j < blocksz; j++)
	    outputs[i][j] += outbuf[i][j];
      }
    } else {
      // Simple effect: We can write directly to the output buffer.
      dsp[0]->compute(blocksz, inputs, outputs);
    }
    // Finally grab the passive controls and write them back to the
    // corresponding control ports. NOTE: Depending on the plugin
    // architecture, this might require a host call to get the control GUI
    // updated in real-time (if the host supports this at all).
    // FIXME: It's not clear how to aggregate the data of the different
    // voices. We compute the maximum of each control for now.
    for (int i = 0; i < n_out; i++) {
      int j = outctrls[i], k = ui[0]->elems[j].port;
      float *z = ui[0]->elems[j].zone;
      *ports[k] = *z;
      for (int l = 1; l < nvoices; l++) {
	float *z = ui[l]->elems[j].zone;
	if (*ports[k] < *z)
	  *ports[k] = *z;
      }
    }
    // Keep track of the last gates set for each voice, so that voices can be
    // forcibly retriggered if needed.
    if (gate >= 0)
      for (int i = 0; i < nvoices; i++)
	vd->lastgate[i] =
	  *ui[i]->elems[gate].zone;
  }

  // This processes just a single MIDI message, so to process an entire series
  // of MIDI events you'll have to loop over the event data in the plugin's
  // MIDI callback. XXXTODO: Sample-accurate processing of MIDI events.

  void process_midi(unsigned char *data, int sz)
  {
#if DEBUG_MIDI
    fprintf(stderr, "midi ev (%d bytes):", sz);
    for (int i = 0; i < sz; i++)
      fprintf(stderr, " 0x%0x", data[i]);
    fprintf(stderr, "\n");
#endif
    uint8_t status = data[0] & 0xf0, chan = data[0] & 0x0f;
    bool is_instr = maxvoices > 0;
    switch (status) {
    case 0x90: {
      if (!is_instr) break;
      // note on
#if DEBUG_NOTES
      fprintf(stderr, "note-on  chan %d, note %d, vel %d\n", chan+1,
	      data[1], data[2]);
#endif
      if (data[2] == 0) goto note_off;
      alloc_voice(chan, data[1], data[2]);
      break;
    }
    case 0x80: {
      if (!is_instr) break;
      // note off
#if DEBUG_NOTES
      fprintf(stderr, "note-off chan %d, note %d, vel %d\n", chan+1,
	      data[1], data[2]);
#endif
      note_off:
      dealloc_voice(chan, data[1], data[2]);
      break;
    }
    case 0xe0: {
      if (!is_instr) break;
      // pitch bend
      // data[1] is LSB, data[2] MSB, range is 0..0x3fff (which maps to
      // -2..+2 semitones by default), center point is 0x2000 = 8192
      int val = data[1] | (data[2]<<7);
      vd->bend[chan] =
	(val-0x2000)/8192.0f*vd->range[chan];
#if DEBUG_MIDICC
      fprintf(stderr, "pitch-bend (chan %d): %g cent\n", chan+1,
	      vd->bend[chan]*100.0);
#endif
      update_voices(chan);
      break;
    }
    case 0xb0: {
      // controller change
      switch (data[1]) {
      case 120: case 123:
	if (!is_instr) break;
	// all-sound-off and all-notes-off controllers (these are treated
	// the same in the current implementation)
	all_notes_off(chan);
#if DEBUG_MIDICC
	fprintf(stderr, "all-notes-off (chan %d)\n", chan+1);
#endif
	break;
      case 121:
	// all-controllers-off (in the current implementation, this just
	// resets the RPN-related controllers)
	data_msb[chan] = data_lsb[chan] = 0;
	rpn_msb[chan] = rpn_lsb[chan] = 0x7f;
#if DEBUG_MIDICC
	fprintf(stderr, "all-controllers-off (chan %d)\n", chan+1);
#endif
	break;
      case 101: case 100:
	// RPN MSB/LSB
	if (data[1] == 101)
	  rpn_msb[chan] = data[2];
	else
	  rpn_lsb[chan] = data[2];
	break;
      case 6: case 38:
	// data entry coarse/fine
	if (data[1] == 6)
	  data_msb[chan] = data[2];
	else
	  data_lsb[chan] = data[2];
	goto rpn;
      case 96: case 97:
	// data increment/decrement
	/* NOTE: The specification of these controllers is a complete
	   mess. Originally, the MIDI specification didn't have anything
	   to say about their exact behaviour at all. Nowadays, the
	   behaviour depends on which RPN or NRPN is being modified, which
	   is also rather confusing. Fortunately, as we only handle RPNs
	   0..2 here anyway, it's sufficient to assume the MSB for RPN #2
	   (channel coarse tuning) and the LSB otherwise. */
	if (rpn_msb[chan] == 0 && rpn_lsb[chan] == 2) {
	  // modify the MSB
	  if (data[1] == 96 && data_msb[chan] < 0x7f)
	    data_msb[chan]++;
	  else if (data[1] == 97 && data_msb[chan] > 0)
	    data_msb[chan]--;
	} else {
	  // modify the LSB
	  if (data[1] == 96 && data_lsb[chan] < 0x7f)
	    data_lsb[chan]++;
	  else if (data[1] == 97 && data_lsb[chan] > 0)
	    data_lsb[chan]--;
	}
      rpn:
	if (!is_instr) break;
	if (rpn_msb[chan] == 0) {
	  switch (rpn_lsb[chan]) {
	  case 0:
	    // pitch bend range, coarse value is in semitones, fine value
	    // in cents
	    vd->range[chan] = data_msb[chan]+
	      data_lsb[chan]/100.0;
#if DEBUG_RPN
	    fprintf(stderr, "pitch-bend-range (chan %d): %g cent\n", chan+1,
		    vd->range[chan]*100.0);
#endif
	    break;
	  case 1:
	    {
	      // channel fine tuning (14 bit value, range -100..+100 cents)
	      int value = (data_msb[chan]<<7) |
		data_lsb[chan];
	      vd->fine[chan] = (value-8192)/8192.0f;
	    }
	    goto master_tune;
	  case 2:
	    // channel coarse tuning (only msb is used, range -64..+63
	    // semitones)
	    vd->coarse[chan] = data_msb[chan]-64;
	  master_tune:
	    vd->tune[chan] = vd->coarse[chan]+
	      vd->fine[chan];
#if DEBUG_RPN
	    fprintf(stderr, "master-tuning (chan %d): %g cent\n", chan+1,
		    vd->tune[chan]*100.0);
#endif
	    update_voices(chan);
	    break;
	  default:
	    break;
	  }
	}
	break;
      default: {
#if FAUST_MIDICC
	// interpret all other controller changes according to the MIDI
	// controller map defined in the Faust plugin itself
	std::map<uint8_t,int>::iterator it = ctrlmap.find(data[1]);
	if (it != ctrlmap.end()) {
	  // defined MIDI controller
	  int j = inctrls[it->second],
	    k = ui[0]->elems[j].port;
	  float val = ctrlval(ui[0]->elems[j], data[2]);
	  midivals[chan][k] = val;
	  if (is_instr) {
	    // instrument: update running voices on this channel
	    for (boost::circular_buffer<int>::iterator it =
		   vd->used_voices.begin();
		 it != vd->used_voices.end(); it++) {
	      int i = *it;
	      if (vd->note_info[i].ch == chan)
		*ui[i]->elems[j].zone = val;
	    }
	  } else {
	    // simple effect: here we only have a single dsp instance and
	    // we're operating in omni mode, so we just update the control no
	    // matter what the midi channel is
	    *ui[0]->elems[j].zone = val;
	  }
#if DEBUG_MIDICC
	  fprintf(stderr, "ctrl-change chan %d, ctrl %d, val %d\n", chan+1,
		  data[1], data[2]);
#endif
	}
#endif
	break;
      }
      }
      break;
    }
    default:
      break;
    }
  }

  // Process an MTS sysex message and update the control values accordingly.

  void process_sysex(uint8_t *data, int sz)
  {
    if (!data || sz < 2) return;
#if DEBUG_MIDI
    fprintf(stderr, "midi sysex (%d bytes):", sz);
    for (int i = 0; i < sz; i++)
      fprintf(stderr, " 0x%0x", data[i]);
    fprintf(stderr, "\n");
#endif
    if (data[0] == 0xf0) {
      // Skip over the f0 and f7 status bytes in case they are included in the
      // dump.
      data++; sz--;
      if (data[sz-1] == 0xf7) sz--;
    }
    if ((data[0] == 0x7e || data[0] == 0x7f) && data[2] == 8) {
      // MIDI tuning standard
      bool realtime = data[0] == 0x7f;
      if ((sz == 19 && data[3] == 8) ||
	  (sz == 31 && data[3] == 9)) {
	// MTS scale/octave tuning 1- or 2-byte form
	bool onebyte = data[3] == 8;
	unsigned chanmsk = (data[4]<<14) | (data[5]<<7) | data[6];
	for (int i = 0; i < 12; i++) {
	  float t;
	  if (onebyte)
	    t = (data[i+7]-64)/100.0;
	  else
	    t = (((data[2*i+7]<<7)|data[2*i+8])-8192)/8192.0;
	  for (uint8_t ch = 0; ch < 16; ch++)
	    if (chanmsk & (1<<ch))
	      vd->tuning[ch][i] = t;
	}
	if (realtime) {
	  for (uint8_t ch = 0; ch < 16; ch++)
	    if (chanmsk & (1<<ch)) {
	      // update running voices on this channel
	      update_voices(ch);
	    }
	}
#if DEBUG_MTS
	fprintf(stderr, "octave-tuning-%s (chan ",
		realtime?"realtime":"non-realtime");
	bool first = true;
	for (uint8_t i = 0; i < 16; )
	  if (chanmsk & (1<<i)) {
	    uint8_t j;
	    for (j = i+1; j < 16 && (chanmsk&(1<<j)); )
	      j++;
	    if (first)
	      first = false;
	    else
	      fprintf(stderr, ",");
	    if (j > i+1)
	      fprintf(stderr, "%u-%u", i+1, j);
	    else
	      fprintf(stderr, "%u", i+1);
	    i = j;
	  } else
	    i++;
	fprintf(stderr, "):");
	if (onebyte) {
	  for (int i = 7; i < 19; i++) {
	    int val = data[i];
	    fprintf(stderr, " %d", val-64);
	  }
	} else {
	  for (int i = 7; i < 31; i++) {
	    int val = data[i++] << 7;
	    val |= data[i];
	    fprintf(stderr, " %g", ((double)val-8192.0)/8192.0*100.0);
	  }
	}
	fprintf(stderr, "\n");
#endif
      }
    }
  }

  // Change to a given preloaded tuning. The given tuning number may be in the
  // range 1..PFaustPlugin::n_tunings, zero denotes the default tuning (equal
  // temperament). This is only supported if FAUST_MTS is defined at compile
  // time.

  void change_tuning(int num)
  {
#if FAUST_MTS
    if (!mts || num == tuning_no) return;
    if (num < 0) num = 0;
    if (num > mts->tuning.size())
      num = mts->tuning.size();
    tuning_no = num;
    if (tuning_no > 0) {
      process_sysex(mts->tuning[tuning_no-1].data,
		    mts->tuning[tuning_no-1].len);
    } else {
      memset(vd->tuning, 0, sizeof(vd->tuning));
#if DEBUG_MTS
      fprintf(stderr,
	      "octave-tuning-default (chan 1-16): equal temperament\n");
#endif
    }
#endif
  }

};

Meta *LV2Plugin::meta = 0;
int LV2Plugin::n_tunings = 0;
#if FAUST_MTS
MTSTunings *LV2Plugin::mts = 0;
#endif

/* LV2-specific part starts here. ********************************************/

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
  LV2Plugin* plugin =
    new LV2Plugin(LV2Plugin::numVoices(), (int)rate);
  // Scan host features for URID map.
  for (int i = 0; features[i]; i++) {
    if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
      plugin->map = (LV2_URID_Map*)features[i]->data;
      plugin->midi_event =
	plugin->map->map(plugin->map->handle, MIDI_EVENT_URI);
    }
  }
  if (!plugin->map) {
    fprintf
      (stderr, "%s: host doesn't support urid:map, giving up\n",
       PLUGIN_URI);
    delete plugin;
    return 0;
  }
  return (LV2_Handle)plugin;
}

static void
cleanup(LV2_Handle instance)
{
  LV2Plugin* plugin = (LV2Plugin*)instance;
  delete plugin;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
  LV2Plugin* plugin = (LV2Plugin*)instance;
  int i = port, k = plugin->ui[0]->nports;
  int n = plugin->dsp[0]->getNumInputs(), m = plugin->dsp[0]->getNumOutputs();
  if (i < k)
    plugin->ports[i] = (float*)data;
  else {
    i -= k;
    if (i < n)
      plugin->inputs[i] = (float*)data;
    else {
      i -= n;
      if (i < m)
	plugin->outputs[i] = (float*)data;
      else if (i == m)
	plugin->event_port = (LV2_Atom_Sequence*)data;
      else if (i == m+1)
	plugin->poly = (float*)data;
      else if (i == m+2)
	plugin->tuning = (float*)data;
      else
	fprintf(stderr, "%s: bad port number %u\n", PLUGIN_URI, port);
    }
  }
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
  LV2Plugin* plugin = (LV2Plugin*)instance;
  // Process incoming MIDI events.
  if (plugin->event_port) {
    LV2_ATOM_SEQUENCE_FOREACH(plugin->event_port, ev) {
      if (ev->body.type == plugin->midi_event) {
	uint8_t *data = (uint8_t*)(ev+1);
#if 0
	// FIXME: Consider doing sample-accurate note onsets here. LV2 keeps
	// track of the exact onset in the frames and subframes fields
	// (http://lv2plug.in/ns/doc/html/structLV2__Atom__Event.html), but we
	// can't use that information at present, since our gate parameter is
	// a control variable which can only change at block boundaries. In
	// the future, the gate could be implemented as an audio signal to get
	// sample-accurate note onsets.
	uint32_t frames = ev->body.frames;
#endif
	if (data[0] == 0xf0)
	  plugin->process_sysex(data, ev->body.size);
	else
	  plugin->process_midi(data, ev->body.size);
      }
    }
  }
  // Process audio.
  plugin->process_audio(n_samples, plugin->inputs, plugin->outputs);
}

static void
activate(LV2_Handle instance)
{
  LV2Plugin* plugin = (LV2Plugin*)instance;
  plugin->resume();
}

static void
deactivate(LV2_Handle instance)
{
  LV2Plugin* plugin = (LV2Plugin*)instance;
  plugin->suspend();
}

const void*
extension_data(const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  PLUGIN_URI,
  instantiate,
  connect_port,
  activate,
  run,
  deactivate,
  cleanup,
  extension_data
};

extern "C"
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  switch (index) {
  case 0:
    return &descriptor;
  default:
    return NULL;
  }
}

//----------------------------------------------------------------------------
//  Dynamic manifest
//----------------------------------------------------------------------------

// NOTE: If your LV2 host doesn't offer this extension then you'll have to
// create a static ttl file with the descriptions of the ports. You can do
// this by compiling this code to a standalone executable. Running the
// executable then prints the manifest on stdout.

extern "C"
LV2_SYMBOL_EXPORT
int lv2_dyn_manifest_open(LV2_Dyn_Manifest_Handle *handle,
			  const LV2_Feature *const *features)
{
  LV2Plugin* plugin =
    new LV2Plugin(LV2Plugin::numVoices(), 48000);
  *handle = (LV2_Dyn_Manifest_Handle)plugin;
  return 0;
}

extern "C"
LV2_SYMBOL_EXPORT
int lv2_dyn_manifest_get_subjects(LV2_Dyn_Manifest_Handle handle,
				  FILE *fp)
{
  fprintf(fp, "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n\
<%s> a lv2:Plugin .\n", PLUGIN_URI);
  return 0;
}

#include <string>
#include <ctype.h>

static string mangle(const string &s)
{
  string t = s;
  size_t n = s.size();
  for (size_t i = 0; i < n; i++)
    if ((i == 0 && !isalpha(t[i]) && t[i] != '_') ||
	(!isalnum(t[i]) && t[i] != '_'))
      t[i] = '_';
  return t;
}

static unsigned steps(float min, float max, float step)
{
  if (step == 0.0) return 1;
  int n = (max-min)/step;
  if (n < 0) n = -n;
  if (n == 0) n = 1;
  return n;
}

#if FAUST_META
static bool is_xmlstring(const char *s)
{
  // This is just a basic sanity check. The string must not contain any
  // (unescaped) newlines, carriage returns or double quotes.
  return !strchr(s, '\n') && !strchr(s, '\r') && !strchr(s, '"');
}
#endif

extern "C"
LV2_SYMBOL_EXPORT
int lv2_dyn_manifest_get_data(LV2_Dyn_Manifest_Handle handle,
			      FILE *fp,
			      const char *uri)
{
  LV2Plugin* plugin = (LV2Plugin*)handle;
  int k = plugin->ui[0]->nports;
  int n = plugin->dsp[0]->getNumInputs(), m = plugin->dsp[0]->getNumOutputs();
  bool is_instr = plugin->maxvoices > 0, have_midi = is_instr;
  // Scan the global metadata for plugin name, description, license etc.
  const char *plugin_name = NULL, *plugin_author = NULL, *plugin_descr = NULL,
    *plugin_version = NULL, *plugin_license = NULL;
#if FAUST_META
  plugin_name = plugin->pluginName();
  plugin_descr = plugin->pluginDescription();
  plugin_author = plugin->pluginAuthor();
  plugin_version = plugin->pluginVersion();
  plugin_license = plugin->pluginLicense();
#endif
  if (!plugin_name || !*plugin_name) plugin_name = "parametric_eq";
  fprintf(fp, "@prefix doap:  <http://usefulinc.com/ns/doap#> .\n\
@prefix foaf:  <http://xmlns.com/foaf/0.1/> .\n\
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .\n\
@prefix ui:    <http://lv2plug.in/ns/extensions/ui#> .\n\
@prefix epp:   <http://lv2plug.in/ns/ext/port-props#> .\n\
@prefix atom:  <http://lv2plug.in/ns/ext/atom#> .\n\
@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n\
@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n\
@prefix units: <http://lv2plug.in/ns/extensions/units#> .\n\
@prefix urid:  <http://lv2plug.in/ns/ext/urid#> .\n\
<%s>\n\
       a lv2:Plugin%s ;\n\
       doap:name \"%s\" ;\n\
       lv2:binary <parametric_eq%s> ;\n\
       lv2:requiredFeature urid:map ;\n\
       lv2:optionalFeature epp:supportsStrictBounds ;\n\
       lv2:optionalFeature lv2:hardRTCapable ;\n", PLUGIN_URI,
	  is_instr?", lv2:InstrumentPlugin":"",
	  plugin_name, DLLEXT);
  if (plugin_author && *plugin_author)
    fprintf(fp, "\
       doap:maintainer [ foaf:name \"%s\" ] ;\n", plugin_author);
  // doap:description just seems to be ignored by all LV2 hosts anyway, so we
  // rather use rdfs:comment now which works with Ardour at least.
  if (plugin_descr && *plugin_descr)
    fprintf(fp, "\
       rdfs:comment \"%s\" ;\n", plugin_descr);
  if (plugin_version && *plugin_version)
    fprintf(fp, "\
       doap:revision \"%s\" ;\n", plugin_version);
  if (plugin_license && *plugin_license)
    fprintf(fp, "\
       doap:license \"%s\" ;\n", plugin_license);
#if FAUST_UI
    fprintf(fp, "\
       ui:ui <%sui> ;\n", PLUGIN_URI);
#endif
  int idx = 0;
  // control ports
  for (int i = 0; i < k; i++, idx++) {
    int j = plugin->ctrls[i];
    assert(idx == plugin->ui[0]->elems[j].port);
    fprintf(fp, "%s [\n", idx==0?"    lv2:port":" ,");
    const char *label = plugin->ui[0]->elems[j].label;
    assert(label);
    string sym = mangle(plugin->ui[0]->elems[j].label);
    switch (plugin->ui[0]->elems[j].type) {
    // active controls (input ports)
    case UI_BUTTON: case UI_CHECK_BUTTON:
    fprintf(fp, "\
	a lv2:InputPort ;\n\
	a lv2:ControlPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"%s_%d\" ;\n\
	lv2:name \"%s\" ;\n\
        lv2:portProperty epp:hasStrictBounds ;\n\
        lv2:portProperty lv2:toggled ;\n\
	lv2:default 0.00000 ;\n\
	lv2:minimum 0.00000 ;\n\
	lv2:maximum 1.00000 ;\n", idx, sym.c_str(), idx, label);
      break;
    case UI_NUM_ENTRY: case UI_H_SLIDER: case UI_V_SLIDER:
    fprintf(fp, "\
	a lv2:InputPort ;\n\
	a lv2:ControlPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"%s_%d\" ;\n\
	lv2:name \"%s\" ;\n\
        lv2:portProperty epp:hasStrictBounds ;\n\
        epp:rangeSteps %u ;\n\
	lv2:default %g ;\n\
	lv2:minimum %g ;\n\
	lv2:maximum %g ;\n", idx, sym.c_str(), idx, label,
	    steps(plugin->ui[0]->elems[j].min,
		  plugin->ui[0]->elems[j].max,
		  plugin->ui[0]->elems[j].step),
	    plugin->ui[0]->elems[j].init,
	    plugin->ui[0]->elems[j].min,
	    plugin->ui[0]->elems[j].max);
      break;
    // passive controls (output ports)
    case UI_H_BARGRAPH: case UI_V_BARGRAPH:
    fprintf(fp, "\
	a lv2:OutputPort ;\n\
	a lv2:ControlPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"%s_%d\" ;\n\
	lv2:name \"%s\" ;\n\
	lv2:default %g ;\n\
	lv2:minimum %g ;\n\
	lv2:maximum %g ;\n", idx, sym.c_str(), idx, label,
	    plugin->ui[0]->elems[j].min,
	    plugin->ui[0]->elems[j].min,
	    plugin->ui[0]->elems[j].max);
      break;
    default:
      assert(0 && "this can't happen");
      break;
    }
    // Scan for Faust control metadata we understand and add corresponding
    // hints to the LV2 description of the port.
    std::map< int, list<strpair> >::iterator it =
      plugin->ui[0]->metadata.find(j);
    if (it != plugin->ui[0]->metadata.end()) {
      for (std::list<strpair>::iterator jt = it->second.begin();
	   jt != it->second.end(); jt++) {
	const char *key = jt->first, *val = jt->second;
#if FAUST_MIDICC
	unsigned num;
	if (!strcmp(key, "midi") && sscanf(val, "ctrl %u", &num) == 1)
	  have_midi = true;
#endif
	if (!strcmp(key, "unit"))
	  fprintf(fp, "\
	units:unit [\n\
            a            units:Unit ;\n\
            units:name   \"%s\" ;\n\
            units:symbol \"%s\" ;\n\
            units:render \"%%f %s\"\n\
	] ;\n", val, val, val);
	else if (!strcmp(key, "scale") && !strcmp(val, "log"))
	  fprintf(fp, "\
	lv2:portProperty epp:logarithmic ;\n");
	else if (!strcmp(key, "tooltip"))
	  fprintf(fp, "\
	rdfs:comment \"%s\" ;\n", val);
	else if (strcmp(key, "lv2")) continue;
	if (!strcmp(val, "integer"))
	  fprintf(fp, "\
	lv2:portProperty lv2:integer ;\n");
	else if (!strcmp(val, "enumeration"))
	  fprintf(fp, "\
	lv2:portProperty lv2:enumeration ;\n");
	else if (!strcmp(val, "reportsLatency"))
	  fprintf(fp, "\
	lv2:portProperty lv2:reportsLatency ;\n\
	lv2:designation lv2:latency ;\n");
	else if (!strcmp(val, "hidden") || !strcmp(val, "notOnGUI"))
	  fprintf(fp, "\
	lv2:portProperty epp:notOnGUI ;\n");
	else if (!strncmp(val, "scalepoint", 10) ||
		 !strncmp(val, "scalePoint", 10)) {
	  val += 10;
	  if (!isspace(*val)) continue;
	  char *label = (char*)malloc(strlen(val)+1);
	  float point;
	  int pos;
	  while (sscanf(val, "%s %g%n", label, &point, &pos) == 2) {
	    fprintf(fp, "\
	lv2:scalePoint [ rdfs:label \"%s\"; rdf:value %g ] ;\n",
		    label, point);
	    val += pos;
	  }
	  free(label);
	} else
	  fprintf(stderr, "%s: bad port property '%s:%s'\n", PLUGIN_URI,
		  key, val);
      }
    }
    fprintf(fp, "    ]");
  }
  // audio inputs
  for (int i = 0; i < n; i++, idx++)
    fprintf(fp, "%s [\n\
	a lv2:InputPort ;\n\
	a lv2:AudioPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"in%d\" ;\n\
	lv2:name \"in%d\" ;\n\
    ]", idx==0?"    lv2:port":" ,", idx, i, i);
  // audio outputs
  for (int i = 0; i < m; i++, idx++)
    fprintf(fp, "%s [\n\
	a lv2:OutputPort ;\n\
	a lv2:AudioPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"out%d\" ;\n\
	lv2:name \"out%d\" ;\n\
    ]", idx==0?"    lv2:port":" ,", idx, i, i);
  if (have_midi) {
    // midi input
    fprintf(fp, "%s [\n\
	a lv2:InputPort ;\n\
	a atom:AtomPort ;\n\
	atom:bufferType atom:Sequence ;\n\
	atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent> ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"midiin\" ;\n\
	lv2:name \"midiin\"\n\
    ]", idx==0?"    lv2:port":" ,", idx);
    idx++;
  }
  if (is_instr) {
    // polyphony control
    fprintf(fp, "%s [\n\
	a lv2:InputPort ;\n\
	a lv2:ControlPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"polyphony\" ;\n\
	lv2:name \"polyphony\" ;\n\
        lv2:portProperty epp:hasStrictBounds ;\n\
#       lv2:portProperty epp:expensive ;\n\
        lv2:portProperty lv2:integer ;\n\
        epp:rangeSteps %d ;\n\
	lv2:default %d ;\n\
	lv2:minimum 1 ;\n\
	lv2:maximum %d ;\n\
    ]", idx==0?"    lv2:port":" ,", idx, plugin->maxvoices-1,
      plugin->maxvoices>1?plugin->maxvoices/2:1,
      plugin->maxvoices);
    idx++;
#if FAUST_MTS
    if (plugin->n_tunings > 0) {
      // tuning control
      fprintf(fp, "%s [\n\
	a lv2:InputPort ;\n\
	a lv2:ControlPort ;\n\
	lv2:index %d ;\n\
	lv2:symbol \"tuning\" ;\n\
	lv2:name \"tuning\" ;\n\
        lv2:portProperty epp:hasStrictBounds ;\n\
        lv2:portProperty lv2:integer ;\n\
        epp:rangeSteps %d ;\n\
	lv2:default 0 ;\n\
	lv2:minimum 0 ;\n\
	lv2:maximum %d ;\n",
	idx==0?"    lv2:port":" ,", idx, plugin->n_tunings, plugin->n_tunings);
      for (int i = 0; i <= plugin->n_tunings; i++)
	fprintf(fp, "\
	lv2:scalePoint [ rdfs:label \"%s\"; rdf:value %d ] ;\n",
		(i>0)?plugin->mts->tuning[i-1].name:"default", i);
      fprintf(fp, "    ]");
      idx++;
    }
#endif
  }
  fprintf(fp, "\n.\n");
  return 0;
}

extern "C"
LV2_SYMBOL_EXPORT
void lv2_dyn_manifest_close(LV2_Dyn_Manifest_Handle handle)
{
  LV2Plugin* plugin = (LV2Plugin*)handle;
  delete plugin;
}

int main()
{
  LV2_Dyn_Manifest_Handle handle;
  LV2_Feature **features = { NULL };
  int res = lv2_dyn_manifest_open(&handle, features);
  if (res) return res;
  res = lv2_dyn_manifest_get_data(handle, stdout, PLUGIN_URI);
  return res;
}

#endif
