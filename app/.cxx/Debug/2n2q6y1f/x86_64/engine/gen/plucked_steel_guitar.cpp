/* ------------------------------------------------------------
name: "Steel Guitar"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_plucked_steel_guitar -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_plucked_steel_guitar_H__
#define  __Inst_plucked_steel_guitar_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_plucked_steel_guitar
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

static float Inst_plucked_steel_guitar_faustpower2_f(float value) {
	return value * value;
}

class Inst_plucked_steel_guitar : public dsp {
	
 private:
	
	int iRec10[2];
	int IOTA0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	float fConst2;
	float fRec22[2];
	float fRec25[2];
	FAUSTFLOAT fHslider2;
	float fVec0[2];
	int iRec27[2];
	float fConst3;
	float fConst4;
	int iRec29[2];
	float fRec28[3];
	FAUSTFLOAT fHslider3;
	float fRec30[4];
	float fRec31[2048];
	float fVec1[2];
	float fVec2[2];
	float fRec26[2048];
	float fRec19[2];
	float fRec16[2048];
	float fRec18[2];
	float fRec15[4];
	int iRec6[2];
	float fRec2[2048];
	float fRec0[2];
	
 public:
	Inst_plucked_steel_guitar() {
	}
	
	void metadata(Meta* m) { 
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_plucked_steel_guitar -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay4:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Plucked steel string. Slide sets where the string is plucked; it rings after you lift.");
		m->declare("envelopes.lib/ar:author", "Yann Orlarey, Stéphane Letz");
		m->declare("envelopes.lib/author", "GRAME");
		m->declare("envelopes.lib/copyright", "GRAME");
		m->declare("envelopes.lib/license", "LGPL with exception");
		m->declare("envelopes.lib/name", "Faust Envelope Library");
		m->declare("envelopes.lib/version", "1.3.0");
		m->declare("filename", "steel-guitar.dsp");
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
		m->declare("name", "Steel Guitar");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "98 659");
		m->declare("ringout", "8");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
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
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 0.00882353f * fConst0;
		fConst2 = 0.0014705883f * fConst0;
		fConst3 = 0.002f * fConst0;
		fConst4 = 15.707963f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.5f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iRec10[l0] = 0;
		}
		IOTA0 = 0;
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec22[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec25[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fVec0[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			iRec27[l4] = 0;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			iRec29[l5] = 0;
		}
		for (int l6 = 0; l6 < 3; l6 = l6 + 1) {
			fRec28[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 4; l7 = l7 + 1) {
			fRec30[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2048; l8 = l8 + 1) {
			fRec31[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fVec1[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fVec2[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2048; l11 = l11 + 1) {
			fRec26[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec19[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2048; l13 = l13 + 1) {
			fRec16[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec18[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 4; l15 = l15 + 1) {
			fRec15[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			iRec6[l16] = 0;
		}
		for (int l17 = 0; l17 < 2048; l17 = l17 + 1) {
			fRec2[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec0[l18] = 0.0f;
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
	
	virtual Inst_plucked_steel_guitar* clone() {
		return new Inst_plucked_steel_guitar();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Steel Guitar");
		ui_interface->addHorizontalSlider("freq", &fHslider0, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("slide", &fHslider1, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("velocity", &fHslider3, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider0);
		float fSlow1 = 3.4e+02f / fSlow0 + -0.1f;
		float fSlow2 = 0.7f * static_cast<float>(fHslider1);
		float fSlow3 = fConst2 * (0.85f - fSlow2) * fSlow1;
		float fSlow4 = fSlow3 + -1.499995f;
		int iSlow5 = static_cast<int>(fSlow4);
		int iSlow6 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow5 + 4))));
		int iSlow7 = iSlow6 + 1;
		float fSlow8 = std::floor(fSlow4);
		float fSlow9 = fSlow3 + (-3.0f - fSlow8);
		float fSlow10 = fSlow3 + (-2.0f - fSlow8);
		float fSlow11 = fSlow3 + (-1.0f - fSlow8);
		float fSlow12 = fSlow3 - fSlow8;
		float fSlow13 = fSlow12 * fSlow11;
		float fSlow14 = fSlow13 * fSlow10;
		float fSlow15 = 0.041666668f * fSlow14 * fSlow9;
		int iSlow16 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow5 + 3))));
		int iSlow17 = iSlow16 + 1;
		float fSlow18 = 0.16666667f * fSlow14;
		int iSlow19 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow5 + 2))));
		int iSlow20 = iSlow19 + 1;
		float fSlow21 = 0.25f * fSlow13;
		int iSlow22 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow5 + 1))));
		int iSlow23 = iSlow22 + 1;
		float fSlow24 = 0.16666667f * fSlow12;
		int iSlow25 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow5))));
		int iSlow26 = iSlow25 + 1;
		float fSlow27 = 0.041666668f * fSlow11;
		float fSlow28 = fSlow3 + (-4.0f - fSlow8);
		float fSlow29 = static_cast<float>(fHslider2);
		float fSlow30 = 1.0f / std::max<float>(1.0f, fConst3 * Inst_plucked_steel_guitar_faustpower2_f(1.0f - 0.00033333333f * fSlow0));
		float fSlow31 = std::tan(fConst4 * fSlow0);
		float fSlow32 = 2.0f * (1.0f - 1.0f / Inst_plucked_steel_guitar_faustpower2_f(fSlow31));
		float fSlow33 = 1.0f / fSlow31;
		float fSlow34 = (fSlow33 + -1.4142135f) / fSlow31 + 1.0f;
		float fSlow35 = (fSlow33 + 1.4142135f) / fSlow31 + 1.0f;
		float fSlow36 = 1.0f / fSlow35;
		float fSlow37 = (0.7f * static_cast<float>(fHslider3) + 0.3f) / fSlow35;
		float fSlow38 = fConst2 * (fSlow2 + 0.15f) * fSlow1;
		float fSlow39 = fSlow38 + -1.499995f;
		int iSlow40 = static_cast<int>(fSlow39);
		int iSlow41 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow40 + 4))));
		int iSlow42 = iSlow41 + 2;
		float fSlow43 = std::floor(fSlow39);
		float fSlow44 = fSlow38 + (-3.0f - fSlow43);
		float fSlow45 = fSlow38 + (-2.0f - fSlow43);
		float fSlow46 = fSlow38 + (-1.0f - fSlow43);
		float fSlow47 = fSlow38 - fSlow43;
		float fSlow48 = fSlow47 * fSlow46;
		float fSlow49 = fSlow48 * fSlow45;
		float fSlow50 = 0.041666668f * fSlow49 * fSlow44;
		int iSlow51 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow40 + 3))));
		int iSlow52 = iSlow51 + 2;
		float fSlow53 = 0.16666667f * fSlow49;
		int iSlow54 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow40 + 2))));
		int iSlow55 = iSlow54 + 2;
		float fSlow56 = 0.25f * fSlow48;
		int iSlow57 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow40 + 1))));
		int iSlow58 = iSlow57 + 2;
		float fSlow59 = 0.16666667f * fSlow47;
		int iSlow60 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow40))));
		int iSlow61 = iSlow60 + 2;
		float fSlow62 = 0.041666668f * fSlow46;
		float fSlow63 = fSlow38 + (-4.0f - fSlow43);
		int iSlow64 = iSlow41 + 1;
		int iSlow65 = iSlow51 + 1;
		int iSlow66 = iSlow54 + 1;
		int iSlow67 = iSlow57 + 1;
		int iSlow68 = iSlow60 + 1;
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iRec10[0] = 0;
			int iRec11 = iRec10[1];
			float fRec14 = static_cast<float>(iRec6[1]) - 0.9978437f * (0.7f * fRec15[2] + 0.15f * (fRec15[1] + fRec15[3]));
			fRec22[0] = fSlow28 * (fSlow9 * (fSlow10 * (fSlow27 * fRec2[(IOTA0 - iSlow26) & 2047] - fSlow24 * fRec2[(IOTA0 - iSlow23) & 2047]) + fSlow21 * fRec2[(IOTA0 - iSlow20) & 2047]) - fSlow18 * fRec2[(IOTA0 - iSlow17) & 2047]) + fSlow15 * fRec2[(IOTA0 - iSlow7) & 2047];
			fRec25[0] = 0.95f * fRec22[1] + 0.05f * fRec25[1];
			float fRec23 = fRec25[0];
			fVec0[0] = fSlow29;
			iRec27[0] = (iRec27[1] + (iRec27[1] > 0)) * (fSlow29 <= fVec0[1]) + (fSlow29 > fVec0[1]);
			float fTemp0 = fSlow30 * static_cast<float>(iRec27[0]);
			iRec29[0] = 1103515245 * iRec29[1] + 12345;
			fRec28[0] = 4.656613e-10f * static_cast<float>(iRec29[0]) - fSlow36 * (fSlow34 * fRec28[2] + fSlow32 * fRec28[1]);
			float fTemp1 = fSlow37 * (fRec28[2] + fRec28[0] + 2.0f * fRec28[1]) * std::max<float>(0.0f, std::min<float>(fTemp0, 2.0f - fTemp0));
			fRec30[0] = fRec0[1];
			fRec31[IOTA0 & 2047] = -(0.9978437f * (0.7f * fRec30[2] + 0.15f * (fRec30[1] + fRec30[3])));
			fVec1[0] = fSlow63 * (fSlow44 * (fSlow45 * (fSlow62 * fRec31[(IOTA0 - iSlow61) & 2047] - fSlow59 * fRec31[(IOTA0 - iSlow58) & 2047]) + fSlow56 * fRec31[(IOTA0 - iSlow55) & 2047]) - fSlow53 * fRec31[(IOTA0 - iSlow52) & 2047]) + fSlow50 * fRec31[(IOTA0 - iSlow42) & 2047];
			fVec2[0] = fVec1[1] + fTemp1;
			fRec26[IOTA0 & 2047] = 0.95f * fVec2[1] + 0.05f * fRec26[(IOTA0 - 1) & 2047];
			float fRec24 = fSlow28 * (fSlow9 * (fSlow10 * (fSlow27 * fRec26[(IOTA0 - iSlow25) & 2047] - fSlow24 * fRec26[(IOTA0 - iSlow22) & 2047]) + fSlow21 * fRec26[(IOTA0 - iSlow19) & 2047]) - fSlow18 * fRec26[(IOTA0 - iSlow16) & 2047]) + fSlow15 * fRec26[(IOTA0 - iSlow6) & 2047];
			fRec19[0] = fRec23;
			float fRec20 = fTemp1 + fRec19[1];
			float fRec21 = fRec24;
			fRec16[IOTA0 & 2047] = fRec20;
			float fRec17 = fSlow63 * (fSlow44 * (fSlow45 * (fSlow62 * fRec16[(IOTA0 - iSlow68) & 2047] - fSlow59 * fRec16[(IOTA0 - iSlow67) & 2047]) + fSlow56 * fRec16[(IOTA0 - iSlow66) & 2047]) - fSlow53 * fRec16[(IOTA0 - iSlow65) & 2047]) + fSlow50 * fRec16[(IOTA0 - iSlow64) & 2047];
			fRec18[0] = fRec21;
			fRec15[0] = fRec18[1];
			float fRec12 = fRec15[1];
			float fRec13 = fRec15[1];
			iRec6[0] = iRec11;
			float fRec7 = fRec14;
			float fRec8 = fRec12;
			float fRec9 = fRec13;
			fRec2[IOTA0 & 2047] = fRec7;
			float fRec3 = fRec17;
			float fRec4 = fRec8;
			float fRec5 = fRec9;
			fRec0[0] = fRec3;
			float fRec1 = fRec5;
			output0[i0] = static_cast<FAUSTFLOAT>(0.6f * tanhf(12.0f * fRec1));
			iRec10[1] = iRec10[0];
			IOTA0 = IOTA0 + 1;
			fRec22[1] = fRec22[0];
			fRec25[1] = fRec25[0];
			fVec0[1] = fVec0[0];
			iRec27[1] = iRec27[0];
			iRec29[1] = iRec29[0];
			fRec28[2] = fRec28[1];
			fRec28[1] = fRec28[0];
			for (int j0 = 3; j0 > 0; j0 = j0 - 1) {
				fRec30[j0] = fRec30[j0 - 1];
			}
			fVec1[1] = fVec1[0];
			fVec2[1] = fVec2[0];
			fRec19[1] = fRec19[0];
			fRec18[1] = fRec18[0];
			for (int j1 = 3; j1 > 0; j1 = j1 - 1) {
				fRec15[j1] = fRec15[j1 - 1];
			}
			iRec6[1] = iRec6[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#endif
