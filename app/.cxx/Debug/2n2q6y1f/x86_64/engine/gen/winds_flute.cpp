/* ------------------------------------------------------------
name: "Flute"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_winds_flute -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_winds_flute_H__
#define  __Inst_winds_flute_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_winds_flute
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

static float Inst_winds_flute_faustpower2_f(float value) {
	return value * value;
}

class Inst_winds_flute : public dsp {
	
 private:
	
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	float fConst2;
	float fRec0[2];
	float fConst3;
	float fConst4;
	FAUSTFLOAT fHslider1;
	float fConst5;
	float fRec2[2];
	int iRec18[2];
	float fRec24[2];
	float fRec25[2];
	int IOTA0;
	float fRec26[2048];
	float fConst6;
	float fConst7;
	FAUSTFLOAT fHslider2;
	float fConst8;
	float fVec0[2];
	float fConst9;
	float fConst10;
	float fConst11;
	float fConst12;
	float fConst13;
	float fConst14;
	int iRec28[2];
	float fRec27[3];
	float fConst15;
	FAUSTFLOAT fHslider3;
	float fConst16;
	float fConst17;
	float fRec29[2];
	float fVec1[2048];
	float fConst18;
	float fVec2[2];
	float fRec23[2];
	float fRec14[2048];
	float fRec10[2];
	float fRec6[2048];
	float fRec4[2];
	float fRec5[2];
	float fRec3[2];
	float fRec1[3];
	
 public:
	Inst_winds_flute() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_winds_flute -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay4:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Breathy flute. Press to blow harder, slide to brighten or soften the tone.");
		m->declare("filename", "flute.dsp");
		m->declare("filters.lib/dcblocker:author", "Julius O. Smith III");
		m->declare("filters.lib/dcblocker:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/dcblocker:license", "MIT-style STK-4.3 license");
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
		m->declare("filters.lib/pole:author", "Julius O. Smith III");
		m->declare("filters.lib/pole:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/pole:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.7.1");
		m->declare("filters.lib/zero:author", "Julius O. Smith III");
		m->declare("filters.lib/zero:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/zero:license", "MIT-style STK-4.3 license");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Flute");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "294 1047");
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
		fConst1 = std::exp(-(33.333332f / fConst0));
		fConst2 = 1.0f - fConst1;
		fConst3 = 3.1415927f / fConst0;
		fConst4 = std::exp(-(5e+01f / fConst0));
		fConst5 = 1.0f - fConst4;
		fConst6 = 0.00882353f * fConst0;
		fConst7 = 32.5f / fConst0;
		fConst8 = 0.00039705881f * fConst0;
		fConst9 = std::tan(6283.1855f / fConst0);
		fConst10 = 2.0f * (1.0f - 1.0f / Inst_winds_flute_faustpower2_f(fConst9));
		fConst11 = 1.0f / fConst9;
		fConst12 = (fConst11 + -1.4142135f) / fConst9 + 1.0f;
		fConst13 = (fConst11 + 1.4142135f) / fConst9 + 1.0f;
		fConst14 = 1.0f / fConst13;
		fConst15 = 0.05f / fConst13;
		fConst16 = std::exp(-(1e+01f / fConst0));
		fConst17 = std::exp(-(16.666666f / fConst0));
		fConst18 = 0.0010735294f * fConst0;
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
			iRec18[l2] = 0;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec24[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec25[l4] = 0.0f;
		}
		IOTA0 = 0;
		for (int l5 = 0; l5 < 2048; l5 = l5 + 1) {
			fRec26[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fVec0[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			iRec28[l7] = 0;
		}
		for (int l8 = 0; l8 < 3; l8 = l8 + 1) {
			fRec27[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec29[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2048; l10 = l10 + 1) {
			fVec1[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fVec2[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec23[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2048; l13 = l13 + 1) {
			fRec14[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec10[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2048; l15 = l15 + 1) {
			fRec6[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec4[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec5[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec3[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 3; l19 = l19 + 1) {
			fRec1[l19] = 0.0f;
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
	
	virtual Inst_winds_flute* clone() {
		return new Inst_winds_flute();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Flute");
		ui_interface->addHorizontalSlider("freq", &fHslider2, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("slide", &fHslider0, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = fConst2 * (8.5e+03f * static_cast<float>(fHslider0) + 1.5e+03f);
		float fSlow1 = static_cast<float>(fHslider1);
		float fSlow2 = fConst5 * (0.65f * fSlow1 + 0.35f);
		float fSlow3 = static_cast<float>(fHslider2);
		float fSlow4 = 3.4e+02f * std::max<float>(1.4409221f * (1.0f / (fSlow3 * std::pow(2.0f, 0.00083333335f * (34.62468f * std::log(0.0034013605f * fSlow3) + 8.0f))) - fConst7), 0.00016666666f) + 0.27f;
		float fSlow5 = fConst8 * fSlow4;
		float fSlow6 = fSlow5 + -1.499995f;
		int iSlow7 = static_cast<int>(fSlow6);
		int iSlow8 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow7 + 4)))) + 1;
		float fSlow9 = std::floor(fSlow6);
		float fSlow10 = fSlow5 + (-3.0f - fSlow9);
		float fSlow11 = fSlow5 + (-2.0f - fSlow9);
		float fSlow12 = fSlow5 + (-1.0f - fSlow9);
		float fSlow13 = fSlow5 - fSlow9;
		float fSlow14 = fSlow13 * fSlow12;
		float fSlow15 = fSlow14 * fSlow11;
		float fSlow16 = 0.041666668f * fSlow15 * fSlow10;
		int iSlow17 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow7 + 3)))) + 1;
		float fSlow18 = 0.16666667f * fSlow15;
		int iSlow19 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow7 + 2)))) + 1;
		float fSlow20 = 0.25f * fSlow14;
		int iSlow21 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow7 + 1)))) + 1;
		float fSlow22 = 0.16666667f * fSlow13;
		int iSlow23 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow7)))) + 1;
		float fSlow24 = 0.041666668f * fSlow12;
		float fSlow25 = fSlow5 + (-4.0f - fSlow9);
		float fSlow26 = static_cast<float>(fHslider3) * (0.3f * fSlow1 + 0.58f);
		float fSlow27 = fConst18 * fSlow4;
		float fSlow28 = fSlow27 + -1.499995f;
		int iSlow29 = static_cast<int>(fSlow28);
		int iSlow30 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow29 + 4)))) + 1;
		float fSlow31 = std::floor(fSlow28);
		float fSlow32 = fSlow27 + (-3.0f - fSlow31);
		float fSlow33 = fSlow27 + (-2.0f - fSlow31);
		float fSlow34 = fSlow27 + (-1.0f - fSlow31);
		float fSlow35 = fSlow27 - fSlow31;
		float fSlow36 = fSlow35 * fSlow34;
		float fSlow37 = fSlow36 * fSlow33;
		float fSlow38 = 0.041666668f * fSlow37 * fSlow32;
		int iSlow39 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow29 + 3)))) + 1;
		float fSlow40 = 0.16666667f * fSlow37;
		int iSlow41 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow29 + 2)))) + 1;
		float fSlow42 = 0.25f * fSlow36;
		int iSlow43 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow29 + 1)))) + 1;
		float fSlow44 = 0.16666667f * fSlow35;
		int iSlow45 = static_cast<int>(std::min<float>(fConst6, static_cast<float>(std::max<int>(0, iSlow29)))) + 1;
		float fSlow46 = 0.041666668f * fSlow34;
		float fSlow47 = fSlow27 + (-4.0f - fSlow31);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec0[0] = fSlow0 + fConst1 * fRec0[1];
			float fTemp0 = std::tan(fConst3 * fRec0[0]);
			float fTemp1 = 1.0f / fTemp0;
			float fTemp2 = (fTemp1 + 1.4142135f) / fTemp0 + 1.0f;
			fRec2[0] = fSlow2 + fConst4 * fRec2[1];
			iRec18[0] = 0;
			fRec24[0] = 0.285f * fRec23[1] + 0.7f * fRec24[1];
			float fRec22 = fRec24[0] + static_cast<float>(iRec18[1]);
			fRec25[0] = fRec4[1];
			fRec26[IOTA0 & 2047] = 0.95f * fRec25[1];
			fVec0[0] = fSlow25 * (fSlow10 * (fSlow11 * (fSlow24 * fRec26[(IOTA0 - iSlow23) & 2047] - fSlow22 * fRec26[(IOTA0 - iSlow21) & 2047]) + fSlow20 * fRec26[(IOTA0 - iSlow19) & 2047]) - fSlow18 * fRec26[(IOTA0 - iSlow17) & 2047]) + fSlow16 * fRec26[(IOTA0 - iSlow8) & 2047];
			iRec28[0] = 1103515245 * iRec28[1] + 12345;
			fRec27[0] = 4.656613e-10f * static_cast<float>(iRec28[0]) - fConst14 * (fConst12 * fRec27[2] + fConst10 * fRec27[1]);
			float fTemp3 = ((fSlow26 > fRec29[1]) ? fConst17 : fConst16);
			fRec29[0] = fSlow26 * (1.0f - fTemp3) + fRec29[1] * fTemp3;
			float fTemp4 = fRec29[0] * (fConst15 * (fRec27[2] + fRec27[0] + 2.0f * fRec27[1]) + 1.0f) - 0.5f * fVec0[1];
			float fTemp5 = 0.5f * fRec10[1] + std::max<float>(-1.0f, std::min<float>(1.0f, fTemp4 * (Inst_winds_flute_faustpower2_f(fTemp4) + -1.0f)));
			fVec1[IOTA0 & 2047] = fTemp5;
			fVec2[0] = fSlow47 * (fSlow32 * (fSlow33 * (fSlow46 * fVec1[(IOTA0 - iSlow45) & 2047] - fSlow44 * fVec1[(IOTA0 - iSlow43) & 2047]) + fSlow42 * fVec1[(IOTA0 - iSlow41) & 2047]) - fSlow40 * fVec1[(IOTA0 - iSlow39) & 2047]) + fSlow38 * fVec1[(IOTA0 - iSlow30) & 2047];
			fRec23[0] = fVec2[1];
			float fRec19 = fRec22;
			float fRec20 = fRec23[0];
			float fRec21 = fRec23[0];
			fRec14[IOTA0 & 2047] = fRec19;
			float fRec15 = fSlow47 * (fSlow32 * (fSlow33 * (fSlow46 * fRec14[(IOTA0 - iSlow45) & 2047] - fSlow44 * fRec14[(IOTA0 - iSlow43) & 2047]) + fSlow42 * fRec14[(IOTA0 - iSlow41) & 2047]) - fSlow40 * fRec14[(IOTA0 - iSlow39) & 2047]) + fSlow38 * fRec14[(IOTA0 - iSlow30) & 2047];
			float fRec16 = fRec20;
			float fRec17 = fRec21;
			fRec10[0] = fRec15;
			float fRec11 = fRec10[1];
			float fRec12 = fRec16;
			float fRec13 = fRec17;
			fRec6[IOTA0 & 2047] = fRec11;
			float fRec7 = fSlow25 * (fSlow10 * (fSlow11 * (fSlow24 * fRec6[(IOTA0 - iSlow23) & 2047] - fSlow22 * fRec6[(IOTA0 - iSlow21) & 2047]) + fSlow20 * fRec6[(IOTA0 - iSlow19) & 2047]) - fSlow18 * fRec6[(IOTA0 - iSlow17) & 2047]) + fSlow16 * fRec6[(IOTA0 - iSlow8) & 2047];
			float fRec8 = fRec12;
			float fRec9 = fRec13;
			fRec4[0] = fRec7;
			fRec5[0] = fRec9;
			fRec3[0] = fRec5[0] + 0.995f * fRec3[1] - fRec5[1];
			fRec1[0] = 3.5f * fRec3[0] * fRec2[0] - (fRec1[2] * ((fTemp1 + -1.4142135f) / fTemp0 + 1.0f) + 2.0f * fRec1[1] * (1.0f - 1.0f / Inst_winds_flute_faustpower2_f(fTemp0))) / fTemp2;
			output0[i0] = static_cast<FAUSTFLOAT>(0.8f * tanhf((fRec1[2] + fRec1[0] + 2.0f * fRec1[1]) / fTemp2));
			fRec0[1] = fRec0[0];
			fRec2[1] = fRec2[0];
			iRec18[1] = iRec18[0];
			fRec24[1] = fRec24[0];
			fRec25[1] = fRec25[0];
			IOTA0 = IOTA0 + 1;
			fVec0[1] = fVec0[0];
			iRec28[1] = iRec28[0];
			fRec27[2] = fRec27[1];
			fRec27[1] = fRec27[0];
			fRec29[1] = fRec29[0];
			fVec2[1] = fVec2[0];
			fRec23[1] = fRec23[0];
			fRec10[1] = fRec10[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec3[1] = fRec3[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
		}
	}

};

#endif
