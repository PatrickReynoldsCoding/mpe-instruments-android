/* ------------------------------------------------------------
name: "FM Keys"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_synths_fm_keys -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_synths_fm_keys_H__
#define  __Inst_synths_fm_keys_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_synths_fm_keys
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

class Inst_synths_fm_keysSIG0 {
	
  private:
	
	int iVec0[2];
	int iRec0[2];
	
  public:
	
	int getNumInputsInst_synths_fm_keysSIG0() {
		return 0;
	}
	int getNumOutputsInst_synths_fm_keysSIG0() {
		return 1;
	}
	
	void instanceInitInst_synths_fm_keysSIG0(int sample_rate) {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			iRec0[l1] = 0;
		}
	}
	
	void fillInst_synths_fm_keysSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			iVec0[0] = 1;
			iRec0[0] = (iVec0[1] + iRec0[1]) % 65536;
			table[i1] = std::sin(9.58738e-05f * static_cast<float>(iRec0[0]));
			iVec0[1] = iVec0[0];
			iRec0[1] = iRec0[0];
		}
	}

};

static Inst_synths_fm_keysSIG0* newInst_synths_fm_keysSIG0() { return (Inst_synths_fm_keysSIG0*)new Inst_synths_fm_keysSIG0(); }
static void deleteInst_synths_fm_keysSIG0(Inst_synths_fm_keysSIG0* dsp) { delete dsp; }

class Inst_synths_fm_keysSIG1 {
	
  private:
	
	int iVec3[2];
	int iRec6[2];
	
  public:
	
	int getNumInputsInst_synths_fm_keysSIG1() {
		return 0;
	}
	int getNumOutputsInst_synths_fm_keysSIG1() {
		return 1;
	}
	
	void instanceInitInst_synths_fm_keysSIG1(int sample_rate) {
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			iVec3[l9] = 0;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			iRec6[l10] = 0;
		}
	}
	
	void fillInst_synths_fm_keysSIG1(int count, float* table) {
		for (int i2 = 0; i2 < count; i2 = i2 + 1) {
			iVec3[0] = 1;
			iRec6[0] = (iVec3[1] + iRec6[1]) % 65536;
			table[i2] = std::cos(9.58738e-05f * static_cast<float>(iRec6[0]));
			iVec3[1] = iVec3[0];
			iRec6[1] = iRec6[0];
		}
	}

};

static Inst_synths_fm_keysSIG1* newInst_synths_fm_keysSIG1() { return (Inst_synths_fm_keysSIG1*)new Inst_synths_fm_keysSIG1(); }
static void deleteInst_synths_fm_keysSIG1(Inst_synths_fm_keysSIG1* dsp) { delete dsp; }

static float ftbl0Inst_synths_fm_keysSIG0[65536];
static float Inst_synths_fm_keys_faustpower2_f(float value) {
	return value * value;
}
static float ftbl1Inst_synths_fm_keysSIG1[65536];

class Inst_synths_fm_keys : public dsp {
	
 private:
	
	int iVec1[2];
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec1[2];
	FAUSTFLOAT fHslider1;
	float fVec2[2];
	int iRec2[2];
	float fConst2;
	float fRec3[2];
	float fConst3;
	float fConst4;
	float fConst5;
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT fHslider3;
	float fConst6;
	float fConst7;
	float fRec4[2];
	float fConst8;
	float fRec5[2];
	float fRec7[2];
	
 public:
	Inst_synths_fm_keys() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_synths_fm_keys -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("description", "FM electric piano. Strike for a bright tine; press into the key to keep it ringing and brighter.");
		m->declare("envelopes.lib/adsr:author", "Yann Orlarey and Andrey Bundin");
		m->declare("envelopes.lib/author", "GRAME");
		m->declare("envelopes.lib/copyright", "GRAME");
		m->declare("envelopes.lib/license", "LGPL with exception");
		m->declare("envelopes.lib/name", "Faust Envelope Library");
		m->declare("envelopes.lib/version", "1.3.0");
		m->declare("filename", "fm-keys.dsp");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "FM Keys");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "65 1047");
		m->declare("ringout", "4");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/onePoleSwitching:author", "Jonatan Liljedahl, revised by Dario Sanfilippo");
		m->declare("signals.lib/onePoleSwitching:licence", "STK-4.3");
		m->declare("signals.lib/version", "1.6.0");
	}

	virtual int getNumInputs() {
		return 0;
	}
	virtual int getNumOutputs() {
		return 1;
	}
	
	static void classInit(int sample_rate) {
		Inst_synths_fm_keysSIG0* sig0 = newInst_synths_fm_keysSIG0();
		sig0->instanceInitInst_synths_fm_keysSIG0(sample_rate);
		sig0->fillInst_synths_fm_keysSIG0(65536, ftbl0Inst_synths_fm_keysSIG0);
		Inst_synths_fm_keysSIG1* sig1 = newInst_synths_fm_keysSIG1();
		sig1->instanceInitInst_synths_fm_keysSIG1(sample_rate);
		sig1->fillInst_synths_fm_keysSIG1(65536, ftbl1Inst_synths_fm_keysSIG1);
		deleteInst_synths_fm_keysSIG0(sig0);
		deleteInst_synths_fm_keysSIG1(sig1);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 14.0f / fConst0;
		fConst2 = 1.0f / std::max<float>(1.0f, 0.35f * fConst0);
		fConst3 = std::max<float>(1.0f, 0.002f * fConst0);
		fConst4 = 1.0f / fConst3;
		fConst5 = 1.0f / std::max<float>(1.0f, 1.1f * fConst0);
		fConst6 = std::exp(-(3.3333333f / fConst0));
		fConst7 = std::exp(-(2e+01f / fConst0));
		fConst8 = 1.0f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			iVec1[l2] = 0;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec1[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec2[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			iRec2[l5] = 0;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec3[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec4[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec5[l8] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec7[l11] = 0.0f;
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
	
	virtual Inst_synths_fm_keys* clone() {
		return new Inst_synths_fm_keys();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("FM Keys");
		ui_interface->addHorizontalSlider("freq", &fHslider0, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("velocity", &fHslider2, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider0);
		float fSlow1 = fConst1 * fSlow0;
		float fSlow2 = static_cast<float>(fHslider1);
		int iSlow3 = fSlow2 == 0.0f;
		float fSlow4 = static_cast<float>(fHslider2);
		float fSlow5 = 0.04f * fSlow4;
		float fSlow6 = fSlow2 * static_cast<float>(fHslider3);
		float fSlow7 = 2.2f * fSlow4 + 0.6f;
		float fSlow8 = fConst8 * fSlow0;
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec1[0] = 1;
			int iTemp0 = 1 - iVec1[1];
			float fTemp1 = ((iTemp0) ? 0.0f : fSlow1 + fRec1[1]);
			fRec1[0] = fTemp1 - std::floor(fTemp1);
			fVec2[0] = fSlow2;
			iRec2[0] = iSlow3 * (iRec2[1] + 1);
			fRec3[0] = fSlow2 + fRec3[1] * static_cast<float>(fVec2[1] >= fSlow2);
			float fTemp2 = std::max<float>(0.0f, std::min<float>(fConst4 * fRec3[0], std::max<float>(fConst5 * (fConst3 - fRec3[0]) + 1.0f, 0.0f)) * (1.0f - fConst2 * static_cast<float>(iRec2[0])));
			float fTemp3 = ((fSlow6 > fRec4[1]) ? fConst7 : fConst6);
			fRec4[0] = fSlow6 * (1.0f - fTemp3) + fRec4[1] * fTemp3;
			float fTemp4 = std::max<float>(fTemp2, fRec4[0]);
			float fTemp5 = ((iTemp0) ? 0.0f : fSlow8 + fRec5[1]);
			fRec5[0] = fTemp5 - std::floor(fTemp5);
			float fTemp6 = ftbl0Inst_synths_fm_keysSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec5[0]), 65535))] * (1.2f * fRec4[0] + fSlow7 * fTemp4);
			float fTemp7 = ((iTemp0) ? 0.0f : fSlow8 + fRec7[1]);
			fRec7[0] = fTemp7 - std::floor(fTemp7);
			int iTemp8 = std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec7[0]), 65535));
			output0[i0] = static_cast<FAUSTFLOAT>(0.5f * fTemp4 * (ftbl0Inst_synths_fm_keysSIG0[iTemp8] * std::cos(fTemp6) + ftbl1Inst_synths_fm_keysSIG1[iTemp8] * std::sin(fTemp6) + fSlow5 * Inst_synths_fm_keys_faustpower2_f(fTemp2) * ftbl0Inst_synths_fm_keysSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec1[0]), 65535))]));
			iVec1[1] = iVec1[0];
			fRec1[1] = fRec1[0];
			fVec2[1] = fVec2[0];
			iRec2[1] = iRec2[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec7[1] = fRec7[0];
		}
	}

};

#endif
