/* ------------------------------------------------------------
name: "Violin"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_strings_violin -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_strings_violin_H__
#define  __Inst_strings_violin_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_strings_violin
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

static float Inst_strings_violin_faustpower2_f(float value) {
	return value * value;
}

class Inst_strings_violin : public dsp {
	
 private:
	
	int iRec18[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	float fConst3;
	float fConst4;
	float fConst5;
	float fConst6;
	float fRec25[4];
	int IOTA0;
	float fRec26[8192];
	float fConst7;
	float fConst8;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	float fConst9;
	float fConst10;
	float fRec27[2];
	float fConst11;
	float fConst12;
	float fVec0[2];
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT fHslider3;
	float fConst13;
	float fConst14;
	float fRec28[2];
	float fRec29[2];
	float fVec1[8192];
	float fVec2[2];
	float fRec24[4];
	float fRec22[3];
	float fConst15;
	float fConst16;
	float fConst17;
	float fConst18;
	float fConst19;
	float fConst20;
	float fConst21;
	float fRec30[3];
	float fConst22;
	float fConst23;
	float fConst24;
	float fConst25;
	float fConst26;
	float fConst27;
	float fConst28;
	float fRec31[3];
	float fConst29;
	int iRec14[2];
	float fRec10[8192];
	float fRec6[2];
	float fRec2[8192];
	float fRec0[2];
	
 public:
	Inst_strings_violin() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_strings_violin -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay4:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Bowed string. Press to bow, slide to move the bow towards the bridge.");
		m->declare("filename", "violin.dsp");
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
		m->declare("name", "Violin");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "196 1000");
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
		fConst1 = std::tan(7853.9814f / fConst0);
		fConst2 = 2.0f * (1.0f - 1.0f / Inst_strings_violin_faustpower2_f(fConst1));
		fConst3 = 1.0f / fConst1;
		fConst4 = (fConst3 + -0.6666667f) / fConst1 + 1.0f;
		fConst5 = (fConst3 + 0.6666667f) / fConst1 + 1.0f;
		fConst6 = 1.0f / fConst5;
		fConst7 = 0.023529412f * fConst0;
		fConst8 = std::exp(-(33.333332f / fConst0));
		fConst9 = 9.0f / fConst0;
		fConst10 = 1.0f - fConst8;
		fConst11 = 12.1f / fConst0;
		fConst12 = 0.5f * fConst0;
		fConst13 = std::exp(-(12.5f / fConst0));
		fConst14 = std::exp(-(2e+01f / fConst0));
		fConst15 = 0.5f / (fConst1 * fConst5);
		fConst16 = std::tan(1570.7964f / fConst0);
		fConst17 = 2.0f * (1.0f - 1.0f / Inst_strings_violin_faustpower2_f(fConst16));
		fConst18 = 1.0f / fConst16;
		fConst19 = (fConst18 + -0.33333334f) / fConst16 + 1.0f;
		fConst20 = (fConst18 + 0.33333334f) / fConst16 + 1.0f;
		fConst21 = 1.0f / fConst20;
		fConst22 = 0.8f / (fConst16 * fConst20);
		fConst23 = std::tan(848.23004f / fConst0);
		fConst24 = 2.0f * (1.0f - 1.0f / Inst_strings_violin_faustpower2_f(fConst23));
		fConst25 = 1.0f / fConst23;
		fConst26 = (fConst25 + -0.33333334f) / fConst23 + 1.0f;
		fConst27 = (fConst25 + 0.33333334f) / fConst23 + 1.0f;
		fConst28 = 1.0f / fConst27;
		fConst29 = 0.7f / (fConst23 * fConst27);
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider1 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iRec18[l0] = 0;
		}
		for (int l1 = 0; l1 < 4; l1 = l1 + 1) {
			fRec25[l1] = 0.0f;
		}
		IOTA0 = 0;
		for (int l2 = 0; l2 < 8192; l2 = l2 + 1) {
			fRec26[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec27[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec0[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec28[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec29[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 8192; l7 = l7 + 1) {
			fVec1[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fVec2[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 4; l9 = l9 + 1) {
			fRec24[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 3; l10 = l10 + 1) {
			fRec22[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = l11 + 1) {
			fRec30[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 3; l12 = l12 + 1) {
			fRec31[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			iRec14[l13] = 0;
		}
		for (int l14 = 0; l14 < 8192; l14 = l14 + 1) {
			fRec10[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec6[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 8192; l16 = l16 + 1) {
			fRec2[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec0[l17] = 0.0f;
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
	
	virtual Inst_strings_violin* clone() {
		return new Inst_strings_violin();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Violin");
		ui_interface->addHorizontalSlider("freq", &fHslider1, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("slide", &fHslider0, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider1);
		float fSlow1 = fConst10 * std::min<float>(0.45f, std::max<float>(0.14f * static_cast<float>(fHslider0) + 0.06f, fConst9 * fSlow0));
		float fSlow2 = fConst12 * std::max<float>(1.0f / fSlow0 - fConst11, 0.00025f);
		float fSlow3 = static_cast<float>(fHslider2);
		float fSlow4 = static_cast<float>(fHslider3) * (0.6f * std::sqrt(fSlow3) + 0.15f);
		float fSlow5 = fConst10 * (0.3f * fSlow3 + 0.45f);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iRec18[0] = 0;
			int iRec19 = iRec18[1];
			float fRec23 = static_cast<float>(iRec14[1]) - 0.98926467f * (0.6f * fRec24[2] + 0.2f * (fRec24[1] + fRec24[3]));
			fRec25[0] = fRec0[1];
			fRec26[IOTA0 & 8191] = -(0.99880147f * (0.8f * fRec25[2] + 0.1f * (fRec25[1] + fRec25[3])));
			fRec27[0] = fSlow1 + fConst8 * fRec27[1];
			float fTemp0 = fSlow2 * fRec27[0];
			float fTemp1 = fTemp0 + -1.499995f;
			int iTemp2 = static_cast<int>(fTemp1);
			int iTemp3 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp2 + 4)))) + 1;
			float fTemp4 = std::floor(fTemp1);
			float fTemp5 = fTemp0 + (-3.0f - fTemp4);
			float fTemp6 = fTemp0 + (-2.0f - fTemp4);
			float fTemp7 = fTemp0 + (-1.0f - fTemp4);
			float fTemp8 = fTemp0 - fTemp4;
			float fTemp9 = fTemp8 * fTemp7;
			float fTemp10 = fTemp9 * fTemp6;
			float fTemp11 = fTemp10 * fTemp5;
			int iTemp12 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp2 + 3)))) + 1;
			int iTemp13 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp2 + 2)))) + 1;
			int iTemp14 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp2 + 1)))) + 1;
			int iTemp15 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp2)))) + 1;
			float fTemp16 = fTemp0 + (-4.0f - fTemp4);
			fVec0[0] = fTemp16 * (fTemp5 * (fTemp6 * (0.041666668f * fRec26[(IOTA0 - iTemp15) & 8191] * fTemp7 - 0.16666667f * fTemp8 * fRec26[(IOTA0 - iTemp14) & 8191]) + 0.25f * fTemp9 * fRec26[(IOTA0 - iTemp13) & 8191]) - 0.16666667f * fTemp10 * fRec26[(IOTA0 - iTemp12) & 8191]) + 0.041666668f * fTemp11 * fRec26[(IOTA0 - iTemp3) & 8191];
			float fTemp17 = ((fSlow4 > fRec28[1]) ? fConst14 : fConst13);
			fRec28[0] = fSlow4 * (1.0f - fTemp17) + fRec28[1] * fTemp17;
			float fTemp18 = fRec28[0] - (fRec6[1] + fVec0[1]);
			fRec29[0] = fSlow5 + fConst8 * fRec29[1];
			float fTemp19 = fTemp18 * std::min<float>(1.0f, std::pow(std::fabs((5.0f - 4.0f * fRec29[0]) * fTemp18) + 0.75f, -4.0f));
			float fTemp20 = fVec0[1] + fTemp19;
			fVec1[IOTA0 & 8191] = fTemp20;
			float fTemp21 = fSlow2 * (1.0f - fRec27[0]);
			float fTemp22 = fTemp21 + -1.499995f;
			int iTemp23 = static_cast<int>(fTemp22);
			int iTemp24 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp23 + 4)))) + 1;
			float fTemp25 = std::floor(fTemp22);
			float fTemp26 = fTemp21 + (-3.0f - fTemp25);
			float fTemp27 = fTemp21 + (-2.0f - fTemp25);
			float fTemp28 = fTemp21 + (-1.0f - fTemp25);
			float fTemp29 = fTemp21 - fTemp25;
			float fTemp30 = fTemp29 * fTemp28;
			float fTemp31 = fTemp30 * fTemp27;
			float fTemp32 = fTemp31 * fTemp26;
			int iTemp33 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp23 + 3)))) + 1;
			int iTemp34 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp23 + 2)))) + 1;
			int iTemp35 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp23 + 1)))) + 1;
			int iTemp36 = static_cast<int>(std::min<float>(fConst7, static_cast<float>(std::max<int>(0, iTemp23)))) + 1;
			float fTemp37 = fTemp21 + (-4.0f - fTemp25);
			fVec2[0] = fTemp37 * (fTemp26 * (fTemp27 * (0.041666668f * fVec1[(IOTA0 - iTemp36) & 8191] * fTemp28 - 0.16666667f * fTemp29 * fVec1[(IOTA0 - iTemp35) & 8191]) + 0.25f * fTemp30 * fVec1[(IOTA0 - iTemp34) & 8191]) - 0.16666667f * fTemp31 * fVec1[(IOTA0 - iTemp33) & 8191]) + 0.041666668f * fTemp32 * fVec1[(IOTA0 - iTemp24) & 8191];
			fRec24[0] = fVec2[1];
			fRec22[0] = fRec24[1] - fConst6 * (fConst4 * fRec22[2] + fConst2 * fRec22[1]);
			fRec30[0] = fRec24[1] - fConst21 * (fConst19 * fRec30[2] + fConst17 * fRec30[1]);
			fRec31[0] = fRec24[1] - fConst28 * (fConst26 * fRec31[2] + fConst24 * fRec31[1]);
			float fTemp38 = fConst29 * (fRec31[0] - fRec31[2]) + fConst22 * (fRec30[0] - fRec30[2]) + fConst15 * (fRec22[0] - fRec22[2]);
			float fRec20 = fTemp38;
			float fRec21 = fTemp38;
			iRec14[0] = iRec19;
			float fRec15 = fRec23;
			float fRec16 = fRec20;
			float fRec17 = fRec21;
			fRec10[IOTA0 & 8191] = fRec15;
			float fRec11 = fTemp37 * (fTemp26 * (fTemp27 * (0.041666668f * fTemp28 * fRec10[(IOTA0 - iTemp36) & 8191] - 0.16666667f * fTemp29 * fRec10[(IOTA0 - iTemp35) & 8191]) + 0.25f * fTemp30 * fRec10[(IOTA0 - iTemp34) & 8191]) - 0.16666667f * fTemp31 * fRec10[(IOTA0 - iTemp33) & 8191]) + 0.041666668f * fTemp32 * fRec10[(IOTA0 - iTemp24) & 8191];
			float fRec12 = fRec16;
			float fRec13 = fRec17;
			fRec6[0] = fRec11;
			float fRec7 = fRec6[1] + fTemp19;
			float fRec8 = fRec12;
			float fRec9 = fRec13;
			fRec2[IOTA0 & 8191] = fRec7;
			float fRec3 = fTemp16 * (fTemp5 * (fTemp6 * (0.041666668f * fTemp7 * fRec2[(IOTA0 - iTemp15) & 8191] - 0.16666667f * fTemp8 * fRec2[(IOTA0 - iTemp14) & 8191]) + 0.25f * fTemp9 * fRec2[(IOTA0 - iTemp13) & 8191]) - 0.16666667f * fTemp10 * fRec2[(IOTA0 - iTemp12) & 8191]) + 0.041666668f * fTemp11 * fRec2[(IOTA0 - iTemp3) & 8191];
			float fRec4 = fRec8;
			float fRec5 = fRec9;
			fRec0[0] = fRec3;
			float fRec1 = fRec5;
			output0[i0] = static_cast<FAUSTFLOAT>(0.5f * fRec1);
			iRec18[1] = iRec18[0];
			for (int j0 = 3; j0 > 0; j0 = j0 - 1) {
				fRec25[j0] = fRec25[j0 - 1];
			}
			IOTA0 = IOTA0 + 1;
			fRec27[1] = fRec27[0];
			fVec0[1] = fVec0[0];
			fRec28[1] = fRec28[0];
			fRec29[1] = fRec29[0];
			fVec2[1] = fVec2[0];
			for (int j1 = 3; j1 > 0; j1 = j1 - 1) {
				fRec24[j1] = fRec24[j1 - 1];
			}
			fRec22[2] = fRec22[1];
			fRec22[1] = fRec22[0];
			fRec30[2] = fRec30[1];
			fRec30[1] = fRec30[0];
			fRec31[2] = fRec31[1];
			fRec31[1] = fRec31[0];
			iRec14[1] = iRec14[0];
			fRec6[1] = fRec6[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#endif
