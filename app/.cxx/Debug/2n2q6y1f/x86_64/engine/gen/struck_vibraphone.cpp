/* ------------------------------------------------------------
name: "Vibraphone"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_struck_vibraphone -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_struck_vibraphone_H__
#define  __Inst_struck_vibraphone_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_struck_vibraphone
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

class Inst_struck_vibraphoneSIG0 {
	
  private:
	
	int iVec0[2];
	int iRec0[2];
	
  public:
	
	int getNumInputsInst_struck_vibraphoneSIG0() {
		return 0;
	}
	int getNumOutputsInst_struck_vibraphoneSIG0() {
		return 1;
	}
	
	void instanceInitInst_struck_vibraphoneSIG0(int sample_rate) {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			iRec0[l1] = 0;
		}
	}
	
	void fillInst_struck_vibraphoneSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			iVec0[0] = 1;
			iRec0[0] = (iVec0[1] + iRec0[1]) % 65536;
			table[i1] = std::sin(9.58738e-05f * static_cast<float>(iRec0[0]));
			iVec0[1] = iVec0[0];
			iRec0[1] = iRec0[0];
		}
	}

};

static Inst_struck_vibraphoneSIG0* newInst_struck_vibraphoneSIG0() { return (Inst_struck_vibraphoneSIG0*)new Inst_struck_vibraphoneSIG0(); }
static void deleteInst_struck_vibraphoneSIG0(Inst_struck_vibraphoneSIG0* dsp) { delete dsp; }

static float ftbl0Inst_struck_vibraphoneSIG0[65536];
static float Inst_struck_vibraphone_faustpower2_f(float value) {
	return value * value;
}

class Inst_struck_vibraphone : public dsp {
	
 private:
	
	int iVec1[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec1[2];
	FAUSTFLOAT fHslider0;
	float fConst2;
	FAUSTFLOAT fHslider1;
	float fVec2[2];
	float fConst3;
	float fRec2[2];
	FAUSTFLOAT fHslider2;
	float fConst4;
	int iVec3[2];
	int iRec4[2];
	float fConst5;
	float fConst6;
	float fConst7;
	int iRec5[2];
	FAUSTFLOAT fHslider3;
	float fRec3[3];
	float fRec6[2];
	float fConst8;
	float fRec7[3];
	float fRec8[2];
	float fConst9;
	float fRec9[3];
	
 public:
	Inst_struck_vibraphone() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_struck_vibraphone -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("description", "Struck metal bar with a damper: rings while your finger is down, stops when you lift. Press for tremolo.");
		m->declare("envelopes.lib/ar:author", "Yann Orlarey, Stéphane Letz");
		m->declare("envelopes.lib/author", "GRAME");
		m->declare("envelopes.lib/copyright", "GRAME");
		m->declare("envelopes.lib/license", "LGPL with exception");
		m->declare("envelopes.lib/name", "Faust Envelope Library");
		m->declare("envelopes.lib/version", "1.3.0");
		m->declare("filename", "vibraphone.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/resonbp:author", "Julius O. Smith III");
		m->declare("filters.lib/resonbp:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/resonbp:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.7.1");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Vibraphone");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "175 1400");
		m->declare("ringout", "8");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "1.6.0");
	}

	virtual int getNumInputs() {
		return 0;
	}
	virtual int getNumOutputs() {
		return 1;
	}
	
	static void classInit(int sample_rate) {
		Inst_struck_vibraphoneSIG0* sig0 = newInst_struck_vibraphoneSIG0();
		sig0->instanceInitInst_struck_vibraphoneSIG0(sample_rate);
		sig0->fillInst_struck_vibraphoneSIG0(65536, ftbl0Inst_struck_vibraphoneSIG0);
		deleteInst_struck_vibraphoneSIG0(sig0);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 5.5f / fConst0;
		fConst2 = std::exp(-(5e+01f / fConst0));
		fConst3 = 1.0f - fConst2;
		fConst4 = 31.415926f / fConst0;
		fConst5 = std::max<float>(1.0f, 0.0004f * fConst0);
		fConst6 = 1.0f / fConst5;
		fConst7 = 1.0f / std::max<float>(1.0f, 0.005f * fConst0);
		fConst8 = 12.566371f / fConst0;
		fConst9 = 3.1415927f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.5f);
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
			fRec2[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			iVec3[l6] = 0;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			iRec4[l7] = 0;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			iRec5[l8] = 0;
		}
		for (int l9 = 0; l9 < 3; l9 = l9 + 1) {
			fRec3[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec6[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = l11 + 1) {
			fRec7[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec8[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 3; l13 = l13 + 1) {
			fRec9[l13] = 0.0f;
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
	
	virtual Inst_struck_vibraphone* clone() {
		return new Inst_struck_vibraphone();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Vibraphone");
		ui_interface->addHorizontalSlider("freq", &fHslider2, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("velocity", &fHslider3, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = 0.225f * static_cast<float>(fHslider0);
		float fSlow1 = static_cast<float>(fHslider1);
		float fSlow2 = 0.5f * (1.0f - fSlow1);
		float fSlow3 = fConst3 * (0.9f * fSlow1 + fSlow2);
		float fSlow4 = static_cast<float>(fHslider2);
		float fSlow5 = 4.546444f * fSlow4;
		float fSlow6 = std::tan(fConst4 * fSlow4);
		float fSlow7 = 1.0f / fSlow6;
		float fSlow8 = 2.0f * (1.0f - 1.0f / Inst_struck_vibraphone_faustpower2_f(fSlow6));
		float fSlow9 = 2.3283064e-10f * (static_cast<float>(fHslider3) + 1.0f);
		float fSlow10 = 0.2f / fSlow6;
		float fSlow11 = fConst3 * (2.2f * fSlow1 + fSlow2);
		float fSlow12 = 1.8185775f * fSlow4;
		float fSlow13 = std::tan(fConst8 * fSlow4);
		float fSlow14 = 1.0f / fSlow13;
		float fSlow15 = 2.0f * (1.0f - 1.0f / Inst_struck_vibraphone_faustpower2_f(fSlow13));
		float fSlow16 = 0.45f / fSlow13;
		float fSlow17 = fConst3 * (5.0f * fSlow1 + fSlow2);
		float fSlow18 = 0.45464438f * fSlow4;
		float fSlow19 = std::tan(fConst9 * fSlow4);
		float fSlow20 = 1.0f / fSlow19;
		float fSlow21 = 2.0f * (1.0f - 1.0f / Inst_struck_vibraphone_faustpower2_f(fSlow19));
		float fSlow22 = 1.3f * std::pow(2e+02f / fSlow4, 0.65f);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec1[0] = 1;
			float fTemp0 = ((1 - iVec1[1]) ? 0.0f : fConst1 + fRec1[1]);
			fRec1[0] = fTemp0 - std::floor(fTemp0);
			fVec2[0] = fSlow1;
			fRec2[0] = fSlow3 + fConst2 * fRec2[1];
			float fTemp1 = 1.0f / std::max<float>(1.0f, fSlow5 * fRec2[0]);
			float fTemp2 = fSlow7 * (fSlow7 + fTemp1) + 1.0f;
			int iTemp3 = fSlow1 > fVec2[1];
			iVec3[0] = iTemp3;
			iRec4[0] = (iRec4[1] + (iRec4[1] > 0)) * (iTemp3 <= iVec3[1]) + (iTemp3 > iVec3[1]);
			float fTemp4 = static_cast<float>(iRec4[0]);
			iRec5[0] = 1103515245 * iRec5[1] + 12345;
			float fTemp5 = fSlow9 * static_cast<float>(iRec5[0]) * std::max<float>(0.0f, std::min<float>(fConst6 * fTemp4, fConst7 * (fConst5 - fTemp4) + 1.0f));
			fRec3[0] = fTemp5 - (fRec3[2] * (fSlow7 * (fSlow7 - fTemp1) + 1.0f) + fSlow8 * fRec3[1]) / fTemp2;
			fRec6[0] = fSlow11 + fConst2 * fRec6[1];
			float fTemp6 = 1.0f / std::max<float>(1.0f, fSlow12 * fRec6[0]);
			float fTemp7 = fSlow14 * (fSlow14 + fTemp6) + 1.0f;
			fRec7[0] = fTemp5 - (fRec7[2] * (fSlow14 * (fSlow14 - fTemp6) + 1.0f) + fSlow15 * fRec7[1]) / fTemp7;
			fRec8[0] = fSlow17 + fConst2 * fRec8[1];
			float fTemp8 = 1.0f / std::max<float>(1.0f, fSlow18 * fRec8[0]);
			float fTemp9 = fSlow20 * (fSlow20 + fTemp8) + 1.0f;
			fRec9[0] = fTemp5 - (fRec9[2] * (fSlow20 * (fSlow20 - fTemp8) + 1.0f) + fSlow21 * fRec9[1]) / fTemp9;
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow22 * (fSlow20 * ((fRec9[0] - fRec9[2]) / fTemp9) + fSlow16 * ((fRec7[0] - fRec7[2]) / fTemp7) + fSlow10 * ((fRec3[0] - fRec3[2]) / fTemp2)) * (1.0f - fSlow0 * (ftbl0Inst_struck_vibraphoneSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec1[0]), 65535))] + 1.0f)));
			iVec1[1] = iVec1[0];
			fRec1[1] = fRec1[0];
			fVec2[1] = fVec2[0];
			fRec2[1] = fRec2[0];
			iVec3[1] = iVec3[0];
			iRec4[1] = iRec4[0];
			iRec5[1] = iRec5[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
		}
	}

};

#endif
