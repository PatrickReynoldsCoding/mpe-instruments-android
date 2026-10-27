#include "mpe/engine.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
dsp* mpe_create_plucked_nylon_guitar();
const char* mpe_meta_plucked_nylon_guitar(const char*);
dsp* mpe_create_plucked_steel_guitar();
const char* mpe_meta_plucked_steel_guitar(const char*);
dsp* mpe_create_strings_cello();
const char* mpe_meta_strings_cello(const char*);
dsp* mpe_create_strings_violin();
const char* mpe_meta_strings_violin(const char*);
dsp* mpe_create_struck_marimba();
const char* mpe_meta_struck_marimba(const char*);
dsp* mpe_create_struck_vibraphone();
const char* mpe_meta_struck_vibraphone(const char*);
dsp* mpe_create_synths_fm_keys();
const char* mpe_meta_synths_fm_keys(const char*);
dsp* mpe_create_synths_lead();
const char* mpe_meta_synths_lead(const char*);
dsp* mpe_create_synths_pad();
const char* mpe_meta_synths_pad(const char*);
dsp* mpe_create_voices_bass();
const char* mpe_meta_voices_bass(const char*);
dsp* mpe_create_voices_soprano();
const char* mpe_meta_voices_soprano(const char*);
dsp* mpe_create_voices_tenor();
const char* mpe_meta_voices_tenor(const char*);
dsp* mpe_create_winds_clarinet();
const char* mpe_meta_winds_clarinet(const char*);
dsp* mpe_create_winds_flute();
const char* mpe_meta_winds_flute(const char*);

namespace mpe {
static std::vector<InstrumentInfo> build() {
  std::vector<InstrumentInfo> v;
  { const char* n = mpe_meta_plucked_nylon_guitar("name"); const char* d = mpe_meta_plucked_nylon_guitar("description");
    v.push_back({ "plucked/nylon-guitar", "plucked", n[0] ? n : "nylon-guitar", d, mpe_create_plucked_nylon_guitar, mpe_meta_plucked_nylon_guitar }); }
  { const char* n = mpe_meta_plucked_steel_guitar("name"); const char* d = mpe_meta_plucked_steel_guitar("description");
    v.push_back({ "plucked/steel-guitar", "plucked", n[0] ? n : "steel-guitar", d, mpe_create_plucked_steel_guitar, mpe_meta_plucked_steel_guitar }); }
  { const char* n = mpe_meta_strings_cello("name"); const char* d = mpe_meta_strings_cello("description");
    v.push_back({ "strings/cello", "strings", n[0] ? n : "cello", d, mpe_create_strings_cello, mpe_meta_strings_cello }); }
  { const char* n = mpe_meta_strings_violin("name"); const char* d = mpe_meta_strings_violin("description");
    v.push_back({ "strings/violin", "strings", n[0] ? n : "violin", d, mpe_create_strings_violin, mpe_meta_strings_violin }); }
  { const char* n = mpe_meta_struck_marimba("name"); const char* d = mpe_meta_struck_marimba("description");
    v.push_back({ "struck/marimba", "struck", n[0] ? n : "marimba", d, mpe_create_struck_marimba, mpe_meta_struck_marimba }); }
  { const char* n = mpe_meta_struck_vibraphone("name"); const char* d = mpe_meta_struck_vibraphone("description");
    v.push_back({ "struck/vibraphone", "struck", n[0] ? n : "vibraphone", d, mpe_create_struck_vibraphone, mpe_meta_struck_vibraphone }); }
  { const char* n = mpe_meta_synths_fm_keys("name"); const char* d = mpe_meta_synths_fm_keys("description");
    v.push_back({ "synths/fm-keys", "synths", n[0] ? n : "fm-keys", d, mpe_create_synths_fm_keys, mpe_meta_synths_fm_keys }); }
  { const char* n = mpe_meta_synths_lead("name"); const char* d = mpe_meta_synths_lead("description");
    v.push_back({ "synths/lead", "synths", n[0] ? n : "lead", d, mpe_create_synths_lead, mpe_meta_synths_lead }); }
  { const char* n = mpe_meta_synths_pad("name"); const char* d = mpe_meta_synths_pad("description");
    v.push_back({ "synths/pad", "synths", n[0] ? n : "pad", d, mpe_create_synths_pad, mpe_meta_synths_pad }); }
  { const char* n = mpe_meta_voices_bass("name"); const char* d = mpe_meta_voices_bass("description");
    v.push_back({ "voices/bass", "voices", n[0] ? n : "bass", d, mpe_create_voices_bass, mpe_meta_voices_bass }); }
  { const char* n = mpe_meta_voices_soprano("name"); const char* d = mpe_meta_voices_soprano("description");
    v.push_back({ "voices/soprano", "voices", n[0] ? n : "soprano", d, mpe_create_voices_soprano, mpe_meta_voices_soprano }); }
  { const char* n = mpe_meta_voices_tenor("name"); const char* d = mpe_meta_voices_tenor("description");
    v.push_back({ "voices/tenor", "voices", n[0] ? n : "tenor", d, mpe_create_voices_tenor, mpe_meta_voices_tenor }); }
  { const char* n = mpe_meta_winds_clarinet("name"); const char* d = mpe_meta_winds_clarinet("description");
    v.push_back({ "winds/clarinet", "winds", n[0] ? n : "clarinet", d, mpe_create_winds_clarinet, mpe_meta_winds_clarinet }); }
  { const char* n = mpe_meta_winds_flute("name"); const char* d = mpe_meta_winds_flute("description");
    v.push_back({ "winds/flute", "winds", n[0] ? n : "flute", d, mpe_create_winds_flute, mpe_meta_winds_flute }); }
  return v;
}
const InstrumentInfo* instruments(int& count) {
  static const std::vector<InstrumentInfo> all = build();
  count = (int)all.size();
  return all.data();
}
int findInstrument(const char* id) {
  int n = 0; const InstrumentInfo* a = instruments(n);
  for (int i = 0; i < n; i++) if (std::strcmp(a[i].id, id) == 0) return i;
  return -1;
}
}  // namespace mpe
