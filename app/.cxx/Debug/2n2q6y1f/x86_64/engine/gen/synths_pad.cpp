/* ------------------------------------------------------------
name: "Pad"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_synths_pad -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_synths_pad_H__
#define  __Inst_synths_pad_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_synths_pad
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

static float Inst_synths_pad_faustpower2_f(float value) {
	return value * value;
}

class Inst_synths_pad : public dsp {
	
 private:
	
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	float fConst2;
	float fRec0[2];
	FAUSTFLOAT fHslider2;
	float fConst3;
	float fConst4;
	float fRec2[2];
	float fRec4[2];
	float fRec6[2];
	float fRec1[3];
	FAUSTFLOAT fHslider3;
	float fConst5;
	float fConst6;
	float fRec8[2];
	float fRec10[2];
	float fRec12[2];
	float fRec9[3];
	
 public:
	Inst_synths_pad() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_synths_pad -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("description", "Warm detuned saw pad. Press to open the filter, slide to add brightness; slow attack and release.");
		m->declare("filename", "pad.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
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
		m->declare("name", "Pad");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/saw2ptr:author", "Julius O. Smith III");
		m->declare("oscillators.lib/saw2ptr:license", "STK-4.3");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "65 1047");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/onePoleSwitching:author", "Jonatan Liljedahl, revised by Dario Sanfilippo");
		m->declare("signals.lib/onePoleSwitching:licence", "STK-4.3");
		m->declare("signals.lib/version", "1.6.0");
	}

	virtual int getNumInputs() {
		return 0;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = std::exp(-(33.333332f / fConst0));
		fConst2 = 1.0f - fConst1;
		fConst3 = 3.1415927f / fConst0;
		fConst4 = 1.0f / fConst0;
		fConst5 = std::exp(-(1.4285715f / fConst0));
		fConst6 = std::exp(-(2.857143f / fConst0));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec0[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec2[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec4[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec6[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 3; l4 = l4 + 1) {
			fRec1[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec8[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec10[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec12[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 3; l8 = l8 + 1) {
			fRec9[l8] = 0.0f;
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
	
	virtual Inst_synths_pad* clone() {
		return new Inst_synths_pad();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Pad");
		ui_interface->addHorizontalSlider("freq", &fHslider2, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("slide", &fHslider0, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = static_cast<float>(fHslider1);
		float fSlow1 = fConst2 * (5.5e+03f * (0.9f * fSlow0 + 0.1f) * (0.65f * static_cast<float>(fHslider0) + 0.35f) + 3.5e+02f);
		float fSlow2 = static_cast<float>(fHslider2);
		float fSlow3 = 12.0f * fSlow2;
		float fSlow4 = std::max<float>(1.1920929e-07f, std::fabs(fSlow2));
		float fSlow5 = fConst4 * fSlow4;
		float fSlow6 = 1.0f - fConst0 / fSlow4;
		float fSlow7 = std::max<float>(1.1920929e-07f, std::fabs(0.996f * fSlow2));
		float fSlow8 = fConst4 * fSlow7;
		float fSlow9 = 1.0f - fConst0 / fSlow7;
		float fSlow10 = std::max<float>(1.1920929e-07f, std::fabs(1.004f * fSlow2));
		float fSlow11 = fConst4 * fSlow10;
		float fSlow12 = 1.0f - fConst0 / fSlow10;
		float fSlow13 = static_cast<float>(fHslider3) * (0.6f * fSlow0 + 0.4f);
		float fSlow14 = std::max<float>(1.1920929e-07f, std::fabs(0.994f * fSlow2));
		float fSlow15 = fConst4 * fSlow14;
		float fSlow16 = 1.0f - fConst0 / fSlow14;
		float fSlow17 = std::max<float>(1.1920929e-07f, std::fabs(1.006f * fSlow2));
		float fSlow18 = fConst4 * fSlow17;
		float fSlow19 = 1.0f - fConst0 / fSlow17;
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec0[0] = fSlow1 + fConst1 * fRec0[1];
			float fTemp0 = std::tan(fConst3 * std::min<float>(fRec0[0], fSlow3));
			float fTemp1 = 1.0f / fTemp0;
			float fTemp2 = (fTemp1 + 1.4142135f) / fTemp0 + 1.0f;
			float fTemp3 = 1.0f - 1.0f / Inst_synths_pad_faustpower2_f(fTemp0);
			float fTemp4 = (fTemp1 + -1.4142135f) / fTemp0 + 1.0f;
			float fTemp5 = fSlow5 + fRec2[1] + -1.0f;
			int iTemp6 = fTemp5 < 0.0f;
			float fTemp7 = fSlow5 + fRec2[1];
			fRec2[0] = ((iTemp6) ? fTemp7 : fTemp5);
			float fRec3 = ((iTemp6) ? fTemp7 : fSlow5 + fRec2[1] + fSlow6 * fTemp5);
			float fTemp8 = fSlow8 + fRec4[1] + -1.0f;
			int iTemp9 = fTemp8 < 0.0f;
			float fTemp10 = fSlow8 + fRec4[1];
			fRec4[0] = ((iTemp9) ? fTemp10 : fTemp8);
			float fRec5 = ((iTemp9) ? fTemp10 : fSlow8 + fRec4[1] + fSlow9 * fTemp8);
			float fTemp11 = fSlow11 + fRec6[1] + -1.0f;
			int iTemp12 = fTemp11 < 0.0f;
			float fTemp13 = fSlow11 + fRec6[1];
			fRec6[0] = ((iTemp12) ? fTemp13 : fTemp11);
			float fRec7 = ((iTemp12) ? fTemp13 : fSlow11 + fRec6[1] + fSlow12 * fTemp11);
			fRec1[0] = 0.33333334f * (2.0f * (fRec7 - (1.0f - (fRec5 + fRec3))) + -1.0f) - (fRec1[2] * fTemp4 + 2.0f * fRec1[1] * fTemp3) / fTemp2;
			float fTemp14 = ((fSlow13 > fRec8[1]) ? fConst6 : fConst5);
			fRec8[0] = fSlow13 * (1.0f - fTemp14) + fRec8[1] * fTemp14;
			output0[i0] = static_cast<FAUSTFLOAT>(0.5f * (fRec8[0] * (fRec1[2] + fRec1[0] + 2.0f * fRec1[1]) / fTemp2));
			float fTemp15 = fSlow15 + fRec10[1] + -1.0f;
			int iTemp16 = fTemp15 < 0.0f;
			float fTemp17 = fSlow15 + fRec10[1];
			fRec10[0] = ((iTemp16) ? fTemp17 : fTemp15);
			float fRec11 = ((iTemp16) ? fTemp17 : fSlow15 + fRec10[1] + fSlow16 * fTemp15);
			float fTemp18 = fSlow18 + fRec12[1] + -1.0f;
			int iTemp19 = fTemp18 < 0.0f;
			float fTemp20 = fSlow18 + fRec12[1];
			fRec12[0] = ((iTemp19) ? fTemp20 : fTemp18);
			float fRec13 = ((iTemp19) ? fTemp20 : fSlow18 + fRec12[1] + fSlow19 * fTemp18);
			fRec9[0] = 0.33333334f * (2.0f * (fRec13 - (1.0f - (fRec3 + fRec11))) + -1.0f) - (fTemp4 * fRec9[2] + 2.0f * fTemp3 * fRec9[1]) / fTemp2;
			output1[i0] = static_cast<FAUSTFLOAT>(0.5f * (fRec8[0] * (fRec9[2] + fRec9[0] + 2.0f * fRec9[1]) / fTemp2));
			fRec0[1] = fRec0[0];
			fRec2[1] = fRec2[0];
			fRec4[1] = fRec4[0];
			fRec6[1] = fRec6[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec8[1] = fRec8[0];
			fRec10[1] = fRec10[0];
			fRec12[1] = fRec12[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
		}
	}

};

#endif
