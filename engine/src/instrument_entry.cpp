// Wraps one generated Faust instrument. Built once per .dsp with
//   GEN   path of the generated C++        CLASS  its Faust class name
//   FN    C identifier for this instrument (strings_cello)
#include <map>
#include <string>
#include "faust/dsp/dsp.h"
#include "faust/gui/meta.h"
#include "faust/gui/UI.h"
#if !defined(GEN) || !defined(CLASS) || !defined(FN)
#error "build with -DGEN=... -DCLASS=... -DFN=..."
#endif
#include GEN

#define JOIN2(a, b) a##b
#define JOIN(a, b) JOIN2(a, b)

dsp* JOIN(mpe_create_, FN)() { return new CLASS(); }

// Value of a `declare <key> "<value>";` line in the .dsp, or "" if absent.
const char* JOIN(mpe_meta_, FN)(const char* key) {
    static std::map<std::string, std::string> declared;
    static bool loaded = false;
    if (!loaded) {
        struct Collect : Meta {
            std::map<std::string, std::string>& out;
            explicit Collect(std::map<std::string, std::string>& o) : out(o) {}
            void declare(const char* k, const char* v) override { out[k] = v; }
        } collect(declared);
        CLASS tmp;
        tmp.metadata(&collect);
        loaded = true;
    }
    auto it = declared.find(key);
    return it == declared.end() ? "" : it->second.c_str();
}
