/* ------------------------------------------------------------
name: "Bass"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_voices_bass -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_voices_bass_H__
#define  __Inst_voices_bass_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_voices_bass
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

class Inst_voices_bassSIG0 {
	
  private:
	
	int iVec0[2];
	int iRec0[2];
	
  public:
	
	int getNumInputsInst_voices_bassSIG0() {
		return 0;
	}
	int getNumOutputsInst_voices_bassSIG0() {
		return 1;
	}
	
	void instanceInitInst_voices_bassSIG0(int sample_rate) {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			iRec0[l1] = 0;
		}
	}
	
	void fillInst_voices_bassSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			iVec0[0] = 1;
			iRec0[0] = (iVec0[1] + iRec0[1]) % 65536;
			table[i1] = std::sin(9.58738e-05f * static_cast<float>(iRec0[0]));
			iVec0[1] = iVec0[0];
			iRec0[1] = iRec0[0];
		}
	}

};

static Inst_voices_bassSIG0* newInst_voices_bassSIG0() { return (Inst_voices_bassSIG0*)new Inst_voices_bassSIG0(); }
static void deleteInst_voices_bassSIG0(Inst_voices_bassSIG0* dsp) { delete dsp; }

static float ftbl0Inst_voices_bassSIG0[65536];

class Inst_voices_bass : public dsp {
	
 private:
	
	int iVec1[2];
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fRec2[2];
	float fVec2[2];
	float fConst2;
	FAUSTFLOAT fHslider1;
	float fConst3;
	float fRec3[2];
	float fRec1[2];
	float fConst4;
	float fConst5;
	float fRec4[2];
	float fConst6;
	float fRec5[2];
	float fRec6[3];
	float fRec7[2];
	float fRec8[2];
	float fRec9[2];
	float fRec10[3];
	float fRec11[2];
	float fRec12[2];
	float fRec13[2];
	float fRec14[3];
	float fRec15[2];
	float fRec16[2];
	float fRec17[2];
	float fRec18[3];
	float fRec19[2];
	float fRec20[2];
	float fRec21[2];
	float fRec22[3];
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT fHslider3;
	float fConst7;
	float fConst8;
	float fRec23[2];
	
 public:
	Inst_voices_bass() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/listInterp:author", "Romain Michon");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_voices_bass -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("description", "Sung vowel (bass). Press to sing louder, slide to morph a - e - i - o - u.");
		m->declare("filename", "bass.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.7.1");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "Bass");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("oscillators.lib/hs_oscsin:author", "Mike Olsen");
		m->declare("oscillators.lib/hs_phasor:author", "Mike Olsen, revised by Stéphane Letz");
		m->declare("oscillators.lib/lf_sawpos:author", "Bart Brouns, revised by Stéphane Letz");
		m->declare("oscillators.lib/lf_sawpos:licence", "STK-4.3");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.6.0");
		m->declare("physmodels.lib/SFFormantModel:author", "Mike Olsen");
		m->declare("physmodels.lib/SFFormantModelFofSmooth:author", "Mike Olsen");
		m->declare("physmodels.lib/autobendFreq:author", "Mike Olsen");
		m->declare("physmodels.lib/fof:author", "Mike Olsen");
		m->declare("physmodels.lib/fofSmooth:author", "Mike Olsen");
		m->declare("physmodels.lib/formantFilterFofSmooth:author", "Mike Olsen");
		m->declare("physmodels.lib/formantFilterbank:author", "Mike Olsen");
		m->declare("physmodels.lib/formantFilterbankFofSmooth:author", "Mike Olsen");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/skirtWidthMultiplier:author", "Mike Olsen");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("physmodels.lib/vocalEffort:author", "Mike Olsen");
		m->declare("physmodels.lib/voiceGender:author", "Mike Olsen");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "82 294");
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
		Inst_voices_bassSIG0* sig0 = newInst_voices_bassSIG0();
		sig0->instanceInitInst_voices_bassSIG0(sample_rate);
		sig0->fillInst_voices_bassSIG0(65536, ftbl0Inst_voices_bassSIG0);
		deleteInst_voices_bassSIG0(sig0);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 1.0f / fConst0;
		fConst2 = std::exp(-(2e+01f / fConst0));
		fConst3 = 4.0f * (1.0f - fConst2);
		fConst4 = std::exp(-(1e+03f / fConst0));
		fConst5 = 1.0f - fConst4;
		fConst6 = 3.1415927f / fConst0;
		fConst7 = std::exp(-(3.3333333f / fConst0));
		fConst8 = std::exp(-(6.6666665f / fConst0));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			iVec1[l2] = 0;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec2[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec2[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec3[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec1[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec4[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec5[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 3; l9 = l9 + 1) {
			fRec6[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec7[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec8[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec9[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 3; l13 = l13 + 1) {
			fRec10[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec11[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec12[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec13[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 3; l17 = l17 + 1) {
			fRec14[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec15[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fRec16[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec17[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 3; l21 = l21 + 1) {
			fRec18[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec19[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = l23 + 1) {
			fRec20[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = l24 + 1) {
			fRec21[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 3; l25 = l25 + 1) {
			fRec22[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			fRec23[l26] = 0.0f;
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
	
	virtual Inst_voices_bass* clone() {
		return new Inst_voices_bass();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Bass");
		ui_interface->addHorizontalSlider("freq", &fHslider0, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("pressure", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("slide", &fHslider1, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider0);
		float fSlow1 = fConst1 * fSlow0;
		float fSlow2 = fConst3 * static_cast<float>(fHslider1);
		float fSlow3 = ((fSlow0 <= 82.41f) ? 0.0f : ((fSlow0 >= 523.25f) ? 1.0f : 0.0022683968f * (fSlow0 + -82.41f)));
		int iSlow4 = fSlow0 >= 2e+02f;
		float fSlow5 = 2.0f * fSlow0 + 3e+01f;
		float fSlow6 = 0.00095238094f * (fSlow0 + -2e+02f);
		float fSlow7 = 22.5f * static_cast<float>(fHslider3) * (0.65f * static_cast<float>(fHslider2) + 0.35f);
		float fSlow8 = 0.8f * (0.0036666666f * (4e+02f - fSlow0) + 3.0f);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec1[0] = 1;
			int iTemp0 = 1 - iVec1[1];
			float fTemp1 = ((iTemp0) ? 0.0f : fSlow1 + fRec2[1]);
			fRec2[0] = fTemp1 - std::floor(fTemp1);
			float fTemp2 = static_cast<float>((fRec2[0] - fRec2[1]) < 0.0f);
			fVec2[0] = fTemp2;
			int iTemp3 = iTemp0 | static_cast<int>(fTemp2);
			fRec3[0] = fSlow2 + fConst2 * fRec3[1];
			float fTemp4 = fRec3[0] + 5.0f;
			int iTemp5 = fTemp4 < 23.0f;
			int iTemp6 = fTemp4 < 24.0f;
			float fTemp7 = fRec3[0] + -18.0f;
			int iTemp8 = fTemp4 < 22.0f;
			float fTemp9 = fRec3[0] + -17.0f;
			int iTemp10 = fTemp4 < 21.0f;
			float fTemp11 = fRec3[0] + -16.0f;
			int iTemp12 = fTemp4 < 2e+01f;
			float fTemp13 = fRec3[0] + -15.0f;
			int iTemp14 = fTemp4 < 19.0f;
			float fTemp15 = fRec3[0] + -14.0f;
			int iTemp16 = fTemp4 < 18.0f;
			int iTemp17 = fTemp4 < 17.0f;
			int iTemp18 = fTemp4 < 16.0f;
			int iTemp19 = fTemp4 < 15.0f;
			int iTemp20 = fTemp4 < 14.0f;
			float fTemp21 = fRec3[0] + -9.0f;
			int iTemp22 = fTemp4 < 13.0f;
			float fTemp23 = fRec3[0] + -8.0f;
			int iTemp24 = fTemp4 < 12.0f;
			float fTemp25 = fRec3[0] + -7.0f;
			int iTemp26 = fTemp4 < 11.0f;
			float fTemp27 = fRec3[0] + -6.0f;
			int iTemp28 = fTemp4 < 1e+01f;
			float fTemp29 = fRec3[0] + -5.0f;
			float fTemp30 = 5e+01f * fTemp29;
			int iTemp31 = fTemp4 < 9.0f;
			float fTemp32 = fRec3[0] + -4.0f;
			int iTemp33 = fTemp4 < 8.0f;
			float fTemp34 = fRec3[0] + -3.0f;
			float fTemp35 = 5e+01f * fTemp34;
			int iTemp36 = fTemp4 < 7.0f;
			float fTemp37 = fRec3[0] + -2.0f;
			int iTemp38 = fTemp4 < 6.0f;
			float fTemp39 = fRec3[0] + -1.0f;
			int iTemp40 = fTemp4 < 5.0f;
			float fTemp41 = 3.5e+02f * fRec3[0];
			int iTemp42 = fTemp4 < 4.0f;
			float fTemp43 = fRec3[0] + 1.0f;
			float fTemp44 = ((iTemp3) ? 0.0f : fRec1[1] + fConst1 * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? 4.95e+03f : ((iTemp40) ? 4.95e+03f - 2.2e+03f * fTemp43 : 2.75e+03f)) : ((iTemp38) ? fTemp41 + 2.75e+03f : 3.1e+03f)) : ((iTemp36) ? 2.4e+02f * fTemp39 + 3.1e+03f : 3.34e+03f)) : ((iTemp33) ? 3.34e+03f - 4.4e+02f * fTemp37 : 2.9e+03f)) : ((iTemp31) ? fTemp35 + 2.9e+03f : 2.95e+03f)) : ((iTemp28) ? 4e+02f * fTemp32 + 2.95e+03f : 3.35e+03f)) : ((iTemp26) ? 3.35e+03f - fTemp30 : 3.3e+03f)) : ((iTemp24) ? 2.9e+02f * fTemp27 + 3.3e+03f : 3.59e+03f)) : ((iTemp22) ? 3.59e+03f - 2.9e+02f * fTemp25 : 3.3e+03f)) : ((iTemp20) ? 1e+02f * fTemp23 + 3.3e+03f : 3.4e+03f)) : ((iTemp19) ? 1.55e+03f * fTemp21 + 3.4e+03f : 4.95e+03f)) : 4.95e+03f) : 4.95e+03f) : 4.95e+03f) : 4.95e+03f) : ((iTemp12) ? 4.95e+03f - 1.7e+03f * fTemp15 : 3.25e+03f)) : ((iTemp10) ? 3.3e+02f * fTemp13 + 3.25e+03f : 3.58e+03f)) : ((iTemp8) ? 3.58e+03f - 4e+01f * fTemp11 : 3.54e+03f)) : ((iTemp5) ? 3.54e+03f - 5.4e+02f * fTemp9 : 3e+03f)) : ((iTemp6) ? 3e+02f * fTemp7 + 3e+03f : 3.3e+03f)));
			fRec1[0] = fTemp44 - std::floor(fTemp44);
			float fTemp45 = 2e+01f * fTemp13;
			float fTemp46 = fRec3[0] + -13.0f;
			float fTemp47 = fRec3[0] + -11.0f;
			float fTemp48 = 8e+01f * fTemp47;
			float fTemp49 = fRec3[0] + -1e+01f;
			float fTemp50 = 2e+01f * fTemp21;
			float fTemp51 = 2e+01f * fTemp29;
			float fTemp52 = 2e+01f * fTemp32;
			float fTemp53 = 1e+01f * fRec3[0];
			int iTemp54 = fTemp4 < 3.0f;
			float fTemp55 = fRec3[0] + 2.0f;
			int iTemp56 = fTemp4 < 2.0f;
			float fTemp57 = fRec3[0] + 3.0f;
			int iTemp58 = fTemp4 < 1.0f;
			int iTemp59 = fTemp4 < 0.0f;
			float fTemp60 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 1.4e+02f : ((iTemp58) ? 6e+01f * fTemp4 + 1.4e+02f : 2e+02f)) : 2e+02f) : ((iTemp54) ? 2e+02f - 65.0f * fTemp57 : 135.0f)) : ((iTemp42) ? 65.0f * fTemp55 + 135.0f : 2e+02f)) : ((iTemp40) ? 2e+02f - 7e+01f * fTemp43 : 1.3e+02f)) : ((iTemp38) ? 1.3e+02f - fTemp53 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp28) ? fTemp52 + 1.2e+02f : 1.4e+02f)) : ((iTemp26) ? 1.4e+02f - fTemp51 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp19) ? fTemp50 + 1.2e+02f : 1.4e+02f)) : ((iTemp18) ? 6e+01f * fTemp49 + 1.4e+02f : 2e+02f)) : ((iTemp17) ? 2e+02f - fTemp48 : 1.2e+02f)) : 1.2e+02f) : ((iTemp14) ? 8e+01f * fTemp46 + 1.2e+02f : 2e+02f)) : ((iTemp12) ? 2e+02f - 6e+01f * fTemp15 : 1.4e+02f)) : ((iTemp10) ? 1.4e+02f - fTemp45 : 1.2e+02f)) : 1.2e+02f) : ((iTemp5) ? 15.0f * fTemp9 + 1.2e+02f : 135.0f)) : ((iTemp6) ? 135.0f - 15.0f * fTemp7 : 1.2e+02f));
			fRec4[0] = fConst5 * fTemp60 + fConst4 * fRec4[1];
			float fTemp61 = std::exp(-(fConst6 * fRec4[0]));
			int iTemp62 = fRec3[0] >= 5.0f;
			int iTemp63 = fRec3[0] >= 3.0f;
			int iTemp64 = fRec3[0] >= 2.0f;
			int iTemp65 = fRec3[0] >= 1.0f;
			int iTemp66 = fRec3[0] >= 4.0f;
			int iTemp67 = fRec3[0] >= 8.0f;
			int iTemp68 = fRec3[0] >= 7.0f;
			int iTemp69 = fRec3[0] >= 6.0f;
			float fTemp70 = ((iTemp62) ? ((iTemp67) ? 2.0f : ((iTemp68) ? 3.0f : ((iTemp69) ? 3.0f : 2.0f))) : ((iTemp63) ? ((iTemp66) ? 1.5f : 1.0f) : ((iTemp64) ? 1.25f : ((iTemp65) ? 1.25f : 1.0f))));
			float fTemp71 = fTemp70 + fSlow3 * (((iTemp62) ? ((iTemp67) ? 12.0f : ((iTemp68) ? 12.0f : ((iTemp69) ? 12.0f : 15.0f))) : ((iTemp63) ? ((iTemp66) ? 4.0f : 1e+01f) : ((iTemp64) ? 2.5f : ((iTemp65) ? 2.5f : 1e+01f)))) - fTemp70);
			fRec5[0] = fConst5 * fTemp71 * fTemp60 + fConst4 * fRec5[1];
			float fTemp72 = std::exp(-(fConst6 * fRec5[0]));
			float fTemp73 = fRec3[0] + -12.0f;
			float fTemp74 = ((iTemp24) ? 0.1f - 0.084151f * fTemp27 : 0.015849f);
			fRec6[0] = fVec2[1] + fRec6[1] * (fTemp72 + fTemp61) - fTemp72 * fTemp61 * fRec6[2];
			float fTemp75 = 1e+02f * fTemp7;
			float fTemp76 = fRec3[0] + 4.0f;
			float fTemp77 = ((iTemp3) ? 0.0f : fRec7[1] + fConst1 * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 3.5e+03f : ((iTemp58) ? 3.5e+03f - 2e+02f * fTemp4 : 3.3e+03f)) : ((iTemp56) ? 4e+02f * fTemp76 + 3.3e+03f : 3.7e+03f)) : ((iTemp54) ? 3.7e+03f - 2e+02f * fTemp57 : 3.5e+03f)) : 3.5e+03f) : ((iTemp40) ? 3.5e+03f - 1.05e+03f * fTemp43 : 2.45e+03f)) : ((iTemp38) ? fTemp41 + 2.45e+03f : 2.8e+03f)) : ((iTemp36) ? 2.5e+02f * fTemp39 + 2.8e+03f : 3.05e+03f)) : ((iTemp33) ? 3.05e+03f - 4.5e+02f * fTemp37 : 2.6e+03f)) : ((iTemp31) ? 75.0f * fTemp34 + 2.6e+03f : 2675.0f)) : ((iTemp28) ? 325.0f * fTemp32 + 2675.0f : 3e+03f)) : 3e+03f) : ((iTemp24) ? 3.5e+02f * fTemp27 + 3e+03f : 3.35e+03f)) : ((iTemp22) ? 3.35e+03f - 3.5e+02f * fTemp25 : 3e+03f)) : 3e+03f) : ((iTemp19) ? 9e+02f * fTemp21 + 3e+03f : 3.9e+03f)) : ((iTemp18) ? 3.9e+03f - 3e+02f * fTemp49 : 3.6e+03f)) : ((iTemp17) ? 3e+02f * fTemp47 + 3.6e+03f : 3.9e+03f)) : ((iTemp16) ? 3.9e+03f - 1e+02f * fTemp73 : 3.8e+03f)) : 3.8e+03f) : ((iTemp12) ? 3.8e+03f - 9e+02f * fTemp15 : 2.9e+03f)) : ((iTemp10) ? 3e+02f * fTemp13 + 2.9e+03f : 3.2e+03f)) : ((iTemp8) ? 5e+01f * fTemp11 + 3.2e+03f : 3.25e+03f)) : ((iTemp5) ? 3.25e+03f - 4.5e+02f * fTemp9 : 2.8e+03f)) : ((iTemp6) ? fTemp75 + 2.8e+03f : 2.9e+03f)));
			fRec7[0] = fTemp77 - std::floor(fTemp77);
			float fTemp78 = 1e+01f * fTemp9;
			float fTemp79 = 1e+01f * fTemp13;
			float fTemp80 = 5e+01f * fTemp15;
			float fTemp81 = 2e+01f * fTemp49;
			float fTemp82 = 1e+01f * fTemp29;
			float fTemp83 = 1e+01f * fTemp32;
			float fTemp84 = 6e+01f * fTemp43;
			float fTemp85 = 2e+01f * fTemp57;
			float fTemp86 = 2e+01f * fTemp4;
			float fTemp87 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 1.3e+02f : ((iTemp58) ? fTemp86 + 1.3e+02f : 1.5e+02f)) : 1.5e+02f) : ((iTemp54) ? 1.5e+02f - fTemp85 : 1.3e+02f)) : ((iTemp42) ? 5e+01f * fTemp55 + 1.3e+02f : 1.8e+02f)) : ((iTemp40) ? 1.8e+02f - fTemp84 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp28) ? fTemp83 + 1.2e+02f : 1.3e+02f)) : ((iTemp26) ? 1.3e+02f - fTemp82 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp19) ? 1e+01f * fTemp21 + 1.2e+02f : 1.3e+02f)) : ((iTemp18) ? fTemp81 + 1.3e+02f : 1.5e+02f)) : ((iTemp17) ? 1.5e+02f - 3e+01f * fTemp47 : 1.2e+02f)) : 1.2e+02f) : ((iTemp14) ? 6e+01f * fTemp46 + 1.2e+02f : 1.8e+02f)) : ((iTemp12) ? 1.8e+02f - fTemp80 : 1.3e+02f)) : ((iTemp10) ? 1.3e+02f - fTemp79 : 1.2e+02f)) : 1.2e+02f) : ((iTemp5) ? fTemp78 + 1.2e+02f : 1.3e+02f)) : ((iTemp6) ? 1.3e+02f - 1e+01f * fTemp7 : 1.2e+02f));
			fRec8[0] = fConst5 * fTemp87 + fConst4 * fRec8[1];
			float fTemp88 = std::exp(-(fConst6 * fRec8[0]));
			fRec9[0] = fConst5 * fTemp71 * fTemp87 + fConst4 * fRec9[1];
			float fTemp89 = std::exp(-(fConst6 * fRec9[0]));
			float fTemp90 = ((iTemp16) ? 0.029314f * fTemp73 + 0.050119f : 0.079433f);
			fRec10[0] = fVec2[1] + fRec10[1] * (fTemp89 + fTemp88) - fTemp89 * fTemp88 * fRec10[2];
			float fTemp91 = ((iTemp3) ? 0.0f : fRec11[1] + fConst1 * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 2.8e+03f : ((iTemp58) ? 2.8e+03f - 1e+02f * fTemp4 : 2.7e+03f)) : 2.7e+03f) : ((iTemp54) ? 1.3e+02f * fTemp57 + 2.7e+03f : 2.83e+03f)) : ((iTemp42) ? 2.83e+03f - 3e+02f * fTemp55 : 2.53e+03f)) : ((iTemp40) ? 2.53e+03f - 2.8e+02f * fTemp43 : 2.25e+03f)) : ((iTemp38) ? 1.5e+02f * fRec3[0] + 2.25e+03f : 2.4e+03f)) : ((iTemp36) ? 2e+02f * fTemp39 + 2.4e+03f : 2.6e+03f)) : ((iTemp33) ? 2.6e+03f - 2e+02f * fTemp37 : 2.4e+03f)) : 2.4e+03f) : ((iTemp28) ? 3.5e+02f * fTemp32 + 2.4e+03f : 2.75e+03f)) : ((iTemp26) ? 2.75e+03f - fTemp30 : 2.7e+03f)) : ((iTemp24) ? 2e+02f * fTemp27 + 2.7e+03f : 2.9e+03f)) : ((iTemp22) ? 2.9e+03f - 2e+02f * fTemp25 : 2.7e+03f)) : ((iTemp20) ? 5e+01f * fTemp23 + 2.7e+03f : 2.75e+03f)) : ((iTemp19) ? 1.5e+02f * fTemp21 + 2.75e+03f : 2.9e+03f)) : ((iTemp18) ? 2.9e+03f - 1e+02f * fTemp49 : 2.8e+03f)) : ((iTemp17) ? 1.5e+02f * fTemp47 + 2.8e+03f : 2.95e+03f)) : ((iTemp16) ? 2.95e+03f - 1.2e+02f * fTemp73 : 2.83e+03f)) : ((iTemp14) ? 2.83e+03f - 1.3e+02f * fTemp46 : 2.7e+03f)) : ((iTemp12) ? 2.7e+03f - fTemp80 : 2.65e+03f)) : ((iTemp10) ? 2.65e+03f - 5e+01f * fTemp13 : 2.6e+03f)) : ((iTemp8) ? 2e+02f * fTemp11 + 2.6e+03f : 2.8e+03f)) : ((iTemp5) ? 2.8e+03f - 2e+02f * fTemp9 : 2.6e+03f)) : ((iTemp6) ? fTemp75 + 2.6e+03f : 2.7e+03f)));
			fRec11[0] = fTemp91 - std::floor(fTemp91);
			float fTemp92 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? 1.2e+02f : ((iTemp54) ? 1.2e+02f - fTemp85 : 1e+02f)) : ((iTemp42) ? 7e+01f * fTemp55 + 1e+02f : 1.7e+02f)) : ((iTemp40) ? 1.7e+02f - fTemp84 : 1.1e+02f)) : ((iTemp38) ? 1.1e+02f - fTemp53 : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f) : ((iTemp28) ? fTemp52 + 1e+02f : 1.2e+02f)) : ((iTemp26) ? 1.2e+02f - fTemp51 : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f) : ((iTemp19) ? fTemp50 + 1e+02f : 1.2e+02f)) : 1.2e+02f) : ((iTemp17) ? 1.2e+02f - 2e+01f * fTemp47 : 1e+02f)) : 1e+02f) : ((iTemp14) ? 7e+01f * fTemp46 + 1e+02f : 1.7e+02f)) : ((iTemp12) ? 1.7e+02f - fTemp80 : 1.2e+02f)) : ((iTemp10) ? 1.2e+02f - fTemp45 : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f);
			fRec12[0] = fConst5 * fTemp92 + fConst4 * fRec12[1];
			float fTemp93 = std::exp(-(fConst6 * fRec12[0]));
			fRec13[0] = fConst5 * fTemp71 * fTemp92 + fConst4 * fRec13[1];
			float fTemp94 = std::exp(-(fConst6 * fRec13[0]));
			fRec14[0] = fVec2[1] + fRec14[1] * (fTemp94 + fTemp93) - fTemp94 * fTemp93 * fRec14[2];
			float fTemp95 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 1.15e+03f : ((iTemp58) ? 4.5e+02f * fTemp4 + 1.15e+03f : 1.6e+03f)) : ((iTemp56) ? 1e+02f * fTemp76 + 1.6e+03f : 1.7e+03f)) : ((iTemp54) ? 1.7e+03f - 9e+02f * fTemp57 : 8e+02f)) : ((iTemp42) ? 8e+02f - 1e+02f * fTemp55 : 7e+02f)) : ((iTemp40) ? 3.4e+02f * fTemp43 + 7e+02f : 1.04e+03f)) : ((iTemp38) ? 5.8e+02f * fRec3[0] + 1.04e+03f : 1.62e+03f)) : ((iTemp36) ? 1.3e+02f * fTemp39 + 1.62e+03f : 1.75e+03f)) : ((iTemp33) ? 1.75e+03f - 1e+03f * fTemp37 : 7.5e+02f)) : ((iTemp31) ? 7.5e+02f - 1.5e+02f * fTemp34 : 6e+02f)) : ((iTemp28) ? 5.2e+02f * fTemp32 + 6e+02f : 1.12e+03f)) : ((iTemp26) ? 6.8e+02f * fTemp29 + 1.12e+03f : 1.8e+03f)) : ((iTemp24) ? 5e+01f * fTemp27 + 1.8e+03f : 1.85e+03f)) : ((iTemp22) ? 1.85e+03f - 1.03e+03f * fTemp25 : 8.2e+02f)) : ((iTemp20) ? 8.2e+02f - 1.9e+02f * fTemp23 : 6.3e+02f)) : ((iTemp19) ? 5.2e+02f * fTemp21 + 6.3e+02f : 1.15e+03f)) : ((iTemp18) ? 8.5e+02f * fTemp49 + 1.15e+03f : 2e+03f)) : ((iTemp17) ? 1.4e+02f * fTemp47 + 2e+03f : 2.14e+03f)) : ((iTemp16) ? 2.14e+03f - 1.34e+03f * fTemp73 : 8e+02f)) : ((iTemp14) ? 8e+02f - 1e+02f * fTemp46 : 7e+02f)) : ((iTemp12) ? 3.8e+02f * fTemp15 + 7e+02f : 1.08e+03f)) : ((iTemp10) ? 6.2e+02f * fTemp13 + 1.08e+03f : 1.7e+03f)) : ((iTemp8) ? 1.7e+02f * fTemp11 + 1.7e+03f : 1.87e+03f)) : ((iTemp5) ? 1.87e+03f - 1.07e+03f * fTemp9 : 8e+02f)) : ((iTemp6) ? 8e+02f - 2e+02f * fTemp7 : 6e+02f));
			float fTemp96 = ((iTemp3) ? 0.0f : fRec15[1] + fConst1 * (((fTemp95 >= 1.3e+03f) & iSlow4) ? fTemp95 - fSlow6 * (fTemp95 + -1.3e+03f) : ((fTemp95 <= fSlow5) ? fSlow5 : fTemp95)));
			fRec15[0] = fTemp96 - std::floor(fTemp96);
			float fTemp97 = 1e+01f * fTemp43;
			float fTemp98 = 2e+01f * fTemp55;
			float fTemp99 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 9e+01f : ((iTemp58) ? 9e+01f - 1e+01f * fTemp4 : 8e+01f)) : ((iTemp56) ? 2e+01f * fTemp76 + 8e+01f : 1e+02f)) : ((iTemp54) ? 1e+02f - fTemp85 : 8e+01f)) : ((iTemp42) ? 8e+01f - fTemp98 : 6e+01f)) : ((iTemp40) ? fTemp97 + 6e+01f : 7e+01f)) : ((iTemp38) ? fTemp53 + 7e+01f : 8e+01f)) : ((iTemp36) ? 1e+01f * fTemp39 + 8e+01f : 9e+01f)) : ((iTemp33) ? 9e+01f - 1e+01f * fTemp37 : 8e+01f)) : 8e+01f) : ((iTemp28) ? fTemp83 + 8e+01f : 9e+01f)) : ((iTemp26) ? 9e+01f - fTemp82 : 8e+01f)) : ((iTemp24) ? 1e+01f * fTemp27 + 8e+01f : 9e+01f)) : ((iTemp22) ? 9e+01f - 1e+01f * fTemp25 : 8e+01f)) : ((iTemp20) ? 8e+01f - 2e+01f * fTemp23 : 6e+01f)) : ((iTemp19) ? 3e+01f * fTemp21 + 6e+01f : 9e+01f)) : ((iTemp18) ? 1e+01f * fTemp49 + 9e+01f : 1e+02f)) : ((iTemp17) ? 1e+02f - 1e+01f * fTemp47 : 9e+01f)) : ((iTemp16) ? 9e+01f - 1e+01f * fTemp73 : 8e+01f)) : ((iTemp14) ? 8e+01f - 2e+01f * fTemp46 : 6e+01f)) : ((iTemp12) ? 3e+01f * fTemp15 + 6e+01f : 9e+01f)) : ((iTemp10) ? 9e+01f - fTemp79 : 8e+01f)) : ((iTemp8) ? 1e+01f * fTemp11 + 8e+01f : 9e+01f)) : ((iTemp5) ? 9e+01f - fTemp78 : 8e+01f)) : ((iTemp6) ? 8e+01f - 2e+01f * fTemp7 : 6e+01f));
			fRec16[0] = fConst5 * fTemp99 + fConst4 * fRec16[1];
			float fTemp100 = std::exp(-(fConst6 * fRec16[0]));
			fRec17[0] = fConst5 * fTemp71 * fTemp99 + fConst4 * fRec17[1];
			float fTemp101 = std::exp(-(fConst6 * fRec17[0]));
			fRec18[0] = fVec2[1] + fRec18[1] * (fTemp101 + fTemp100) - fTemp101 * fTemp100 * fRec18[2];
			float fTemp102 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 8e+02f : ((iTemp58) ? 8e+02f - 4e+02f * fTemp4 : 4e+02f)) : ((iTemp56) ? 4e+02f - 5e+01f * fTemp76 : 3.5e+02f)) : ((iTemp54) ? 1e+02f * fTemp57 + 3.5e+02f : 4.5e+02f)) : ((iTemp42) ? 4.5e+02f - 125.0f * fTemp55 : 325.0f)) : ((iTemp40) ? 275.0f * fTemp43 + 325.0f : 6e+02f)) : ((iTemp38) ? 6e+02f - 2e+02f * fRec3[0] : 4e+02f)) : ((iTemp36) ? 4e+02f - 1.5e+02f * fTemp39 : 2.5e+02f)) : ((iTemp33) ? 1.5e+02f * fTemp37 + 2.5e+02f : 4e+02f)) : ((iTemp31) ? 4e+02f - fTemp35 : 3.5e+02f)) : ((iTemp28) ? 3.1e+02f * fTemp32 + 3.5e+02f : 6.6e+02f)) : ((iTemp26) ? 6.6e+02f - 2.2e+02f * fTemp29 : 4.4e+02f)) : ((iTemp24) ? 4.4e+02f - 1.7e+02f * fTemp27 : 2.7e+02f)) : ((iTemp22) ? 1.6e+02f * fTemp25 + 2.7e+02f : 4.3e+02f)) : ((iTemp20) ? 4.3e+02f - 6e+01f * fTemp23 : 3.7e+02f)) : ((iTemp19) ? 4.3e+02f * fTemp21 + 3.7e+02f : 8e+02f)) : ((iTemp18) ? 8e+02f - 4.5e+02f * fTemp49 : 3.5e+02f)) : ((iTemp17) ? 3.5e+02f - fTemp48 : 2.7e+02f)) : ((iTemp16) ? 1.8e+02f * fTemp73 + 2.7e+02f : 4.5e+02f)) : ((iTemp14) ? 4.5e+02f - 125.0f * fTemp46 : 325.0f)) : ((iTemp12) ? 325.0f * (fTemp15 + 1.0f) : 6.5e+02f)) : ((iTemp10) ? 6.5e+02f - 2.5e+02f * fTemp13 : 4e+02f)) : ((iTemp8) ? 4e+02f - 1.1e+02f * fTemp11 : 2.9e+02f)) : ((iTemp5) ? 1.1e+02f * fTemp9 + 2.9e+02f : 4e+02f)) : ((iTemp6) ? 4e+02f - 5e+01f * fTemp7 : 3.5e+02f));
			float fTemp103 = ((iTemp3) ? 0.0f : fRec19[1] + fConst1 * ((fTemp102 <= fSlow0) ? fSlow0 : fTemp102));
			fRec19[0] = fTemp103 - std::floor(fTemp103);
			float fTemp104 = ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 8e+01f : ((iTemp58) ? 8e+01f - fTemp86 : 6e+01f)) : ((iTemp56) ? 6e+01f - 1e+01f * fTemp76 : 5e+01f)) : ((iTemp54) ? fTemp85 + 5e+01f : 7e+01f)) : ((iTemp42) ? 7e+01f - fTemp98 : 5e+01f)) : ((iTemp40) ? fTemp97 + 5e+01f : 6e+01f)) : ((iTemp38) ? 6e+01f - 2e+01f * fRec3[0] : 4e+01f)) : ((iTemp36) ? 2e+01f * fTemp39 + 4e+01f : 6e+01f)) : ((iTemp33) ? 6e+01f - 2e+01f * fTemp37 : 4e+01f)) : 4e+01f) : ((iTemp28) ? 4e+01f * (fTemp32 + 1.0f) : 8e+01f)) : ((iTemp26) ? 8e+01f - fTemp82 : 7e+01f)) : ((iTemp24) ? 7e+01f - 3e+01f * fTemp27 : 4e+01f)) : 4e+01f) : 4e+01f) : ((iTemp19) ? 4e+01f * (fTemp21 + 1.0f) : 8e+01f)) : ((iTemp18) ? 8e+01f - fTemp81 : 6e+01f)) : 6e+01f) : ((iTemp16) ? 6e+01f - 2e+01f * fTemp73 : 4e+01f)) : ((iTemp14) ? 1e+01f * fTemp46 + 4e+01f : 5e+01f)) : 5e+01f) : ((iTemp10) ? fTemp45 + 5e+01f : 7e+01f)) : ((iTemp8) ? 7e+01f - 3e+01f * fTemp11 : 4e+01f)) : ((iTemp5) ? 3e+01f * fTemp9 + 4e+01f : 7e+01f)) : ((iTemp6) ? 7e+01f - 3e+01f * fTemp7 : 4e+01f));
			fRec20[0] = fConst5 * fTemp104 + fConst4 * fRec20[1];
			float fTemp105 = std::exp(-(fConst6 * fRec20[0]));
			fRec21[0] = fConst5 * fTemp71 * fTemp104 + fConst4 * fRec21[1];
			float fTemp106 = std::exp(-(fConst6 * fRec21[0]));
			fRec22[0] = fVec2[1] + fRec22[1] * (fTemp106 + fTemp105) - fTemp106 * fTemp105 * fRec22[2];
			float fTemp107 = ((fSlow7 > fRec23[1]) ? fConst8 : fConst7);
			fRec23[0] = fSlow7 * (1.0f - fTemp107) + fRec23[1] * fTemp107;
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow8 * fRec23[0] * (fRec22[0] * (1.0f - (fTemp105 + fTemp106 * (1.0f - fTemp105))) * ftbl0Inst_voices_bassSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec19[0]), 65535))] + fRec18[0] * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 0.630957f : ((iTemp58) ? 0.630957f - 0.567861f * fTemp4 : 0.063096f)) : ((iTemp56) ? 0.036904f * fTemp76 + 0.063096f : 0.1f)) : ((iTemp54) ? 0.254813f * fTemp57 + 0.1f : 0.354813f)) : ((iTemp42) ? 0.354813f - 0.103624f * fTemp55 : 0.251189f)) : ((iTemp40) ? 0.195495f * fTemp43 + 0.251189f : 0.446684f)) : ((iTemp38) ? 0.446684f - 0.195495f * fRec3[0] : 0.251189f)) : ((iTemp36) ? 0.251189f - 0.219566f * fTemp39 : 0.031623f)) : ((iTemp33) ? 0.250215f * fTemp37 + 0.031623f : 0.281838f)) : ((iTemp31) ? 0.281838f - 0.181838f * fTemp34 : 0.1f)) : ((iTemp28) ? 0.401187f * fTemp32 + 0.1f : 0.501187f)) : ((iTemp26) ? 0.501187f - 0.301661f * fTemp29 : 0.199526f)) : ((iTemp24) ? 0.199526f - 0.13643f * fTemp27 : 0.063096f)) : ((iTemp22) ? 0.253132f * fTemp25 + 0.063096f : 0.316228f)) : ((iTemp20) ? 0.316228f - 0.216228f * fTemp23 : 0.1f)) : ((iTemp19) ? 0.401187f * fTemp21 + 0.1f : 0.501187f)) : ((iTemp18) ? 0.501187f - 0.401187f * fTemp49 : 0.1f)) : ((iTemp17) ? 0.151189f * fTemp47 + 0.1f : 0.251189f)) : ((iTemp16) ? 0.030649f * fTemp73 + 0.251189f : 0.281838f)) : ((iTemp14) ? 0.281838f - 0.123349f * fTemp46 : 0.158489f)) : ((iTemp12) ? 0.342698f * fTemp15 + 0.158489f : 0.501187f)) : ((iTemp10) ? 0.501187f - 0.301661f * fTemp13 : 0.199526f)) : ((iTemp8) ? 0.199526f - 0.021698f * fTemp11 : 0.177828f)) : ((iTemp5) ? 0.1384f * fTemp9 + 0.177828f : 0.316228f)) : ((iTemp6) ? 0.316228f - 0.216228f * fTemp7 : 0.1f)) * (1.0f - (fTemp100 + fTemp101 * (1.0f - fTemp100))) * ftbl0Inst_voices_bassSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec15[0]), 65535))] + fRec14[0] * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 0.1f : ((iTemp58) ? 0.1f - 0.068377f * fTemp4 : 0.031623f)) : 0.031623f) : ((iTemp54) ? 0.126866f * fTemp57 + 0.031623f : 0.158489f)) : ((iTemp42) ? 0.158489f - 0.126866f * fTemp55 : 0.031623f)) : ((iTemp40) ? 0.32319f * fTemp43 + 0.031623f : 0.354813f)) : 0.354813f) : ((iTemp36) ? 0.354813f - 0.196324f * fTemp39 : 0.158489f)) : ((iTemp33) ? 0.158489f - 0.069364f * fTemp37 : 0.089125f)) : ((iTemp31) ? 0.089125f - 0.064006f * fTemp34 : 0.025119f)) : ((iTemp28) ? 0.045676f * fTemp32 + 0.025119f : 0.070795f)) : ((iTemp26) ? 0.055098f * fTemp29 + 0.070795f : 0.125893f)) : ((iTemp24) ? 0.125893f - 0.062797f * fTemp27 : 0.063096f)) : ((iTemp22) ? 0.063096f - 0.012977f * fTemp25 : 0.050119f)) : ((iTemp20) ? 0.020676f * fTemp23 + 0.050119f : 0.070795f)) : ((iTemp19) ? 0.070795f - 0.045676f * fTemp21 : 0.025119f)) : ((iTemp18) ? 0.152709f * fTemp49 + 0.025119f : 0.177828f)) : ((iTemp17) ? 0.177828f - 0.127709f * fTemp47 : 0.050119f)) : fTemp90) : ((iTemp14) ? 0.079433f - 0.06165f * fTemp46 : 0.017783f)) : ((iTemp12) ? 0.428901f * fTemp15 + 0.017783f : 0.446684f)) : ((iTemp10) ? 0.446684f - 0.195495f * fTemp13 : 0.251189f)) : ((iTemp8) ? 0.251189f - 0.125296f * fTemp11 : 0.125893f)) : ((iTemp5) ? 0.125296f * fTemp9 + 0.125893f : 0.251189f)) : ((iTemp6) ? 0.251189f - 0.109935f * fTemp7 : 0.141254f)) * (1.0f - (fTemp93 + fTemp94 * (1.0f - fTemp93))) * ftbl0Inst_voices_bassSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec11[0]), 65535))] + fRec10[0] * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? ((iTemp58) ? ((iTemp59) ? 0.015849f : ((iTemp58) ? 0.001934f * fTemp4 + 0.015849f : 0.017783f)) : ((iTemp56) ? 0.017783f - 0.001934f * fTemp76 : 0.015849f)) : ((iTemp54) ? 0.023962f * fTemp57 + 0.015849f : 0.039811f)) : ((iTemp42) ? 0.039811f - 0.029811f * fTemp55 : 0.01f)) : ((iTemp40) ? 0.344813f * fTemp43 + 0.01f : 0.354813f)) : ((iTemp38) ? 0.354813f - 0.103624f * fRec3[0] : 0.251189f)) : ((iTemp36) ? 0.251189f - 0.171756f * fTemp39 : 0.079433f)) : ((iTemp33) ? 0.020567f * fTemp37 + 0.079433f : 0.1f)) : ((iTemp31) ? 0.1f - 0.060189f * fTemp34 : 0.039811f)) : ((iTemp28) ? 0.023285f * fTemp32 + 0.039811f : 0.063096f)) : ((iTemp26) ? 0.036904f * fTemp29 + 0.063096f : 0.1f)) : fTemp74) : ((iTemp22) ? 0.063584f * fTemp25 + 0.015849f : 0.079433f)) : ((iTemp20) ? 0.079433f - 0.04781f * fTemp23 : 0.031623f)) : ((iTemp19) ? 0.068377f * fTemp21 + 0.031623f : 0.1f)) : ((iTemp18) ? 0.1f - 0.09f * fTemp49 : 0.01f)) : ((iTemp17) ? 0.040119f * fTemp47 + 0.01f : 0.050119f)) : fTemp90) : ((iTemp14) ? 0.079433f - 0.069433f * fTemp46 : 0.01f)) : ((iTemp12) ? 0.388107f * fTemp15 + 0.01f : 0.398107f)) : ((iTemp10) ? 0.398107f - 0.198581f * fTemp13 : 0.199526f)) : ((iTemp8) ? 0.199526f - 0.099526f * fTemp11 : 0.1f)) : ((iTemp5) ? 0.151189f * fTemp9 + 0.1f : 0.251189f)) : ((iTemp6) ? 0.251189f - 0.051663f * fTemp7 : 0.199526f)) * (1.0f - (fTemp88 + fTemp89 * (1.0f - fTemp88))) * ftbl0Inst_voices_bassSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec7[0]), 65535))] + fRec6[0] * ((iTemp5) ? ((iTemp8) ? ((iTemp10) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp17) ? ((iTemp18) ? ((iTemp19) ? ((iTemp20) ? ((iTemp22) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp31) ? ((iTemp33) ? ((iTemp36) ? ((iTemp38) ? ((iTemp40) ? ((iTemp42) ? ((iTemp54) ? ((iTemp56) ? 0.001f : ((iTemp54) ? 0.000778f * fTemp57 + 0.001f : 0.001778f)) : ((iTemp42) ? 0.001778f - 0.001147f * fTemp55 : 0.000631f)) : ((iTemp40) ? 0.099369f * fTemp43 + 0.000631f : 0.1f)) : ((iTemp38) ? 0.025893f * fRec3[0] + 0.1f : 0.125893f)) : ((iTemp36) ? 0.125893f - 0.086082f * fTemp39 : 0.039811f)) : ((iTemp33) ? 0.039811f - 0.029811f * fTemp37 : 0.01f)) : ((iTemp31) ? 0.005849f * fTemp34 + 0.01f : 0.015849f)) : ((iTemp28) ? 0.015849f - 0.00326f * fTemp32 : 0.012589f)) : ((iTemp26) ? 0.087411f * fTemp29 + 0.012589f : 0.1f)) : fTemp74) : ((iTemp22) ? 0.004104f * fTemp25 + 0.015849f : 0.019953f)) : 0.019953f) : ((iTemp19) ? 0.019953f - 0.016791f * fTemp21 : 0.003162f)) : ((iTemp18) ? 0.003162f - 0.001577f * fTemp49 : 0.001585f)) : ((iTemp17) ? 0.004725f * fTemp47 + 0.001585f : 0.00631f)) : ((iTemp16) ? 0.00631f - 0.003148f * fTemp73 : 0.003162f)) : ((iTemp14) ? 0.003162f - 0.002162f * fTemp46 : 0.001f)) : ((iTemp12) ? 0.078433f * fTemp15 + 0.001f : 0.079433f)) : ((iTemp10) ? 0.020567f * fTemp13 + 0.079433f : 0.1f)) : ((iTemp8) ? 0.1f - 0.068377f * fTemp11 : 0.031623f)) : ((iTemp5) ? 0.018496f * fTemp9 + 0.031623f : 0.050119f)) : 0.050119f) * (1.0f - (fTemp61 + fTemp72 * (1.0f - fTemp61))) * ftbl0Inst_voices_bassSIG0[std::max<int>(0, std::min<int>(static_cast<int>(65536.0f * fRec1[0]), 65535))]));
			iVec1[1] = iVec1[0];
			fRec2[1] = fRec2[0];
			fVec2[1] = fVec2[0];
			fRec3[1] = fRec3[0];
			fRec1[1] = fRec1[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec9[1] = fRec9[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
			fRec13[1] = fRec13[0];
			fRec14[2] = fRec14[1];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec16[1] = fRec16[0];
			fRec17[1] = fRec17[0];
			fRec18[2] = fRec18[1];
			fRec18[1] = fRec18[0];
			fRec19[1] = fRec19[0];
			fRec20[1] = fRec20[0];
			fRec21[1] = fRec21[0];
			fRec22[2] = fRec22[1];
			fRec22[1] = fRec22[0];
			fRec23[1] = fRec23[0];
		}
	}

};

#endif
