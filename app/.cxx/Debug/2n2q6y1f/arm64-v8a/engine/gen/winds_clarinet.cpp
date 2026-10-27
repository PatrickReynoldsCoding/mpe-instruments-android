/* ------------------------------------------------------------
name: "Clarinet"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_winds_clarinet -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_winds_clarinet_H__
#define  __Inst_winds_clarinet_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_winds_clarinet
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

static float Inst_winds_clarinet_faustpower2_f(float value) {
	return value * value;
}

class Inst_winds_clarinet : public dsp {
	
 private:
	
	int iRec6[2];
	float fRec12[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	float fConst3;
	float fConst4;
	float fConst5;
	float fConst6;
	int iRec16[2];
	float fRec15[3];
	float fConst7;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	float fConst8;
	float fConst9;
	float fRec17[2];
	float fRec13[2];
	float fConst10;
	FAUSTFLOAT fHslider2;
	float fConst11;
	float fRec18[2];
	int IOTA0;
	float fRec14[2048];
	float fConst12;
	FAUSTFLOAT fHslider3;
	float fConst13;
	float fVec0[2];
	float fRec11[2];
	float fRec2[2048];
	float fRec0[2];
	
 public:
	Inst_winds_clarinet() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_winds_clarinet -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay4:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Reed clarinet. Press to blow harder, slide to change the reed brightness.");
		m->declare("filename", "clarinet.dsp");
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
		m->declare("name", "Clarinet");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "147 880");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
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
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = std::tan(6283.1855f / fConst0);
		fConst2 = 2.0f * (1.0f - 1.0f / Inst_winds_clarinet_faustpower2_f(fConst1));
		fConst3 = 1.0f / fConst1;
		fConst4 = (fConst3 + -1.4142135f) / fConst1 + 1.0f;
		fConst5 = (fConst3 + 1.4142135f) / fConst1 + 1.0f;
		fConst6 = 1.0f / fConst5;
		fConst7 = 0.05f / fConst5;
		fConst8 = std::exp(-(12.5f / fConst0));
		fConst9 = std::exp(-(2e+01f / fConst0));
		fConst10 = std::exp(-(33.333332f / fConst0));
		fConst11 = 1.0f - fConst10;
		fConst12 = 0.00882353f * fConst0;
		fConst13 = 0.0014705883f * fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider3 = static_cast<FAUSTFLOAT>(4.4e+02f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iRec6[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec12[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			iRec16[l2] = 0;
		}
		for (int l3 = 0; l3 < 3; l3 = l3 + 1) {
			fRec15[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec17[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec13[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec18[l6] = 0.0f;
		}
		IOTA0 = 0;
		for (int l7 = 0; l7 < 2048; l7 = l7 + 1) {
			fRec14[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fVec0[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec11[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2048; l10 = l10 + 1) {
			fRec2[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec0[l11] = 0.0f;
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
	
	virtual Inst_winds_clarinet* clone() {
		return new Inst_winds_clarinet();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Clarinet");
		ui_interface->addHorizontalSlider("freq", &fHslider3, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("slide", &fHslider2, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider1) * (0.44f * static_cast<float>(fHslider0) + 0.46f);
		float fSlow1 = fConst11 * (0.5f * static_cast<float>(fHslider2) + 0.3f);
		float fSlow2 = fConst13 * (1.7e+02f / static_cast<float>(fHslider3) + -0.05f);
		float fSlow3 = fSlow2 + -1.499995f;
		int iSlow4 = static_cast<int>(fSlow3);
		int iSlow5 = static_cast<int>(std::min<float>(fConst12, static_cast<float>(std::max<int>(0, iSlow4 + 4)))) + 1;
		float fSlow6 = std::floor(fSlow3);
		float fSlow7 = fSlow2 + (-3.0f - fSlow6);
		float fSlow8 = fSlow2 + (-2.0f - fSlow6);
		float fSlow9 = fSlow2 + (-1.0f - fSlow6);
		float fSlow10 = fSlow2 - fSlow6;
		float fSlow11 = fSlow10 * fSlow9;
		float fSlow12 = fSlow11 * fSlow8;
		float fSlow13 = 0.041666668f * fSlow12 * fSlow7;
		int iSlow14 = static_cast<int>(std::min<float>(fConst12, static_cast<float>(std::max<int>(0, iSlow4 + 3)))) + 1;
		float fSlow15 = 0.16666667f * fSlow12;
		int iSlow16 = static_cast<int>(std::min<float>(fConst12, static_cast<float>(std::max<int>(0, iSlow4 + 2)))) + 1;
		float fSlow17 = 0.25f * fSlow11;
		int iSlow18 = static_cast<int>(std::min<float>(fConst12, static_cast<float>(std::max<int>(0, iSlow4 + 1)))) + 1;
		float fSlow19 = 0.16666667f * fSlow10;
		int iSlow20 = static_cast<int>(std::min<float>(fConst12, static_cast<float>(std::max<int>(0, iSlow4)))) + 1;
		float fSlow21 = 0.041666668f * fSlow9;
		float fSlow22 = fSlow2 + (-4.0f - fSlow6);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iRec6[0] = 0;
			fRec12[0] = 0.5f * (fRec11[1] + fRec12[1]);
			float fRec10 = fRec12[0] + static_cast<float>(iRec6[1]);
			iRec16[0] = 1103515245 * iRec16[1] + 12345;
			fRec15[0] = 4.656613e-10f * static_cast<float>(iRec16[0]) - fConst6 * (fConst4 * fRec15[2] + fConst2 * fRec15[1]);
			float fTemp0 = ((fSlow0 > fRec17[1]) ? fConst9 : fConst8);
			fRec17[0] = fSlow0 * (1.0f - fTemp0) + fRec17[1] * fTemp0;
			float fTemp1 = fRec17[0] * (fConst7 * (fRec15[2] + fRec15[0] + 2.0f * fRec15[1]) + 1.0f);
			fRec13[0] = fRec0[1] + fTemp1;
			fRec18[0] = fSlow1 + fConst10 * fRec18[1];
			fRec14[IOTA0 & 2047] = fTemp1 - fRec13[1] * std::max<float>(-1.0f, std::min<float>(1.0f, 0.7f - fRec13[1] * (0.26f * fRec18[0] + -0.44f)));
			fVec0[0] = fSlow22 * (fSlow7 * (fSlow8 * (fSlow21 * fRec14[(IOTA0 - iSlow20) & 2047] - fSlow19 * fRec14[(IOTA0 - iSlow18) & 2047]) + fSlow17 * fRec14[(IOTA0 - iSlow16) & 2047]) - fSlow15 * fRec14[(IOTA0 - iSlow14) & 2047]) + fSlow13 * fRec14[(IOTA0 - iSlow5) & 2047];
			fRec11[0] = fVec0[1];
			float fRec7 = fRec10;
			float fRec8 = fRec11[0];
			float fRec9 = fRec11[0];
			fRec2[IOTA0 & 2047] = fRec7;
			float fRec3 = fSlow22 * (fSlow7 * (fSlow8 * (fSlow21 * fRec2[(IOTA0 - iSlow20) & 2047] - fSlow19 * fRec2[(IOTA0 - iSlow18) & 2047]) + fSlow17 * fRec2[(IOTA0 - iSlow16) & 2047]) - fSlow15 * fRec2[(IOTA0 - iSlow14) & 2047]) + fSlow13 * fRec2[(IOTA0 - iSlow5) & 2047];
			float fRec4 = fRec8;
			float fRec5 = fRec9;
			fRec0[0] = fRec3;
			float fRec1 = fRec5;
			output0[i0] = static_cast<FAUSTFLOAT>(0.9f * fRec1);
			iRec6[1] = iRec6[0];
			fRec12[1] = fRec12[0];
			iRec16[1] = iRec16[0];
			fRec15[2] = fRec15[1];
			fRec15[1] = fRec15[0];
			fRec17[1] = fRec17[0];
			fRec13[1] = fRec13[0];
			fRec18[1] = fRec18[0];
			IOTA0 = IOTA0 + 1;
			fVec0[1] = fVec0[0];
			fRec11[1] = fRec11[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#endif
