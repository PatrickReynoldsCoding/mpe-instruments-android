/* ------------------------------------------------------------
name: "Marimba"
Code generated with Faust 2.81.10 (https://faust.grame.fr)
Compilation options: -lang cpp -i -ct 1 -cn Inst_struck_marimba -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __Inst_struck_marimba_H__
#define  __Inst_struck_marimba_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS Inst_struck_marimba
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

const static float fInst_struck_marimbaSIG0Wave0[250] = {1.0f,0.776725f,0.625723f,0.855223f,0.760159f,0.698373f,0.768011f,0.641127f,0.244034f,0.707754f,0.634013f,0.247527f,0.660849f,0.450396f,0.567783f,0.106361f,0.716814f,0.66392f,0.291208f,0.310599f,0.801495f,0.635292f,0.307435f,0.874124f,0.497668f,0.487088f,0.459115f,0.733455f,0.541818f,0.441318f,0.31392f,0.40309f,0.685353f,0.60314f,0.400552f,0.453511f,0.634386f,0.291547f,0.131605f,0.368507f,0.839907f,0.60216f,0.288296f,0.57967f,0.0242493f,0.262746f,0.368588f,0.890284f,0.408963f,0.556072f,0.884427f,0.83211f,0.612015f,0.757176f,0.919477f,1.0f,0.827963f,0.89241f,0.0357408f,0.480789f,0.752872f,0.0546301f,0.235937f,0.362938f,0.444472f,0.101751f,0.703418f,0.453136f,0.316629f,0.490394f,0.982508f,0.551622f,0.602009f,0.666957f,0.77683f,0.905662f,0.0987197f,0.402968f,0.829452f,0.307645f,0.64048f,0.983971f,0.584205f,0.650365f,0.334447f,0.58357f,0.540191f,0.672534f,0.245712f,0.687298f,0.883058f,0.79295f,0.600619f,0.572682f,0.122612f,0.388248f,0.290658f,0.380255f,0.290967f,0.567819f,0.0737721f,0.42099f,0.0786578f,0.393995f,0.268983f,0.260614f,0.494086f,0.238026f,0.0987824f,0.277879f,0.440563f,0.0770212f,0.450591f,0.128137f,0.0368275f,0.128699f,0.329605f,0.374512f,0.36359f,0.272594f,0.379052f,0.305241f,0.0741129f,0.345728f,0.29935f,0.221284f,0.0261391f,0.293202f,0.361885f,0.11433f,0.239005f,0.434156f,0.329583f,0.21946f,0.284175f,0.198555f,0.431976f,0.302985f,1.0f,0.146221f,0.140701f,0.264243f,0.185997f,0.426322f,0.30478f,0.34399f,0.19543f,0.386955f,0.1876f,0.172812f,0.0434115f,0.303761f,0.069454f,0.453943f,0.832451f,0.317817f,0.940601f,1.0f,0.180658f,0.737921f,0.832297f,0.402352f,0.126786f,0.594398f,0.485455f,0.32447f,0.365102f,0.777922f,0.588272f,0.401353f,0.610735f,0.158693f,0.0746072f,0.825099f,0.925459f,0.65377f,0.260792f,0.719384f,0.559908f,0.37259f,0.360035f,0.622939f,0.210271f,0.444595f,0.311286f,0.464309f,0.557231f,0.52408f,0.0701056f,0.320749f,0.19446f,0.727609f,0.522062f,0.394004f,0.235035f,0.395646f,0.494796f,0.517318f,0.109752f,0.692849f,0.00632009f,0.0207583f,0.00306107f,0.0637191f,0.081661f,0.03511f,0.127814f,0.202294f,0.0764145f,0.263127f,0.400199f,0.267278f,0.633385f,1.0f,0.739902f,0.413763f,0.41811f,0.612715f,0.672374f,0.339674f,0.21172f,0.459645f,0.1025f,0.32589f,0.148154f,0.265442f,0.0974305f,0.286438f,0.275213f,0.109111f,0.575089f,0.370283f,0.29411f,0.259826f,0.0648719f,0.583418f,0.282663f,0.182004f,0.117421f,0.417727f,0.16965f,0.24853f,0.122819f,0.185486f,0.0433618f,0.373849f,0.252768f,0.195103f,0.0927835f,0.166543f};
class Inst_struck_marimbaSIG0 {
	
  private:
	
	int fInst_struck_marimbaSIG0Wave0_idx;
	
  public:
	
	int getNumInputsInst_struck_marimbaSIG0() {
		return 0;
	}
	int getNumOutputsInst_struck_marimbaSIG0() {
		return 1;
	}
	
	void instanceInitInst_struck_marimbaSIG0(int sample_rate) {
		fInst_struck_marimbaSIG0Wave0_idx = 0;
	}
	
	void fillInst_struck_marimbaSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			table[i1] = fInst_struck_marimbaSIG0Wave0[fInst_struck_marimbaSIG0Wave0_idx];
			fInst_struck_marimbaSIG0Wave0_idx = (1 + fInst_struck_marimbaSIG0Wave0_idx) % 250;
		}
	}

};

static Inst_struck_marimbaSIG0* newInst_struck_marimbaSIG0() { return (Inst_struck_marimbaSIG0*)new Inst_struck_marimbaSIG0(); }
static void deleteInst_struck_marimbaSIG0(Inst_struck_marimbaSIG0* dsp) { delete dsp; }

static float ftbl0Inst_struck_marimbaSIG0[250];
static float Inst_struck_marimba_faustpower2_f(float value) {
	return value * value;
}

class Inst_struck_marimba : public dsp {
	
 private:
	
	int iRec0[2];
	float fRec4[2];
	int IOTA0;
	float fVec0[2048];
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	float fConst2;
	float fRec2[2];
	float fConst3;
	FAUSTFLOAT fHslider1;
	float fConst4;
	float fConst5;
	float fConst6;
	float fConst7;
	FAUSTFLOAT fHslider2;
	float fVec1[2];
	int iRec6[2];
	float fConst8;
	float fConst9;
	float fConst10;
	float fConst11;
	float fConst12;
	float fConst13;
	float fConst14;
	float fConst15;
	float fConst16;
	float fConst17;
	float fConst18;
	float fConst19;
	float fConst20;
	int iRec9[2];
	float fRec8[3];
	float fConst21;
	float fRec7[3];
	FAUSTFLOAT fHslider3;
	float fRec5[3];
	float fConst22;
	float fConst23;
	float fConst24;
	float fConst25;
	float fRec10[3];
	float fConst26;
	float fConst27;
	float fConst28;
	float fConst29;
	float fRec11[3];
	float fConst30;
	float fConst31;
	float fConst32;
	float fConst33;
	float fRec12[3];
	float fConst34;
	float fConst35;
	float fConst36;
	float fConst37;
	float fRec13[3];
	float fConst38;
	float fConst39;
	float fConst40;
	float fConst41;
	float fRec14[3];
	float fConst42;
	float fConst43;
	float fConst44;
	float fConst45;
	float fRec15[3];
	float fConst46;
	float fConst47;
	float fConst48;
	float fConst49;
	float fRec16[3];
	float fConst50;
	float fConst51;
	float fConst52;
	float fConst53;
	float fRec17[3];
	float fConst54;
	float fConst55;
	float fConst56;
	float fConst57;
	float fRec18[3];
	float fConst58;
	float fConst59;
	float fConst60;
	float fConst61;
	float fRec19[3];
	float fConst62;
	float fConst63;
	float fConst64;
	float fConst65;
	float fRec20[3];
	float fConst66;
	float fConst67;
	float fConst68;
	float fConst69;
	float fRec21[3];
	float fConst70;
	float fConst71;
	float fConst72;
	float fConst73;
	float fRec22[3];
	float fConst74;
	float fConst75;
	float fConst76;
	float fConst77;
	float fRec23[3];
	float fConst78;
	float fConst79;
	float fConst80;
	float fConst81;
	float fRec24[3];
	float fConst82;
	float fConst83;
	float fConst84;
	float fConst85;
	float fRec25[3];
	float fConst86;
	float fConst87;
	float fConst88;
	float fConst89;
	float fRec26[3];
	float fConst90;
	float fConst91;
	float fConst92;
	float fConst93;
	float fRec27[3];
	float fConst94;
	float fConst95;
	float fConst96;
	float fConst97;
	float fRec28[3];
	float fConst98;
	float fConst99;
	float fConst100;
	float fConst101;
	float fRec29[3];
	float fConst102;
	float fConst103;
	float fConst104;
	float fConst105;
	float fRec30[3];
	float fConst106;
	float fConst107;
	float fConst108;
	float fConst109;
	float fRec31[3];
	float fConst110;
	float fConst111;
	float fConst112;
	float fConst113;
	float fRec32[3];
	float fConst114;
	float fConst115;
	float fConst116;
	float fConst117;
	float fRec33[3];
	float fConst118;
	float fConst119;
	float fConst120;
	float fConst121;
	float fRec34[3];
	float fConst122;
	float fConst123;
	float fConst124;
	float fConst125;
	float fRec35[3];
	float fConst126;
	float fConst127;
	float fConst128;
	float fConst129;
	float fRec36[3];
	float fConst130;
	float fConst131;
	float fConst132;
	float fConst133;
	float fRec37[3];
	float fConst134;
	float fConst135;
	float fConst136;
	float fConst137;
	float fRec38[3];
	float fConst138;
	float fConst139;
	float fConst140;
	float fConst141;
	float fRec39[3];
	float fConst142;
	float fConst143;
	float fConst144;
	float fConst145;
	float fRec40[3];
	float fConst146;
	float fConst147;
	float fConst148;
	float fConst149;
	float fRec41[3];
	float fConst150;
	float fConst151;
	float fConst152;
	float fConst153;
	float fRec42[3];
	float fConst154;
	float fConst155;
	float fConst156;
	float fConst157;
	float fRec43[3];
	float fConst158;
	float fConst159;
	float fConst160;
	float fConst161;
	float fRec44[3];
	float fConst162;
	float fConst163;
	float fConst164;
	float fConst165;
	float fRec45[3];
	float fConst166;
	float fConst167;
	float fConst168;
	float fConst169;
	float fRec46[3];
	float fConst170;
	float fConst171;
	float fConst172;
	float fConst173;
	float fRec47[3];
	float fConst174;
	float fConst175;
	float fConst176;
	float fConst177;
	float fRec48[3];
	float fConst178;
	float fConst179;
	float fConst180;
	float fConst181;
	float fRec49[3];
	float fConst182;
	float fConst183;
	float fConst184;
	float fConst185;
	float fRec50[3];
	float fConst186;
	float fConst187;
	float fConst188;
	float fConst189;
	float fRec51[3];
	float fConst190;
	float fConst191;
	float fConst192;
	float fConst193;
	float fRec52[3];
	float fConst194;
	float fConst195;
	float fConst196;
	float fConst197;
	float fRec53[3];
	float fConst198;
	float fConst199;
	float fConst200;
	float fConst201;
	float fRec54[3];
	float fConst202;
	float fConst203;
	float fConst204;
	float fConst205;
	float fRec55[3];
	float fConst206;
	float fConst207;
	float fConst208;
	float fConst209;
	float fRec56[3];
	float fConst210;
	float fConst211;
	float fConst212;
	float fConst213;
	float fRec57[3];
	float fConst214;
	float fConst215;
	float fConst216;
	float fConst217;
	float fRec58[3];
	float fVec2[2];
	float fRec59[2];
	float fVec3[2048];
	float fRec3[2];
	
 public:
	Inst_struck_marimba() {
	}
	
	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -i -ct 1 -cn Inst_struck_marimba -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay4:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.2.0");
		m->declare("description", "Struck wooden bar. Slide changes where the mallet hits the bar.");
		m->declare("envelopes.lib/ar:author", "Yann Orlarey, Stéphane Letz");
		m->declare("envelopes.lib/author", "GRAME");
		m->declare("envelopes.lib/copyright", "GRAME");
		m->declare("envelopes.lib/license", "LGPL with exception");
		m->declare("envelopes.lib/name", "Faust Envelope Library");
		m->declare("envelopes.lib/version", "1.3.0");
		m->declare("filename", "marimba.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
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
		m->declare("name", "Marimba");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.5.0");
		m->declare("physmodels.lib/name", "Faust Physical Models Library");
		m->declare("physmodels.lib/version", "1.2.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("range", "130 1047");
		m->declare("ringout", "4");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "1.6.0");
		m->declare("window", "0.04 0.4");
	}

	virtual int getNumInputs() {
		return 0;
	}
	virtual int getNumOutputs() {
		return 1;
	}
	
	static void classInit(int sample_rate) {
		Inst_struck_marimbaSIG0* sig0 = newInst_struck_marimbaSIG0();
		sig0->instanceInitInst_struck_marimbaSIG0(sample_rate);
		sig0->fillInst_struck_marimbaSIG0(250, ftbl0Inst_struck_marimbaSIG0);
		deleteInst_struck_marimbaSIG0(sig0);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, static_cast<float>(fSampleRate)));
		fConst1 = 0.00882353f * fConst0;
		fConst2 = 0.0014705883f * fConst0;
		fConst3 = 0.5f * fConst0 + -1.0f;
		fConst4 = std::pow(0.001f, 1.1183845e+13f / fConst0);
		fConst5 = Inst_struck_marimba_faustpower2_f(fConst4);
		fConst6 = 1228.3942f / fConst0;
		fConst7 = 2.0f * fConst4;
		fConst8 = 1.0f / std::max<float>(1.0f, 0.001f * fConst0);
		fConst9 = std::tan(20420.352f / fConst0);
		fConst10 = 2.0f * (1.0f - 1.0f / Inst_struck_marimba_faustpower2_f(fConst9));
		fConst11 = 1.0f / fConst9;
		fConst12 = (fConst11 + -1.4142135f) / fConst9 + 1.0f;
		fConst13 = 1.0f / ((fConst11 + 1.4142135f) / fConst9 + 1.0f);
		fConst14 = std::tan(31.415926f / fConst0);
		fConst15 = Inst_struck_marimba_faustpower2_f(fConst14);
		fConst16 = 2.0f * (1.0f - 1.0f / fConst15);
		fConst17 = 1.0f / fConst14;
		fConst18 = (fConst17 + -1.4142135f) / fConst14 + 1.0f;
		fConst19 = (fConst17 + 1.4142135f) / fConst14 + 1.0f;
		fConst20 = 1.0f / fConst19;
		fConst21 = 1.0f / (fConst15 * fConst19);
		fConst22 = std::pow(0.001f, 2.7279024e+08f / fConst0);
		fConst23 = Inst_struck_marimba_faustpower2_f(fConst22);
		fConst24 = 1207.5654f / fConst0;
		fConst25 = 2.0f * fConst22;
		fConst26 = std::pow(0.001f, 3.794452e+07f / fConst0);
		fConst27 = Inst_struck_marimba_faustpower2_f(fConst26);
		fConst28 = 1196.1237f / fConst0;
		fConst29 = 2.0f * fConst26;
		fConst30 = std::pow(0.001f, 1573805.6f / fConst0);
		fConst31 = Inst_struck_marimba_faustpower2_f(fConst30);
		fConst32 = 1164.89f / fConst0;
		fConst33 = 2.0f * fConst30;
		fConst34 = std::pow(0.001f, 324983.2f / fConst0);
		fConst35 = Inst_struck_marimba_faustpower2_f(fConst34);
		fConst36 = 1140.285f / fConst0;
		fConst37 = 2.0f * fConst34;
		fConst38 = std::pow(0.001f, 144580.97f / fConst0);
		fConst39 = Inst_struck_marimba_faustpower2_f(fConst38);
		fConst40 = 1124.2943f / fConst0;
		fConst41 = 2.0f * fConst38;
		fConst42 = std::pow(0.001f, 45016.383f / fConst0);
		fConst43 = Inst_struck_marimba_faustpower2_f(fConst42);
		fConst44 = 1096.1897f / fConst0;
		fConst45 = 2.0f * fConst42;
		fConst46 = std::pow(0.001f, 25426.668f / fConst0);
		fConst47 = Inst_struck_marimba_faustpower2_f(fConst46);
		fConst48 = 1079.847f / fConst0;
		fConst49 = 2.0f * fConst46;
		fConst50 = std::pow(0.001f, 12317.6455f / fConst0);
		fConst51 = Inst_struck_marimba_faustpower2_f(fConst50);
		fConst52 = 1056.2349f / fConst0;
		fConst53 = 2.0f * fConst50;
		fConst54 = std::pow(0.001f, 2684.1316f / fConst0);
		fConst55 = Inst_struck_marimba_faustpower2_f(fConst54);
		fConst56 = 993.8931f / fConst0;
		fConst57 = 2.0f * fConst54;
		fConst58 = std::pow(0.001f, 1927.5494f / fConst0);
		fConst59 = Inst_struck_marimba_faustpower2_f(fConst58);
		fConst60 = 977.6448f / fConst0;
		fConst61 = 2.0f * fConst58;
		fConst62 = std::pow(0.001f, 1438.7755f / fConst0);
		fConst63 = Inst_struck_marimba_faustpower2_f(fConst62);
		fConst64 = 962.37036f / fConst0;
		fConst65 = 2.0f * fConst62;
		fConst66 = std::pow(0.001f, 1262.5343f / fConst0);
		fConst67 = Inst_struck_marimba_faustpower2_f(fConst66);
		fConst68 = 955.2515f / fConst0;
		fConst69 = 2.0f * fConst66;
		fConst70 = std::pow(0.001f, 947.5349f / fConst0);
		fConst71 = Inst_struck_marimba_faustpower2_f(fConst70);
		fConst72 = 938.94666f / fConst0;
		fConst73 = 2.0f * fConst70;
		fConst74 = std::pow(0.001f, 574.8899f / fConst0);
		fConst75 = Inst_struck_marimba_faustpower2_f(fConst74);
		fConst76 = 908.22815f / fConst0;
		fConst77 = 2.0f * fConst74;
		fConst78 = std::pow(0.001f, 216.77945f / fConst0);
		fConst79 = Inst_struck_marimba_faustpower2_f(fConst78);
		fConst80 = 838.6607f / fConst0;
		fConst81 = 2.0f * fConst78;
		fConst82 = std::pow(0.001f, 196.83073f / fConst0);
		fConst83 = Inst_struck_marimba_faustpower2_f(fConst82);
		fConst84 = 831.0078f / fConst0;
		fConst85 = 2.0f * fConst82;
		fConst86 = std::pow(0.001f, 165.1287f / fConst0);
		fConst87 = Inst_struck_marimba_faustpower2_f(fConst86);
		fConst88 = 816.701f / fConst0;
		fConst89 = 2.0f * fConst86;
		fConst90 = std::pow(0.001f, 132.81357f / fConst0);
		fConst91 = Inst_struck_marimba_faustpower2_f(fConst90);
		fConst92 = 798.24725f / fConst0;
		fConst93 = 2.0f * fConst90;
		fConst94 = std::pow(0.001f, 78.259705f / fConst0);
		fConst95 = Inst_struck_marimba_faustpower2_f(fConst94);
		fConst96 = 749.93585f / fConst0;
		fConst97 = 2.0f * fConst94;
		fConst98 = std::pow(0.001f, 55.229492f / fConst0);
		fConst99 = Inst_struck_marimba_faustpower2_f(fConst98);
		fConst100 = 715.1899f / fConst0;
		fConst101 = 2.0f * fConst98;
		fConst102 = std::pow(0.001f, 49.682823f / fConst0);
		fConst103 = Inst_struck_marimba_faustpower2_f(fConst102);
		fConst104 = 704.15027f / fConst0;
		fConst105 = 2.0f * fConst102;
		fConst106 = std::pow(0.001f, 45.605556f / fConst0);
		fConst107 = Inst_struck_marimba_faustpower2_f(fConst106);
		fConst108 = 695.04596f / fConst0;
		fConst109 = 2.0f * fConst106;
		fConst110 = std::pow(0.001f, 22.286364f / fConst0);
		fConst111 = Inst_struck_marimba_faustpower2_f(fConst110);
		fConst112 = 612.4893f / fConst0;
		fConst113 = 2.0f * fConst110;
		fConst114 = std::pow(0.001f, 19.86714f / fConst0);
		fConst115 = Inst_struck_marimba_faustpower2_f(fConst114);
		fConst116 = 598.1052f / fConst0;
		fConst117 = 2.0f * fConst114;
		fConst118 = std::pow(0.001f, 17.811546f / fConst0);
		fConst119 = Inst_struck_marimba_faustpower2_f(fConst118);
		fConst120 = 584.1232f / fConst0;
		fConst121 = 2.0f * fConst118;
		fConst122 = std::pow(0.001f, 15.438163f / fConst0);
		fConst123 = Inst_struck_marimba_faustpower2_f(fConst122);
		fConst124 = 565.34845f / fConst0;
		fConst125 = 2.0f * fConst122;
		fConst126 = std::pow(0.001f, 10.208148f / fConst0);
		fConst127 = Inst_struck_marimba_faustpower2_f(fConst126);
		fConst128 = 507.917f / fConst0;
		fConst129 = 2.0f * fConst126;
		fConst130 = std::pow(0.001f, 9.307836f / fConst0);
		fConst131 = Inst_struck_marimba_faustpower2_f(fConst130);
		fConst132 = 494.43643f / fConst0;
		fConst133 = 2.0f * fConst130;
		fConst134 = std::pow(0.001f, 7.818347f / fConst0);
		fConst135 = Inst_struck_marimba_faustpower2_f(fConst134);
		fConst136 = 468.28644f / fConst0;
		fConst137 = 2.0f * fConst134;
		fConst138 = std::pow(0.001f, 5.777866f / fConst0);
		fConst139 = Inst_struck_marimba_faustpower2_f(fConst138);
		fConst140 = 420.71393f / fConst0;
		fConst141 = 2.0f * fConst138;
		fConst142 = std::pow(0.001f, 5.433348f / fConst0);
		fConst143 = Inst_struck_marimba_faustpower2_f(fConst142);
		fConst144 = 410.68658f / fConst0;
		fConst145 = 2.0f * fConst142;
		fConst146 = std::pow(0.001f, 5.0711775f / fConst0);
		fConst147 = Inst_struck_marimba_faustpower2_f(fConst146);
		fConst148 = 399.28763f / fConst0;
		fConst149 = 2.0f * fConst146;
		fConst150 = std::pow(0.001f, 4.173236f / fConst0);
		fConst151 = Inst_struck_marimba_faustpower2_f(fConst150);
		fConst152 = 366.22174f / fConst0;
		fConst153 = 2.0f * fConst150;
		fConst154 = std::pow(0.001f, 3.3885658f / fConst0);
		fConst155 = Inst_struck_marimba_faustpower2_f(fConst154);
		fConst156 = 329.4274f / fConst0;
		fConst157 = 2.0f * fConst154;
		fConst158 = std::pow(0.001f, 3.2268023f / fConst0);
		fConst159 = Inst_struck_marimba_faustpower2_f(fConst158);
		fConst160 = 320.56183f / fConst0;
		fConst161 = 2.0f * fConst158;
		fConst162 = std::pow(0.001f, 2.875977f / fConst0);
		fConst163 = Inst_struck_marimba_faustpower2_f(fConst162);
		fConst164 = 299.35544f / fConst0;
		fConst165 = 2.0f * fConst162;
		fConst166 = std::pow(0.001f, 2.2415073f / fConst0);
		fConst167 = Inst_struck_marimba_faustpower2_f(fConst166);
		fConst168 = 251.72577f / fConst0;
		fConst169 = 2.0f * fConst166;
		fConst170 = std::pow(0.001f, 2.0902803f / fConst0);
		fConst171 = Inst_struck_marimba_faustpower2_f(fConst170);
		fConst172 = 237.9461f / fConst0;
		fConst173 = 2.0f * fConst170;
		fConst174 = std::pow(0.001f, 2.0383441f / fConst0);
		fConst175 = Inst_struck_marimba_faustpower2_f(fConst174);
		fConst176 = 232.93527f / fConst0;
		fConst177 = 2.0f * fConst174;
		fConst178 = std::pow(0.001f, 1.5906833f / fConst0);
		fConst179 = Inst_struck_marimba_faustpower2_f(fConst178);
		fConst180 = 182.17719f / fConst0;
		fConst181 = 2.0f * fConst178;
		fConst182 = std::pow(0.001f, 1.4630294f / fConst0);
		fConst183 = Inst_struck_marimba_faustpower2_f(fConst182);
		fConst184 = 164.47809f / fConst0;
		fConst185 = 2.0f * fConst182;
		fConst186 = std::pow(0.001f, 1.259356f / fConst0);
		fConst187 = Inst_struck_marimba_faustpower2_f(fConst186);
		fConst188 = 132.01099f / fConst0;
		fConst189 = 2.0f * fConst186;
		fConst190 = std::pow(0.001f, 1.1689435f / fConst0);
		fConst191 = Inst_struck_marimba_faustpower2_f(fConst190);
		fConst192 = 115.51008f / fConst0;
		fConst193 = 2.0f * fConst190;
		fConst194 = std::pow(0.001f, 1.0380763f / fConst0);
		fConst195 = Inst_struck_marimba_faustpower2_f(fConst194);
		fConst196 = 88.6991f / fConst0;
		fConst197 = 2.0f * fConst194;
		fConst198 = std::pow(0.001f, 0.91448265f / fConst0);
		fConst199 = Inst_struck_marimba_faustpower2_f(fConst198);
		fConst200 = 59.362152f / fConst0;
		fConst201 = 2.0f * fConst198;
		fConst202 = std::pow(0.001f, 0.8812797f / fConst0);
		fConst203 = Inst_struck_marimba_faustpower2_f(fConst202);
		fConst204 = 50.66214f / fConst0;
		fConst205 = 2.0f * fConst202;
		fConst206 = std::pow(0.001f, 0.7884739f / fConst0);
		fConst207 = Inst_struck_marimba_faustpower2_f(fConst206);
		fConst208 = 24.094069f / fConst0;
		fConst209 = 2.0f * fConst206;
		fConst210 = std::pow(0.001f, 0.7778666f / fConst0);
		fConst211 = Inst_struck_marimba_faustpower2_f(fConst210);
		fConst212 = 20.819712f / fConst0;
		fConst213 = 2.0f * fConst210;
		fConst214 = std::pow(0.001f, 0.73279387f / fConst0);
		fConst215 = Inst_struck_marimba_faustpower2_f(fConst214);
		fConst216 = 6.2831855f / fConst0;
		fConst217 = 2.0f * fConst214;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = static_cast<FAUSTFLOAT>(4.4e+02f);
		fHslider1 = static_cast<FAUSTFLOAT>(0.5f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.0f);
		fHslider3 = static_cast<FAUSTFLOAT>(0.5f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iRec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec4[l1] = 0.0f;
		}
		IOTA0 = 0;
		for (int l2 = 0; l2 < 2048; l2 = l2 + 1) {
			fVec0[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec2[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec1[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			iRec6[l5] = 0;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			iRec9[l6] = 0;
		}
		for (int l7 = 0; l7 < 3; l7 = l7 + 1) {
			fRec8[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 3; l8 = l8 + 1) {
			fRec7[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 3; l9 = l9 + 1) {
			fRec5[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 3; l10 = l10 + 1) {
			fRec10[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = l11 + 1) {
			fRec11[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 3; l12 = l12 + 1) {
			fRec12[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 3; l13 = l13 + 1) {
			fRec13[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 3; l14 = l14 + 1) {
			fRec14[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 3; l15 = l15 + 1) {
			fRec15[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 3; l16 = l16 + 1) {
			fRec16[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 3; l17 = l17 + 1) {
			fRec17[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 3; l18 = l18 + 1) {
			fRec18[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 3; l19 = l19 + 1) {
			fRec19[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 3; l20 = l20 + 1) {
			fRec20[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 3; l21 = l21 + 1) {
			fRec21[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 3; l22 = l22 + 1) {
			fRec22[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 3; l23 = l23 + 1) {
			fRec23[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 3; l24 = l24 + 1) {
			fRec24[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 3; l25 = l25 + 1) {
			fRec25[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 3; l26 = l26 + 1) {
			fRec26[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 3; l27 = l27 + 1) {
			fRec27[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 3; l28 = l28 + 1) {
			fRec28[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 3; l29 = l29 + 1) {
			fRec29[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 3; l30 = l30 + 1) {
			fRec30[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 3; l31 = l31 + 1) {
			fRec31[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 3; l32 = l32 + 1) {
			fRec32[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 3; l33 = l33 + 1) {
			fRec33[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 3; l34 = l34 + 1) {
			fRec34[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 3; l35 = l35 + 1) {
			fRec35[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 3; l36 = l36 + 1) {
			fRec36[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 3; l37 = l37 + 1) {
			fRec37[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 3; l38 = l38 + 1) {
			fRec38[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 3; l39 = l39 + 1) {
			fRec39[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 3; l40 = l40 + 1) {
			fRec40[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 3; l41 = l41 + 1) {
			fRec41[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 3; l42 = l42 + 1) {
			fRec42[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 3; l43 = l43 + 1) {
			fRec43[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 3; l44 = l44 + 1) {
			fRec44[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 3; l45 = l45 + 1) {
			fRec45[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 3; l46 = l46 + 1) {
			fRec46[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 3; l47 = l47 + 1) {
			fRec47[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 3; l48 = l48 + 1) {
			fRec48[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 3; l49 = l49 + 1) {
			fRec49[l49] = 0.0f;
		}
		for (int l50 = 0; l50 < 3; l50 = l50 + 1) {
			fRec50[l50] = 0.0f;
		}
		for (int l51 = 0; l51 < 3; l51 = l51 + 1) {
			fRec51[l51] = 0.0f;
		}
		for (int l52 = 0; l52 < 3; l52 = l52 + 1) {
			fRec52[l52] = 0.0f;
		}
		for (int l53 = 0; l53 < 3; l53 = l53 + 1) {
			fRec53[l53] = 0.0f;
		}
		for (int l54 = 0; l54 < 3; l54 = l54 + 1) {
			fRec54[l54] = 0.0f;
		}
		for (int l55 = 0; l55 < 3; l55 = l55 + 1) {
			fRec55[l55] = 0.0f;
		}
		for (int l56 = 0; l56 < 3; l56 = l56 + 1) {
			fRec56[l56] = 0.0f;
		}
		for (int l57 = 0; l57 < 3; l57 = l57 + 1) {
			fRec57[l57] = 0.0f;
		}
		for (int l58 = 0; l58 < 3; l58 = l58 + 1) {
			fRec58[l58] = 0.0f;
		}
		for (int l59 = 0; l59 < 2; l59 = l59 + 1) {
			fVec2[l59] = 0.0f;
		}
		for (int l60 = 0; l60 < 2; l60 = l60 + 1) {
			fRec59[l60] = 0.0f;
		}
		for (int l61 = 0; l61 < 2048; l61 = l61 + 1) {
			fVec3[l61] = 0.0f;
		}
		for (int l62 = 0; l62 < 2; l62 = l62 + 1) {
			fRec3[l62] = 0.0f;
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
	
	virtual Inst_struck_marimba* clone() {
		return new Inst_struck_marimba();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("Marimba");
		ui_interface->addHorizontalSlider("freq", &fHslider0, FAUSTFLOAT(4.4e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(5e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("gate", &fHslider2, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("slide", &fHslider1, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("velocity", &fHslider3, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fHslider0);
		float fSlow1 = fConst2 * (3.4e+02f / fSlow0 + -0.04f);
		float fSlow2 = fSlow1 + -1.499995f;
		int iSlow3 = static_cast<int>(fSlow2);
		int iSlow4 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow3 + 4))));
		float fSlow5 = std::floor(fSlow2);
		float fSlow6 = fSlow1 + (-3.0f - fSlow5);
		float fSlow7 = fSlow1 + (-2.0f - fSlow5);
		float fSlow8 = fSlow1 + (-1.0f - fSlow5);
		float fSlow9 = fSlow1 - fSlow5;
		float fSlow10 = fSlow9 * fSlow8;
		float fSlow11 = fSlow10 * fSlow7;
		float fSlow12 = 0.041666668f * fSlow11 * fSlow6;
		int iSlow13 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow3 + 3))));
		float fSlow14 = 0.16666667f * fSlow11;
		int iSlow15 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow3 + 2))));
		float fSlow16 = 0.25f * fSlow10;
		int iSlow17 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow3 + 1))));
		float fSlow18 = 0.16666667f * fSlow9;
		int iSlow19 = static_cast<int>(std::min<float>(fConst1, static_cast<float>(std::max<int>(0, iSlow3))));
		float fSlow20 = 0.041666668f * fSlow8;
		float fSlow21 = fSlow1 + (-4.0f - fSlow5);
		int iSlow22 = 50 * static_cast<int>(3.99f * static_cast<float>(fHslider1));
		float fSlow23 = (((195.505f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 49] : 0.0f);
		float fSlow24 = fConst7 * std::cos(fConst6 * fSlow0);
		float fSlow25 = static_cast<float>(fHslider2);
		float fSlow26 = fConst13 * (0.7f * static_cast<float>(fHslider3) + 0.3f);
		float fSlow27 = (((192.19f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 48] : 0.0f);
		float fSlow28 = fConst25 * std::cos(fConst24 * fSlow0);
		float fSlow29 = (((190.369f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 47] : 0.0f);
		float fSlow30 = fConst29 * std::cos(fConst28 * fSlow0);
		float fSlow31 = (((185.398f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 46] : 0.0f);
		float fSlow32 = fConst33 * std::cos(fConst32 * fSlow0);
		float fSlow33 = (((181.482f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 45] : 0.0f);
		float fSlow34 = fConst37 * std::cos(fConst36 * fSlow0);
		float fSlow35 = (((178.937f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 44] : 0.0f);
		float fSlow36 = fConst41 * std::cos(fConst40 * fSlow0);
		float fSlow37 = (((174.464f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 43] : 0.0f);
		float fSlow38 = fConst45 * std::cos(fConst44 * fSlow0);
		float fSlow39 = (((171.863f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 42] : 0.0f);
		float fSlow40 = fConst49 * std::cos(fConst48 * fSlow0);
		float fSlow41 = (((168.105f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 41] : 0.0f);
		float fSlow42 = fConst53 * std::cos(fConst52 * fSlow0);
		float fSlow43 = (((158.183f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 40] : 0.0f);
		float fSlow44 = fConst57 * std::cos(fConst56 * fSlow0);
		float fSlow45 = (((155.597f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 39] : 0.0f);
		float fSlow46 = fConst61 * std::cos(fConst60 * fSlow0);
		float fSlow47 = (((153.166f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 38] : 0.0f);
		float fSlow48 = fConst65 * std::cos(fConst64 * fSlow0);
		float fSlow49 = (((152.033f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 37] : 0.0f);
		float fSlow50 = fConst69 * std::cos(fConst68 * fSlow0);
		float fSlow51 = (((149.438f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 36] : 0.0f);
		float fSlow52 = fConst73 * std::cos(fConst72 * fSlow0);
		float fSlow53 = (((144.549f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 35] : 0.0f);
		float fSlow54 = fConst77 * std::cos(fConst76 * fSlow0);
		float fSlow55 = (((133.477f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 34] : 0.0f);
		float fSlow56 = fConst81 * std::cos(fConst80 * fSlow0);
		float fSlow57 = (((132.259f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 33] : 0.0f);
		float fSlow58 = fConst85 * std::cos(fConst84 * fSlow0);
		float fSlow59 = (((129.982f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 32] : 0.0f);
		float fSlow60 = fConst89 * std::cos(fConst88 * fSlow0);
		float fSlow61 = (((127.045f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 31] : 0.0f);
		float fSlow62 = fConst93 * std::cos(fConst92 * fSlow0);
		float fSlow63 = (((119.356f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 30] : 0.0f);
		float fSlow64 = fConst97 * std::cos(fConst96 * fSlow0);
		float fSlow65 = (((113.826f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 29] : 0.0f);
		float fSlow66 = fConst101 * std::cos(fConst100 * fSlow0);
		float fSlow67 = (((112.069f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 28] : 0.0f);
		float fSlow68 = fConst105 * std::cos(fConst104 * fSlow0);
		float fSlow69 = (((110.62f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 27] : 0.0f);
		float fSlow70 = fConst109 * std::cos(fConst108 * fSlow0);
		float fSlow71 = (((97.4807f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 26] : 0.0f);
		float fSlow72 = fConst113 * std::cos(fConst112 * fSlow0);
		float fSlow73 = (((95.1914f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 25] : 0.0f);
		float fSlow74 = fConst117 * std::cos(fConst116 * fSlow0);
		float fSlow75 = (((92.9661f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 24] : 0.0f);
		float fSlow76 = fConst121 * std::cos(fConst120 * fSlow0);
		float fSlow77 = (((89.978f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 23] : 0.0f);
		float fSlow78 = fConst125 * std::cos(fConst124 * fSlow0);
		float fSlow79 = (((80.8375f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 22] : 0.0f);
		float fSlow80 = fConst129 * std::cos(fConst128 * fSlow0);
		float fSlow81 = (((78.692f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 21] : 0.0f);
		float fSlow82 = fConst133 * std::cos(fConst132 * fSlow0);
		float fSlow83 = (((74.5301f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 20] : 0.0f);
		float fSlow84 = fConst137 * std::cos(fConst136 * fSlow0);
		float fSlow85 = (((66.9587f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 19] : 0.0f);
		float fSlow86 = fConst141 * std::cos(fConst140 * fSlow0);
		float fSlow87 = (((65.3628f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 18] : 0.0f);
		float fSlow88 = fConst145 * std::cos(fConst144 * fSlow0);
		float fSlow89 = (((63.5486f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 17] : 0.0f);
		float fSlow90 = fConst149 * std::cos(fConst148 * fSlow0);
		float fSlow91 = (((58.286f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 16] : 0.0f);
		float fSlow92 = fConst153 * std::cos(fConst152 * fSlow0);
		float fSlow93 = (((52.43f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 15] : 0.0f);
		float fSlow94 = fConst157 * std::cos(fConst156 * fSlow0);
		float fSlow95 = (((51.019f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 14] : 0.0f);
		float fSlow96 = fConst161 * std::cos(fConst160 * fSlow0);
		float fSlow97 = (((47.6439f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 13] : 0.0f);
		float fSlow98 = fConst165 * std::cos(fConst164 * fSlow0);
		float fSlow99 = (((40.0634f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 12] : 0.0f);
		float fSlow100 = fConst169 * std::cos(fConst168 * fSlow0);
		float fSlow101 = (((37.8703f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 11] : 0.0f);
		float fSlow102 = fConst173 * std::cos(fConst172 * fSlow0);
		float fSlow103 = (((37.0728f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 10] : 0.0f);
		float fSlow104 = fConst177 * std::cos(fConst176 * fSlow0);
		float fSlow105 = (((28.9944f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 9] : 0.0f);
		float fSlow106 = fConst181 * std::cos(fConst180 * fSlow0);
		float fSlow107 = (((26.1775f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 8] : 0.0f);
		float fSlow108 = fConst185 * std::cos(fConst184 * fSlow0);
		float fSlow109 = (((21.0102f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 7] : 0.0f);
		float fSlow110 = fConst189 * std::cos(fConst188 * fSlow0);
		float fSlow111 = (((18.384f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 6] : 0.0f);
		float fSlow112 = fConst193 * std::cos(fConst192 * fSlow0);
		float fSlow113 = (((14.1169f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 5] : 0.0f);
		float fSlow114 = fConst197 * std::cos(fConst196 * fSlow0);
		float fSlow115 = (((9.44778f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 4] : 0.0f);
		float fSlow116 = fConst201 * std::cos(fConst200 * fSlow0);
		float fSlow117 = (((8.06313f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 3] : 0.0f);
		float fSlow118 = fConst205 * std::cos(fConst204 * fSlow0);
		float fSlow119 = (((3.83469f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 2] : 0.0f);
		float fSlow120 = fConst209 * std::cos(fConst208 * fSlow0);
		float fSlow121 = (((3.31356f * fSlow0) < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22 + 1] : 0.0f);
		float fSlow122 = fConst213 * std::cos(fConst212 * fSlow0);
		float fSlow123 = ((fSlow0 < fConst3) ? ftbl0Inst_struck_marimbaSIG0[iSlow22] : 0.0f);
		float fSlow124 = fConst217 * std::cos(fConst216 * fSlow0);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iRec0[0] = 0;
			fRec4[0] = 0.05f * fRec3[1] + 0.95f * fRec4[1];
			float fTemp0 = 0.99f * fRec4[0] + static_cast<float>(iRec0[1]);
			fVec0[IOTA0 & 2047] = fTemp0;
			fRec2[0] = fSlow21 * (fSlow6 * (fSlow7 * (fSlow20 * fVec0[(IOTA0 - iSlow19) & 2047] - fSlow18 * fVec0[(IOTA0 - iSlow17) & 2047]) + fSlow16 * fVec0[(IOTA0 - iSlow15) & 2047]) - fSlow14 * fVec0[(IOTA0 - iSlow13) & 2047]) + fSlow12 * fVec0[(IOTA0 - iSlow4) & 2047];
			fVec1[0] = fSlow25;
			iRec6[0] = (iRec6[1] + (iRec6[1] > 0)) * (fSlow25 <= fVec1[1]) + (fSlow25 > fVec1[1]);
			float fTemp1 = fConst8 * static_cast<float>(iRec6[0]);
			iRec9[0] = 1103515245 * iRec9[1] + 12345;
			fRec8[0] = 4.656613e-10f * static_cast<float>(iRec9[0]) - fConst20 * (fConst18 * fRec8[2] + fConst16 * fRec8[1]);
			fRec7[0] = fConst21 * (fRec8[2] + (fRec8[0] - 2.0f * fRec8[1])) - fConst13 * (fConst12 * fRec7[2] + fConst10 * fRec7[1]);
			float fTemp2 = fSlow26 * (fRec7[2] + fRec7[0] + 2.0f * fRec7[1]) * std::max<float>(0.0f, std::min<float>(fTemp1, 2.0f - fTemp1));
			fRec5[0] = fTemp2 + fSlow24 * fRec5[1] - fConst5 * fRec5[2];
			fRec10[0] = fTemp2 + fSlow28 * fRec10[1] - fConst23 * fRec10[2];
			fRec11[0] = fTemp2 + fSlow30 * fRec11[1] - fConst27 * fRec11[2];
			fRec12[0] = fTemp2 + fSlow32 * fRec12[1] - fConst31 * fRec12[2];
			fRec13[0] = fTemp2 + fSlow34 * fRec13[1] - fConst35 * fRec13[2];
			fRec14[0] = fTemp2 + fSlow36 * fRec14[1] - fConst39 * fRec14[2];
			fRec15[0] = fTemp2 + fSlow38 * fRec15[1] - fConst43 * fRec15[2];
			fRec16[0] = fTemp2 + fSlow40 * fRec16[1] - fConst47 * fRec16[2];
			fRec17[0] = fTemp2 + fSlow42 * fRec17[1] - fConst51 * fRec17[2];
			fRec18[0] = fTemp2 + fSlow44 * fRec18[1] - fConst55 * fRec18[2];
			fRec19[0] = fTemp2 + fSlow46 * fRec19[1] - fConst59 * fRec19[2];
			fRec20[0] = fTemp2 + fSlow48 * fRec20[1] - fConst63 * fRec20[2];
			fRec21[0] = fTemp2 + fSlow50 * fRec21[1] - fConst67 * fRec21[2];
			fRec22[0] = fTemp2 + fSlow52 * fRec22[1] - fConst71 * fRec22[2];
			fRec23[0] = fTemp2 + fSlow54 * fRec23[1] - fConst75 * fRec23[2];
			fRec24[0] = fTemp2 + fSlow56 * fRec24[1] - fConst79 * fRec24[2];
			fRec25[0] = fTemp2 + fSlow58 * fRec25[1] - fConst83 * fRec25[2];
			fRec26[0] = fTemp2 + fSlow60 * fRec26[1] - fConst87 * fRec26[2];
			fRec27[0] = fTemp2 + fSlow62 * fRec27[1] - fConst91 * fRec27[2];
			fRec28[0] = fTemp2 + fSlow64 * fRec28[1] - fConst95 * fRec28[2];
			fRec29[0] = fTemp2 + fSlow66 * fRec29[1] - fConst99 * fRec29[2];
			fRec30[0] = fTemp2 + fSlow68 * fRec30[1] - fConst103 * fRec30[2];
			fRec31[0] = fTemp2 + fSlow70 * fRec31[1] - fConst107 * fRec31[2];
			fRec32[0] = fTemp2 + fSlow72 * fRec32[1] - fConst111 * fRec32[2];
			fRec33[0] = fTemp2 + fSlow74 * fRec33[1] - fConst115 * fRec33[2];
			fRec34[0] = fTemp2 + fSlow76 * fRec34[1] - fConst119 * fRec34[2];
			fRec35[0] = fTemp2 + fSlow78 * fRec35[1] - fConst123 * fRec35[2];
			fRec36[0] = fTemp2 + fSlow80 * fRec36[1] - fConst127 * fRec36[2];
			fRec37[0] = fTemp2 + fSlow82 * fRec37[1] - fConst131 * fRec37[2];
			fRec38[0] = fTemp2 + fSlow84 * fRec38[1] - fConst135 * fRec38[2];
			fRec39[0] = fTemp2 + fSlow86 * fRec39[1] - fConst139 * fRec39[2];
			fRec40[0] = fTemp2 + fSlow88 * fRec40[1] - fConst143 * fRec40[2];
			fRec41[0] = fTemp2 + fSlow90 * fRec41[1] - fConst147 * fRec41[2];
			fRec42[0] = fTemp2 + fSlow92 * fRec42[1] - fConst151 * fRec42[2];
			fRec43[0] = fTemp2 + fSlow94 * fRec43[1] - fConst155 * fRec43[2];
			fRec44[0] = fTemp2 + fSlow96 * fRec44[1] - fConst159 * fRec44[2];
			fRec45[0] = fTemp2 + fSlow98 * fRec45[1] - fConst163 * fRec45[2];
			fRec46[0] = fTemp2 + fSlow100 * fRec46[1] - fConst167 * fRec46[2];
			fRec47[0] = fTemp2 + fSlow102 * fRec47[1] - fConst171 * fRec47[2];
			fRec48[0] = fTemp2 + fSlow104 * fRec48[1] - fConst175 * fRec48[2];
			fRec49[0] = fTemp2 + fSlow106 * fRec49[1] - fConst179 * fRec49[2];
			fRec50[0] = fTemp2 + fSlow108 * fRec50[1] - fConst183 * fRec50[2];
			fRec51[0] = fTemp2 + fSlow110 * fRec51[1] - fConst187 * fRec51[2];
			fRec52[0] = fTemp2 + fSlow112 * fRec52[1] - fConst191 * fRec52[2];
			fRec53[0] = fTemp2 + fSlow114 * fRec53[1] - fConst195 * fRec53[2];
			fRec54[0] = fTemp2 + fSlow116 * fRec54[1] - fConst199 * fRec54[2];
			fRec55[0] = fTemp2 + fSlow118 * fRec55[1] - fConst203 * fRec55[2];
			fRec56[0] = fTemp2 + fSlow120 * fRec56[1] - fConst207 * fRec56[2];
			fRec57[0] = fTemp2 + fSlow122 * fRec57[1] - fConst211 * fRec57[2];
			fRec58[0] = fTemp2 + fSlow124 * fRec58[1] - fConst215 * fRec58[2];
			fVec2[0] = (fRec58[0] - fRec58[2]) * fSlow123 + (fRec57[0] - fRec57[2]) * fSlow121 + (fRec56[0] - fRec56[2]) * fSlow119 + (fRec55[0] - fRec55[2]) * fSlow117 + (fRec54[0] - fRec54[2]) * fSlow115 + (fRec53[0] - fRec53[2]) * fSlow113 + (fRec52[0] - fRec52[2]) * fSlow111 + (fRec51[0] - fRec51[2]) * fSlow109 + (fRec50[0] - fRec50[2]) * fSlow107 + (fRec49[0] - fRec49[2]) * fSlow105 + (fRec48[0] - fRec48[2]) * fSlow103 + (fRec47[0] - fRec47[2]) * fSlow101 + (fRec46[0] - fRec46[2]) * fSlow99 + (fRec45[0] - fRec45[2]) * fSlow97 + (fRec44[0] - fRec44[2]) * fSlow95 + (fRec43[0] - fRec43[2]) * fSlow93 + (fRec42[0] - fRec42[2]) * fSlow91 + (fRec41[0] - fRec41[2]) * fSlow89 + (fRec40[0] - fRec40[2]) * fSlow87 + (fRec39[0] - fRec39[2]) * fSlow85 + (fRec38[0] - fRec38[2]) * fSlow83 + (fRec37[0] - fRec37[2]) * fSlow81 + (fRec36[0] - fRec36[2]) * fSlow79 + (fRec35[0] - fRec35[2]) * fSlow77 + (fRec34[0] - fRec34[2]) * fSlow75 + (fRec33[0] - fRec33[2]) * fSlow73 + (fRec32[0] - fRec32[2]) * fSlow71 + (fRec31[0] - fRec31[2]) * fSlow69 + (fRec30[0] - fRec30[2]) * fSlow67 + (fRec29[0] - fRec29[2]) * fSlow65 + (fRec28[0] - fRec28[2]) * fSlow63 + (fRec27[0] - fRec27[2]) * fSlow61 + (fRec26[0] - fRec26[2]) * fSlow59 + (fRec25[0] - fRec25[2]) * fSlow57 + (fRec24[0] - fRec24[2]) * fSlow55 + (fRec23[0] - fRec23[2]) * fSlow53 + (fRec22[0] - fRec22[2]) * fSlow51 + (fRec21[0] - fRec21[2]) * fSlow49 + (fRec20[0] - fRec20[2]) * fSlow47 + (fRec19[0] - fRec19[2]) * fSlow45 + (fRec18[0] - fRec18[2]) * fSlow43 + (fRec17[0] - fRec17[2]) * fSlow41 + (fRec16[0] - fRec16[2]) * fSlow39 + (fRec15[0] - fRec15[2]) * fSlow37 + (fRec14[0] - fRec14[2]) * fSlow35 + (fRec13[0] - fRec13[2]) * fSlow33 + (fRec12[0] - fRec12[2]) * fSlow31 + (fRec11[0] - fRec11[2]) * fSlow29 + (fRec10[0] - fRec10[2]) * fSlow27 + (fRec5[0] - fRec5[2]) * fSlow23;
			fRec59[0] = 0.05f * fRec2[1] + 0.95f * fRec59[1];
			float fTemp3 = 0.99f * fRec59[0] + 0.02f * fVec2[1];
			fVec3[IOTA0 & 2047] = fTemp3;
			fRec3[0] = fSlow21 * (fSlow6 * (fSlow7 * (fSlow20 * fVec3[(IOTA0 - iSlow19) & 2047] - fSlow18 * fVec3[(IOTA0 - iSlow17) & 2047]) + fSlow16 * fVec3[(IOTA0 - iSlow15) & 2047]) - fSlow14 * fVec3[(IOTA0 - iSlow13) & 2047]) + fSlow12 * fVec3[(IOTA0 - iSlow4) & 2047];
			float fRec1 = fRec3[0];
			output0[i0] = static_cast<FAUSTFLOAT>(2.0f * fRec1);
			iRec0[1] = iRec0[0];
			fRec4[1] = fRec4[0];
			IOTA0 = IOTA0 + 1;
			fRec2[1] = fRec2[0];
			fVec1[1] = fVec1[0];
			iRec6[1] = iRec6[0];
			iRec9[1] = iRec9[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec11[2] = fRec11[1];
			fRec11[1] = fRec11[0];
			fRec12[2] = fRec12[1];
			fRec12[1] = fRec12[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fRec14[2] = fRec14[1];
			fRec14[1] = fRec14[0];
			fRec15[2] = fRec15[1];
			fRec15[1] = fRec15[0];
			fRec16[2] = fRec16[1];
			fRec16[1] = fRec16[0];
			fRec17[2] = fRec17[1];
			fRec17[1] = fRec17[0];
			fRec18[2] = fRec18[1];
			fRec18[1] = fRec18[0];
			fRec19[2] = fRec19[1];
			fRec19[1] = fRec19[0];
			fRec20[2] = fRec20[1];
			fRec20[1] = fRec20[0];
			fRec21[2] = fRec21[1];
			fRec21[1] = fRec21[0];
			fRec22[2] = fRec22[1];
			fRec22[1] = fRec22[0];
			fRec23[2] = fRec23[1];
			fRec23[1] = fRec23[0];
			fRec24[2] = fRec24[1];
			fRec24[1] = fRec24[0];
			fRec25[2] = fRec25[1];
			fRec25[1] = fRec25[0];
			fRec26[2] = fRec26[1];
			fRec26[1] = fRec26[0];
			fRec27[2] = fRec27[1];
			fRec27[1] = fRec27[0];
			fRec28[2] = fRec28[1];
			fRec28[1] = fRec28[0];
			fRec29[2] = fRec29[1];
			fRec29[1] = fRec29[0];
			fRec30[2] = fRec30[1];
			fRec30[1] = fRec30[0];
			fRec31[2] = fRec31[1];
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
			fRec40[2] = fRec40[1];
			fRec40[1] = fRec40[0];
			fRec41[2] = fRec41[1];
			fRec41[1] = fRec41[0];
			fRec42[2] = fRec42[1];
			fRec42[1] = fRec42[0];
			fRec43[2] = fRec43[1];
			fRec43[1] = fRec43[0];
			fRec44[2] = fRec44[1];
			fRec44[1] = fRec44[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec46[2] = fRec46[1];
			fRec46[1] = fRec46[0];
			fRec47[2] = fRec47[1];
			fRec47[1] = fRec47[0];
			fRec48[2] = fRec48[1];
			fRec48[1] = fRec48[0];
			fRec49[2] = fRec49[1];
			fRec49[1] = fRec49[0];
			fRec50[2] = fRec50[1];
			fRec50[1] = fRec50[0];
			fRec51[2] = fRec51[1];
			fRec51[1] = fRec51[0];
			fRec52[2] = fRec52[1];
			fRec52[1] = fRec52[0];
			fRec53[2] = fRec53[1];
			fRec53[1] = fRec53[0];
			fRec54[2] = fRec54[1];
			fRec54[1] = fRec54[0];
			fRec55[2] = fRec55[1];
			fRec55[1] = fRec55[0];
			fRec56[2] = fRec56[1];
			fRec56[1] = fRec56[0];
			fRec57[2] = fRec57[1];
			fRec57[1] = fRec57[0];
			fRec58[2] = fRec58[1];
			fRec58[1] = fRec58[0];
			fVec2[1] = fVec2[0];
			fRec59[1] = fRec59[0];
			fRec3[1] = fRec3[0];
		}
	}

};

#endif
